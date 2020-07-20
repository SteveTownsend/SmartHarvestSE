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
#include "Utilities/utils.h"
#include "VM/papyrus.h"
#include "WorldState/PlayerState.h"

namespace shse
{

void CollectionPolicy::AsJSON(nlohmann::json& j) const
{
	j["action"] = CollectibleHandlingJSON(m_action);
	j["notify"] = m_notify;
	j["repeat"] = m_repeat;
}

void to_json(nlohmann::json& j, const CollectionPolicy& policy)
{
	policy.AsJSON(j);
}

Collection::Collection(const std::string& name, const std::string& description, const CollectionPolicy& policy,
	const bool overridesGroup, std::unique_ptr<ConditionTree> filter) :
	m_name(name), m_description(description), m_effectivePolicy(policy), m_overridesGroup(overridesGroup), m_rootFilter(std::move(filter))
{
	// if this collection has static members, add them now to seed the list
	m_members = m_rootFilter->StaticMembers();
}

bool Collection::AddMemberID(const RE::TESForm* form)const 
{
	if (form && m_members.insert(form).second)
	{
		return true;
	}
	return false;
}

bool Collection::IsMemberOf(const RE::TESForm* form) const
{
	// Check static list of IDs
	return form && m_members.contains(form);
}

bool Collection::InScopeAndCollectibleFor(const ConditionMatcher& matcher) const
{
	if (!matcher.Form())
		return false;

	// check Scope - if Collection is scoped, scope for this autoloot check must be valid
	if (!m_scopes.empty() && std::find(m_scopes.cbegin(), m_scopes.cend(), matcher.Scope()) == m_scopes.cend())
	{
		DBG_VMESSAGE("%s/0x%08x has invalid scope %d", matcher.Form()->GetName(), matcher.Form()->GetFormID(), int(matcher.Scope()));
		return false;
	}

	// if (always collectible OR not observed) AND a member of this collection
	return (m_effectivePolicy.Repeat() || !m_observed.contains(matcher.Form()->GetFormID())) && IsMemberOf(matcher.Form());
}

bool Collection::MatchesFilter(const ConditionMatcher& matcher) const
{
	if (matcher.Form() && m_rootFilter->operator()(matcher))
	{
		AddMemberID(matcher.Form());
		return true;
	}
	return false;
}

void Collection::RecordItem(const RE::FormID itemID, const RE::TESForm* form, const float gameTime, const RE::TESForm* place)
{
	DBG_VMESSAGE("Collect %s/0x%08x in %s", form->GetName(), form->GetFormID(), m_name.c_str());
	if (m_observed.insert(
		std::make_pair(itemID, CollectionEntry(form, gameTime, place, PlayerState::Instance().GetPosition()))).second)
	{
		if (m_effectivePolicy.Notify())
		{
			// notify about these, just once
			std::string notificationText;
			static RE::BSFixedString newMemberText(papyrus::GetTranslation(nullptr, RE::BSFixedString("$SHSE_ADDED_TO_COLLECTION")));
			if (!newMemberText.empty())
			{
				notificationText = newMemberText;
				StringUtils::Replace(notificationText, "{ITEMNAME}", form->GetName());
				StringUtils::Replace(notificationText, "{COLLECTION}", m_name);
				if (!notificationText.empty())
				{
					RE::DebugNotification(notificationText.c_str());
				}
			}
		}
	}
}

void Collection::Reset()
{
	m_observed.clear();
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


size_t Collection::PlacedMembers(void) const
{
	return std::count_if(m_members.cbegin(), m_members.cend(),
		[&](const RE::TESForm* form) -> bool { return CollectionManager::Instance().IsPlacedObject(form); });
}

std::string Collection::PrintMembers(void) const
{
	std::ostringstream collectionStr;
	collectionStr << m_members.size() << " members of which " << PlacedMembers() << " are placed in the world\n";
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
	for (const auto member : m_members)
	{
		collectionStr << "  0x" << std::hex << std::setw(8) << std::setfill('0') << member->GetFormID();
		collectionStr << ":" << (CollectionManager::Instance().IsPlacedObject(member) ? 'Y' : 'N') << ":" << member->GetName();
		collectionStr << '\n';
	}
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
	j["policy"] = nlohmann::json(m_effectivePolicy);
	j["rootFilter"] = nlohmann::json(*m_rootFilter);
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
			m_collections.push_back(CollectionFactory::Instance().ParseCollection(collection, m_policy));
		}
		catch (const std::exception& exc) {
			REL_ERROR("Error %s parsing Collection\n%s", exc.what(), collection.dump(2).c_str());
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
	j["groupPolicy"] = nlohmann::json(m_policy);
	j["useMCM"] = m_useMCM;
	j["collections"] = nlohmann::json::array();
	for (const auto& collection : m_collections)
	{
		j["collections"].push_back(*collection);
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
