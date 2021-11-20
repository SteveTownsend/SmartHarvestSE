/*************************************************************************
SmartHarvest SE
Copyright (c) Steve Townsend 2020

>>> SOURCE LICENSE >>>
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation (www.fsf.org); either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License is available at
http://www.fsf.org/licensing/licenses
>>> END OF LICENSE >>>
*************************************************************************/
#include "PrecompiledHeaders.h"
#include "Collections/Collection.h"
#include "Collections/CollectionFactory.h"
#include "Collections/CollectionManager.h"
#include "Data/LoadOrder.h"
#include "Utilities/utils.h"
#include "VM/papyrus.h"
#include "WorldState/PlayerState.h"
#include "WorldState/Saga.h"

namespace shse
{

void CollectionPolicy::AsJSON(nlohmann::json& j) const
{
	j["action"] = CollectibleHandlingString(m_action);
	j["notify"] = m_notify;
	j["repeat"] = m_repeat;
}

void to_json(nlohmann::json& j, const CollectionPolicy& policy)
{
	policy.AsJSON(j);
}

ItemCollected::ItemCollected(const RE::TESForm* item, const Collection* collection, const float gameTime) :
	m_item(item), m_collection(collection), m_gameTime(gameTime)
{
}

std::string ItemCollected::AsString() const
{
	std::ostringstream stream;
	stream << "I collected " << m_item->GetName() << " for " << m_collection->Name() << '.';
	return stream.str();
}

Collection::Collection(const CollectionGroup* owningGroup, const std::string& name, const std::string& description,
	const CollectionPolicy& policy,	const bool overridesGroup, std::unique_ptr<ItemRule> itemRule) :
	m_name(name), m_description(description), m_effectivePolicy(policy),
	m_overridesGroup(overridesGroup), m_itemRule(std::move(itemRule)), m_owningGroup(owningGroup)
{
}

Collection::~Collection()
{
}

// first element - does Collection determine disposition?
// second element - true for one-time Collectible, already processed: defer decision to other rules
std::tuple<bool, bool, bool> Collection::InScopeAndCollectibleFor(const ConditionMatcher& matcher) const
{
	bool inScope(false);
	bool repeat(false);
	bool observed(false);
	if (!matcher.Form())
		return { inScope, repeat, observed };

	// check Scope - if Collection is scoped, scope for this autoloot check must be valid
	if (!m_scopes.empty() && std::find(m_scopes.cbegin(), m_scopes.cend(), matcher.Scope()) == m_scopes.cend())
	{
		DBG_VMESSAGE("{}/0x{:08x} has invalid scope {}", matcher.Form()->GetName(), matcher.Form()->GetFormID(), int(matcher.Scope()));
		return { inScope, repeat, observed };
	}
	inScope = true;
	if (IsMemberOf(matcher))
	{
		// Collection member always handled if repeats allowed, or as-yet unobserved
		// If repeats are disallowed, the item is no longer Collectible after first observation
		// Defer to other rules if repeats are not allowed and this Collection is not dispositive for the item
		return { inScope, m_effectivePolicy.Repeat(), m_observed.contains(matcher.Form()) };
	}
	// absolutely not a member of the collection
	return { inScope, repeat, observed };
}

bool Collection::IsActive() const
{
	// Collections with no Members are not considered active.
	// Administrative groups are not MCM-managed and always-on. User Groups are active if Collections are MCM-enabled.
	return HasMembers() && (!m_owningGroup->UseMCM() || CollectionManager::Instance().IsMCMEnabled());
}

bool Collection::HaveObserved(const RE::TESForm* form) const
{ 
	return m_observed.contains(form);
}

bool Collection::RecordItem(const RE::TESForm* form, const float gameTime, const bool suppressSpam)
{
	DBG_VMESSAGE("Collect {}/0x{:08x} in {}", form->GetName(), form->GetFormID(), m_name.c_str());
	if (m_observed.insert({ form, gameTime }).second)
	{
		Saga::Instance().AddEvent(ItemCollected(form, this, gameTime));
		if (m_effectivePolicy.Notify())
		{
			// don't flood the screen for ages on one pass (especially first-time inventory reconciliation)
			if (!suppressSpam)
			{
				// notify about these, just once
				static RE::BSFixedString newMemberText(papyrus::GetTranslation(nullptr, RE::BSFixedString("$SHSE_ADDED_TO_COLLECTION")));
				if (!newMemberText.empty())
				{
					std::string notificationText(newMemberText);
					StringUtils::Replace(notificationText, "{ITEMNAME}", form->GetName());
					StringUtils::Replace(notificationText, "{COLLECTION}", m_name);
					if (!notificationText.empty())
					{
						RE::DebugNotification(notificationText.c_str());
					}
				}
			}
			return true;
		}
	}
	return false;
}

void Collection::Reset()
{
	m_observed.clear();
	m_scopes.clear();
	InitFromStaticMembers();
}

std::string Collection::Name(void) const
{
	return m_name;
}

std::string Collection::Description(void) const
{
	return m_description;
}

std::string Collection::PrintDefinition() const
{
	std::ostringstream collectionStr;
	collectionStr << *this;
	return collectionStr.str();
}

std::string Collection::PrintMembers(void) const
{
	std::ostringstream collectionStr;
	collectionStr << Count() << " members\n";
	if (!m_scopes.empty())
	{
		collectionStr << "Scope: ";
		size_t scopes(m_scopes.size());
		for (const auto scope : m_scopes)
		{
			collectionStr << ScopeCondition::SecondaryTypeAsScope(scope);
			if (--scopes)
			{
				collectionStr << ', ';
			}
			else
			{
				collectionStr << '\n';
			}
		}
	}
	PrintMemberDetails(collectionStr);
	return collectionStr.str();
}

nlohmann::json Collection::MakeJSON() const
{
	return nlohmann::json(*this);
}

void Collection::AsJSON(nlohmann::json& j) const
{
	j["name"] = m_name;
	j["description"] = m_description;
	if (m_overridesGroup)
	{
		j["policy"] = nlohmann::json(m_effectivePolicy);
	}
	m_itemRule->AsJSON(j);
	if (!m_scopes.empty())
	{
		nlohmann::json scopes(nlohmann::json::array());
		for (const auto scope : m_scopes)
		{
			scopes.push_back(int(scope));
		}
		j["scopes"] = scopes;
	}
	j["members"] = MembersAsJSON();
}

// rehydrate collection state from cosave data
void Collection::UpdateFrom(const nlohmann::json& collectionState, const CollectionPolicy& defaultPolicy)
{
	if (collectionState["members"].empty())
	{
		// ignore cosave, user can rehydrate from inventory etc
		REL_WARNING("Collection State {} member list empty in cosave, skipped", m_name);
		return;
	}
	const std::string name(collectionState["name"].get<std::string>());
	const std::string description(collectionState["description"].get<std::string>());
	const auto policy(collectionState.find("policy"));
	bool overridesPolicy(policy != collectionState.cend());

	m_name = name;
	m_description = description;

	REL_VMESSAGE("Collection State {} overrides Policy = {}", name, overridesPolicy ? "true" : "false");
	SetOverridesGroup(overridesPolicy);
	m_effectivePolicy = overridesPolicy ? CollectionFactory::Instance().ParsePolicy(*policy) : defaultPolicy;
	m_scopes.clear();
	const auto scopes(collectionState.find("scopes"));
	if (scopes != collectionState.cend())
	{
		SetScopesFrom(*scopes);
	}
	SetMembersFrom(collectionState["members"]);
}

void Collection::SetScopesFrom(const nlohmann::json& scopes)
{
	for (const auto scope : scopes)
	{
		m_scopes.push_back(INIFile::SecondaryType(scope.get<int>()));
	}
}

void Collection::SetMembersFrom(const nlohmann::json & members)
{
	for (const nlohmann::json& member : members)
	{
		RE::FormID formID(StringUtils::ToFormID(member["form"].get<std::string>()));
		RE::TESForm* form(LoadOrder::Instance().RehydrateCosaveForm(formID));
		if (!FormUtils::IsConcrete(form))
		{
			// sanitize malformed members - must exist, have name and be playable
			continue;
		}
		SetMemberFrom(member, form);
	}
}

void ConditionCollection::InitFromStaticMembers()
{
	// if this collection has concrete static members, add them now to seed the list
	m_members.clear();
	const auto statics(m_itemRule->StaticMembers());
	std::copy_if(statics.cbegin(), statics.cend(), std::inserter(m_members, m_members.end()), FormUtils::IsConcrete);
}

bool ConditionCollection::AddMemberID(const RE::TESForm* form)const
{
	if (form && m_members.insert(form).second)
	{
		return true;
	}
	return false;
}

void ConditionCollection::SetMemberFrom(const nlohmann::json& member, const RE::TESForm* form)
{
	m_members.insert(form);
	const auto observed(member.find("time"));
	if (observed != member.cend())
	{
		// optional, observed member only
		const float gameTime(observed->get<float>());
		m_observed.insert({ form, gameTime });
		Saga::Instance().AddEvent(ItemCollected(form, this, gameTime));
	}
}

nlohmann::json ConditionCollection::MembersAsJSON() const
{
	nlohmann::json members(nlohmann::json::array());
	for (const auto form : m_members)
	{
		nlohmann::json memberObj(nlohmann::json::object());
		memberObj["form"] = StringUtils::FromFormID(form->GetFormID());
		const auto observed(m_observed.find(form));
		if (observed != m_observed.cend())
		{
			// optional, observed member only
			memberObj["time"] = observed->second;
		}
		members.push_back(memberObj);
	}
	return members;
}

std::ostream& ConditionCollection::PrintMemberDetails(std::ostream& os) const
{
	// static members with observation status
	for (const auto member : m_members)
	{
		os << "  0x" << StringUtils::FromFormID(member->GetFormID());
		os << ", Collected? " << (m_observed.contains(member) ? 'Y' : 'N') << ", (" << member->GetName() << ")\n";
	}
	return os;
}

bool ConditionCollection::HasMembers() const
{
	// Static membership
	return !m_members.empty();
}

bool ConditionCollection::IsStaticMatch(const ConditionMatcher& matcher) const
{
	if (matcher.Form() && m_itemRule->operator()(matcher))
	{
		return AddMemberID(matcher.Form());
	}
	return false;
}

bool ConditionCollection::IsMemberOf(const ConditionMatcher& matcher) const
{
	// Check static list of IDs
	return matcher.Form() && m_members.contains(matcher.Form());
}

void CategoryCollection::InitFromStaticMembers()
{
	// no static members
}

void CategoryCollection::SetMemberFrom(const nlohmann::json& member, const RE::TESForm* form)
{
	const auto observed(member.find("time"));
	if (observed != member.cend())
	{
		// optional, observed member only
		const float gameTime(observed->get<float>());
		m_observed.insert({ form, gameTime });
		Saga::Instance().AddEvent(ItemCollected(form, this, gameTime));
	}
}

nlohmann::json CategoryCollection::MembersAsJSON() const
{
	nlohmann::json members(nlohmann::json::array());
	for (const auto observed : m_observed)
	{
		nlohmann::json memberObj(nlohmann::json::object());
		memberObj["form"] = StringUtils::FromFormID(observed.first->GetFormID());
		memberObj["time"] = observed.second;
		members.push_back(memberObj);
	}
	return members;
}

std::ostream& CategoryCollection::PrintMemberDetails(std::ostream& os) const
{
	// no static members, only those observed
	for (const auto member : m_observed)
	{
		os << "  0x" << StringUtils::FromFormID(member.first->GetFormID());
		os << ", Collected? Y, (" << member.first->GetName() << ")\n";
	}
	return os;
}

bool CategoryCollection::HasMembers() const
{
	// it is assumed that any ObjectType has at least one match
	return true;
}

bool CategoryCollection::IsStaticMatch(const ConditionMatcher&) const
{
	return false;
}

bool CategoryCollection::IsMemberOf(const ConditionMatcher& matcher) const
{
	// Base Object ObjectType changes based on game state, so cannot store member Form IDs
	return matcher.Form() && m_itemRule->operator()(matcher);
}

void to_json(nlohmann::json& j, const Collection& collection)
{
	collection.AsJSON(j);
}

CollectionGroup::CollectionGroup(const std::string& name, const CollectionPolicy& policy, const bool useMCM, const nlohmann::json& collections) :
	m_name(name), m_policy(policy), m_useMCM(useMCM)
{
	// input is JSON array, by construction
	m_collections.reserve(collections.size());
	std::for_each(collections.cbegin(), collections.cend(), [&] (const nlohmann::json& collection)
	{
		try {
			// Group Policy is the default for Group Member Collection
			auto newCollection(CollectionFactory::Instance().ParseCollection(this, collection, m_policy));
			newCollection->Reset();
			CollectionManager::Instance().RecordCollectibleObjectTypes(newCollection);
			m_collections.push_back(newCollection);
		}
		catch (const std::exception& exc) {
			REL_ERROR("Error {} parsing Collection\n{}", exc.what(), collection.dump());
		}
	});
}

std::shared_ptr<Collection> CollectionGroup::CollectionByName(const std::string& collectionName) const
{
	const auto matched(std::find_if(m_collections.cbegin(), m_collections.cend(), [&](const std::shared_ptr<Collection>& collection) -> bool
	{
		return collection->Name() == collectionName;
	}));
	return matched != m_collections.cend()  ? *matched : std::shared_ptr<Collection>();
}

void CollectionGroup::SyncDefaultPolicy()
{
	for (const auto collection : m_collections)
	{
		if (!collection->OverridesGroup())
		{
			collection->SetPolicy(m_policy);
		}
	}
}

void CollectionGroup::AsJSON(nlohmann::json& j) const
{
	j["name"] = m_name;
	j["groupPolicy"] = nlohmann::json(m_policy);
	j["useMCM"] = m_useMCM;
	j["collectionState"] = nlohmann::json::array();
	for (const auto& collection : m_collections)
	{
		j["collectionState"].push_back(*collection);
	}
}

void CollectionGroup::UpdateFrom(const nlohmann::json& group)
{
	CollectionPolicy defaultPolicy(CollectionFactory::Instance().ParsePolicy(group["groupPolicy"]));
	for (const nlohmann::json& collectionState : group["collectionState"])
	{
		std::string name(collectionState["name"].get<std::string>());
		std::shared_ptr<Collection> collection(CollectionByName(name));
		if (!collection)
		{
			// TODO try harder - maybe Load Order or the FormID in the mod changed
			REL_WARNING("Collection {} not found in Group {}", name, m_name);
			continue;
		}
		collection->UpdateFrom(collectionState, defaultPolicy);
	}
}

void to_json(nlohmann::json& j, const CollectionGroup& collectionGroup)
{
	collectionGroup.AsJSON(j);
}

}

std::ostream& operator<<(std::ostream& os, const shse::Collection& collection)
{
	os << collection.MakeJSON().dump(2);
	return os;
}
