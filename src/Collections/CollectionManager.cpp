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

#include <filesystem>
#include <regex>

#include "Utilities/LogStackWalker.h"
#include "Utilities/Exception.h"
#include "Utilities/utils.h"
#include "WorldState/LocationTracker.h"
#include "VM/EventPublisher.h"
#include "VM/papyrus.h"
#include "Collections/CollectionManager.h"
#include "Collections/CollectionFactory.h"
#include "Data/CosaveData.h"
#include "Data/DataCase.h"
#include "Data/SettingsCache.h"
#include "Data/LoadOrder.h"
#include "Looting/ManagedLists.h"
#include "Looting/objects.h"

namespace shse
{

std::unique_ptr<CollectionManager> CollectionManager::m_collectibles;
std::unique_ptr<CollectionManager> CollectionManager::m_excessInventory;
nlohmann::json_schema::json_validator CollectionManager::m_validator;

CollectionManager& CollectionManager::Collectibles()
{
	if (!m_collectibles)
	{
		m_collectibles = std::make_unique<CollectionManager>(L"SHSE\\.Collections\\.(.*)\\.json$");
	}
	return *m_collectibles;
}

CollectionManager& CollectionManager::ExcessInventory()
{
	if (!m_excessInventory)
	{
		m_excessInventory = std::make_unique<CollectionManager>(L"SHSE\\.ExcessInventory\\.(.*)\\.json$");
	}
	return *m_excessInventory;
}

CollectionManager::CollectionManager(const std::wstring& filePattern) :
m_notifications(0), m_ready(false), m_filePattern(filePattern)
{
}

// Generate Collection Definitions from JSON Config
void CollectionManager::ProcessDefinitions(void)
{
	// call only once
	if (IsAvailable())
		return;

	__try {
		if (!LoadData())
			return;

		// data validated and loaded
		m_ready = true;
	}
	__except (LogStackWalker::LogStack(GetExceptionInformation())) {
		REL_FATALERROR("JSON Collection Definitions threw structured exception");
	}
}

void CollectionManager::Refresh() const
{
	// request added items and game time to be pushed to us while we are sleeping
	EventPublisher::Instance().TriggerFlushAddedItems();
}

bool CollectionManager::ItemIsCollectionCandidate(RE::TESBoundObject* item) const
{
	RecursiveLockGuard guard(m_collectionLock);
	return m_collectionsByFormID.contains(item->GetFormID()) || m_collectionsByObjectType.contains(GetEffectiveObjectType(item));
}

void CollectionManager::CollectFromContainer(const RE::TESObjectREFR* refr)
{
	// Container trait was checked in the script that invokes this via SHSE_PluginProxy but we must make sure here
	if (!IsAvailable() || !refr->GetContainer())
		return;
	RecursiveLockGuard guard(m_collectionLock);
	DBG_VMESSAGE("Check REFR 0x{:08x} to Container {}/0x{:08x} for Collectibles",
		refr->GetFormID(), refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID());
	ContainerLister lister(INIFile::SecondaryType::deadbodies, refr);
	// filter to include only collectibles
	lister.FilterLootableItems([=](RE::TESBoundObject* item) -> bool { return ItemIsCollectionCandidate(item); });
	decltype(m_lastInventoryCollectibles) newInventoryCollectibles;
	for (const auto& candidate : lister.GetLootableItems())
	{
		// assume loose item, we don't know the original source
		EnqueueAddedItem(candidate.BoundObject(), INIFile::SecondaryType::itemObjects, GetEffectiveObjectType(candidate.BoundObject()));
		DBG_VMESSAGE("Enqueue Collectible {}/0x{:08x} from Container", candidate.BoundObject()->GetName(), candidate.BoundObject()->GetFormID());
	}
	static RE::BSFixedString extraItemsText(papyrus::GetTranslation(nullptr, RE::BSFixedString("$SHSE_HOTKEY_ADD_CONTENTS_TO_COLLECTIONS")));
	if (!extraItemsText.empty())
	{
		std::string notificationText(extraItemsText);
		RE::DebugNotification(notificationText.c_str());
	}
}

void CollectionManager::CheckEnqueueAddedItem(RE::TESBoundObject* form, const INIFile::SecondaryType scope, const ObjectType objectType)
{
	if (!IsAvailable())
		return;
	RecursiveLockGuard guard(m_collectionLock);
	// only pass this along if it is in >= 1 collection
	if (ItemIsCollectionCandidate(form))
	{
		EnqueueAddedItem(form, scope, objectType);
	}
}

void CollectionManager::EnqueueAddedItem(RE::TESBoundObject* form, const INIFile::SecondaryType scope, const ObjectType objectType)
{
	m_addedItemQueue.emplace_back(std::make_tuple(form, scope, objectType));
}

void CollectionManager::ProcessAddedItems()
{
	if (!IsAvailable())
		return;

#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Collection checks");
#endif
	RecursiveLockGuard guard(m_collectionLock);
	constexpr std::chrono::milliseconds InventoryReconciliationIntervalMillis(5000LL);
	const auto nowTime(std::chrono::high_resolution_clock::now());
	std::vector<OwnedItem> queuedItems;
	queuedItems.swap(m_addedItemQueue);
	if (nowTime - m_lastInventoryCheck >= InventoryReconciliationIntervalMillis)
	{
		DBG_MESSAGE("Inventory reconciliation required");
		m_lastInventoryCheck = nowTime;
		ReconcileInventory(queuedItems);
	}

	const float gameTime(PlayerState::Instance().CurrentGameTime());
	m_notifications = 0;
	for (const auto ownedItem : queuedItems)
	{
		// only process items known to be a member of at least one collection
		RE::TESBoundObject* form(std::get<0>(ownedItem));
		if (ItemIsCollectionCandidate(form))
		{
			DBG_VMESSAGE("Check collectability of added item 0x{:08x}", form->GetFormID());
			AddToRelevantCollections(ConditionMatcher(form, std::get<1>(ownedItem), std::get<2>(ownedItem)), gameTime);
		}
	}

	// send generic 'there were more items' message if spam filter triggered
	if (m_notifications > CollectedSpamLimit)
	{
		size_t extraItems(m_notifications - CollectedSpamLimit);
		static RE::BSFixedString extraItemsText(papyrus::GetTranslation(nullptr, RE::BSFixedString("$SHSE_ADDED_TO_COLLECTION_EXTRAS")));
		if (!extraItemsText.empty())
		{
			std::string notificationText(extraItemsText);
			StringUtils::Replace(notificationText, "{COUNT}", std::to_string(extraItems));
			if (!notificationText.empty())
			{
				RE::DebugNotification(notificationText.c_str());
			}
		}
	}
	// reset list of collected items prior to next scan
	m_collectedOnThisScan.clear();
}

// bucket newly-received items in any matching collections
void CollectionManager::AddToRelevantCollections(const ConditionMatcher& matcher, const float gameTime)
{
	// resolve ID to Form
	if (!matcher.Form())
		return;
	RecursiveLockGuard guard(m_collectionLock);
	const auto staticTargets(m_collectionsByFormID.equal_range(matcher.Form()->GetFormID()));
	bool atLeastOne(false);
	auto checkCollection = [&](decltype(staticTargets.first->second) nextCollection) {
		// skip disabled collections
		if (!nextCollection->IsActive())
			return;
		// Do not record if the policy indicates per-item history not required, or not a member, or if observed and not repeatable
		if (CollectibleHistoryNeeded(nextCollection->Policy().Action()) &&
			(nextCollection->Policy().Repeat() || !nextCollection->HaveObserved(matcher.Form())) &&
			nextCollection->IsMemberOf(matcher))
		{
			// record membership - suppress notifications if there are too many
			if (nextCollection->RecordItem(matcher.Form(), gameTime, m_notifications >= CollectedSpamLimit))
			{
				// another notification required
				++m_notifications;
			}
			atLeastOne = true;
		}
	};
	for (auto formCollection = staticTargets.first; formCollection != staticTargets.second; ++formCollection)
	{
		checkCollection(formCollection->second);
	}
	const auto dynamicTargets(m_collectionsByObjectType.equal_range(matcher.GetObjectType()));
	for (auto formCollection = dynamicTargets.first; formCollection != dynamicTargets.second; ++formCollection)
	{
		checkCollection(formCollection->second);
	}
	if (atLeastOne)
	{
		// Ensure Location of Item Collection is recorded
		LocationTracker::Instance().RecordCurrentPlace(gameTime);
	}
}

std::pair<bool, CollectibleHandling> CollectionManager::TreatAsCollectible(const ConditionMatcher& matcher)
{
	constexpr bool recordDups(true);
	return TreatAsCollectible(matcher, recordDups);
}

std::pair<bool, CollectibleHandling> CollectionManager::TreatAsCollectible(const ConditionMatcher& matcher, const bool recordDups)
{
	if (!IsAvailable() || !matcher.Form())
		return NotCollectible;
	RecursiveLockGuard guard(m_collectionLock);
	// Filter for Static and Dynamic Collections that match this Form
	// Find the most aggressive action for any where we are in scope and a usable member.
	CollectibleHandling action(CollectibleHandling::Leave);
	bool actionable(false);
	bool defer(false);
	RE::FormID formID(matcher.Form()->GetFormID());
	const auto staticTargets(m_collectionsByFormID.equal_range(formID));
	auto checkCollection = [&](decltype(staticTargets.first->second) nextCollection) {
		// skip disabled collections
		if (!nextCollection->IsActive())
			return;
		bool inScope;
		bool repeat;
		bool observed;
		std::tie(inScope, repeat, observed) = nextCollection->InScopeAndCollectibleFor(matcher);
		// skip if item out of scope for this collection
		if (!inScope)
			return;

		// If we already saw this item on this scan, this is a repeat observation. m_observed is not updated
		// until Collection add, which happens later on after item is in inventory.
		observed = observed || m_collectedOnThisScan.contains(formID);
		if (repeat || !observed)
		{
			// The Collection definitively treats this observation of the item as a member
			actionable = true;
			action = UpdateCollectibleHandling(nextCollection->Policy().Action(), action);
			DBG_VMESSAGE("Item {}/0x{:08x} in Collection {} with action {}",
				matcher.Form()->GetName(), matcher.Form()->GetFormID(), nextCollection->Name(), CollectibleHandlingString(action));
		}
		else if (!repeat && !actionable)
		{
			// The Collection decision is qualified: it's a member, but not collected because repeats are not allowed
			DBG_VMESSAGE("Item {}/0x{:08x} in Collection {} with action deferred",
				matcher.Form()->GetName(), matcher.Form()->GetFormID(), nextCollection->Name());
			defer = true;
		}
	};
	for (auto formCollection = staticTargets.first; formCollection != staticTargets.second; ++formCollection)
	{
		checkCollection(formCollection->second);
	}
	const auto dynamicTargets(m_collectionsByObjectType.equal_range(matcher.GetObjectType()));
	for (auto formCollection = dynamicTargets.first; formCollection != dynamicTargets.second; ++formCollection)
	{
		checkCollection(formCollection->second);
	}
	if (defer && !actionable)
	{
		return NotCollectible;
	}
	// prevent dups - if this is an item Collectibility precheck don't trigger this logic, or we won't actually loot the item
	if (recordDups && actionable)
	{
		m_collectedOnThisScan.insert(formID);
	}
	return std::make_pair(actionable, action);
}

// Player inventory can get objects from Loot menus and other sources than our harvesting, we need to account for them
// We don't do this on every pass as it's a decent amount of work. Filter on Collectible item trait to reduce work.
void CollectionManager::ReconcileInventory(std::vector<OwnedItem>& additions)
{
	RE::PlayerCharacter* player(RE::PlayerCharacter::GetSingleton());
	if (!player)
		return;

	// use delta vs last pass to speed this up (resets on game reload)
	ContainerLister lister(INIFile::SecondaryType::deadbodies, player);
	// filter to include only collectibles
	lister.FilterLootableItems([=](RE::TESBoundObject* item) -> bool { return ItemIsCollectionCandidate(item); });
	decltype(m_lastInventoryCollectibles) newInventoryCollectibles;
	for (const auto& candidate : lister.GetLootableItems())
	{
		const auto item(candidate.BoundObject());
		newInventoryCollectibles.insert(item);
		if (!m_lastInventoryCollectibles.contains(item))
		{
			DBG_VMESSAGE("Collectible {}/0x{:08x} new in inventory", item->GetName(), item->GetFormID());
			// Assumes loose item since we apparently did not autoloot it
			additions.emplace_back(std::make_tuple(item, INIFile::SecondaryType::itemObjects, GetEffectiveObjectType(item)));
		}
		else
		{
			DBG_VMESSAGE("Skip {}/0x{:08x} unchanged in inventory", item->GetName(), item->GetFormID());
		}
	}
	m_lastInventoryCollectibles.swap(newInventoryCollectibles);
}

bool CollectionManager::LoadCollectionGroup(
	const std::filesystem::path& defFile, const std::string& groupName, nlohmann::json_schema::json_validator& validator)
{
	try {
		std::ifstream collectionFile(defFile);
		if (collectionFile.fail()) {
			throw FileNotFound(defFile.generic_wstring().c_str());
		}
		nlohmann::json collectionGroupData(nlohmann::json::parse(collectionFile));
		validator.validate(collectionGroupData);
		const auto collectionGroup(CollectionFactory::Instance().ParseGroup(*this, collectionGroupData, groupName));
		BuildDecisionTrees(collectionGroup);
		if (collectionGroup->UseMCM())
		{
			m_mcmVisibleFileByGroupName.insert(std::make_pair(groupName, defFile.string()));
		}
		m_allGroupsByName.insert(std::make_pair(groupName, collectionGroup));
		return true;
	}
	catch (const std::exception& e) {
		REL_ERROR("JSON Collection Definitions {} not loadable, error:\n{}", StringUtils::FromUnicode(defFile.generic_wstring()), e.what());
		return false;
	}
}

void CollectionManager::LoadCollectionFiles(const std::wstring& pattern, nlohmann::json_schema::json_validator& validator)
{
	const std::wregex collectionsFilePattern(pattern);
	for (const auto& nextFile : std::filesystem::directory_iterator(FileUtils::GetPluginPath()))
	{
		if (!std::filesystem::is_regular_file(nextFile))
		{
			DBG_MESSAGE("Skip {}, not a regular file", StringUtils::FromUnicode(nextFile.path().generic_wstring()));
			continue;
		}
		std::wstring fileName(nextFile.path().filename().generic_wstring());
		std::wsmatch matches;
		if (!std::regex_search(fileName, matches, collectionsFilePattern))
		{
			DBG_MESSAGE("Skip {}, does not match Collections filename pattern", StringUtils::FromUnicode(fileName));
			continue;
		}
		// capture string at index 1 is the Collection Name, always present after a regex match
		REL_MESSAGE("Load JSON Collection Definitions {} for Group {}", StringUtils::FromUnicode(fileName), StringUtils::FromUnicode(matches[1].str()));
		if (LoadCollectionGroup(nextFile, StringUtils::FromUnicode(matches[1].str()), validator))
		{
			REL_MESSAGE("JSON Collection Definitions {}/{} parsed and validated", StringUtils::FromUnicode(fileName), StringUtils::FromUnicode(matches[1].str()));
		}
	}
}

[[nodiscard]] bool CollectionManager::LoadSchema(void)
{
	// Validate the schema
	const std::string schemaFileName("SHSE.SchemaCollections.json");
	std::string filePath(StringUtils::FromUnicode(FileUtils::GetPluginPath()) + schemaFileName);
	try {
		std::ifstream schemaFile(filePath);
		if (schemaFile.fail()) {
			throw FileNotFound(filePath.c_str());
		}
		nlohmann::json schema(nlohmann::json::parse(schemaFile));
		m_validator.set_root_schema(schema); // insert root-schema
	}
	catch (const std::exception& e) {
		REL_ERROR("JSON Schema {} not loadable, error:\n{}", filePath.c_str(), e.what());
		return false;
	}

	REL_MESSAGE("JSON Schema {} parsed and validated", filePath.c_str());
	return true;
}

bool CollectionManager::LoadData(void)
{
	try {
		// Find and Load Collection Definitions using the validated schema
		LoadCollectionFiles(m_filePattern, m_validator);
	} catch (const std::exception& e) {
		REL_ERROR("Collection Definitions not loadable: is the file target at {} inside 'Program Files' or another elevated-privilege location? Error:\n{}",
			StringUtils::FromUnicode(FileUtils::GetPluginPath()), e.what());
		return false;
	}
	return true;
}

void CollectionManager::PrintMembership(void) const
{
	for (const auto& collectionGroup : m_allGroupsByName)
	{
		REL_MESSAGE("* Collection Group {}:", collectionGroup.second->Name());
		for (const auto& collection : collectionGroup.second->Collections())
		{
			REL_MESSAGE("** Collection {}:\n{}", collection->Name(), collection->PrintDefinition());
			if (collection->HasMembers())
			{
				REL_MESSAGE("{}", collection->PrintMembers());
				const std::string label(MakeLabel(collectionGroup.second->Name(), collection->Name()));
				m_activeCollectionsByGroupName.insert(std::make_pair(collectionGroup.second->Name(), label));
			}
			else
			{
				REL_MESSAGE("[No Members]");
			}
		}
	}
}

int CollectionManager::NumberOfFiles(void) const
{
	RecursiveLockGuard guard(m_collectionLock);
	return static_cast<int>(m_mcmVisibleFileByGroupName.size());
}

std::string CollectionManager::GroupNameByIndex(const int fileIndex) const
{
	RecursiveLockGuard guard(m_collectionLock);
	int index(0);
	for (const auto& group : m_mcmVisibleFileByGroupName)
	{
		if (index == fileIndex)
			return group.first;
		++index;
	}
	return std::string();
}

std::string CollectionManager::GroupFileByIndex(const int fileIndex) const
{
	RecursiveLockGuard guard(m_collectionLock);
	int index(0);
	for (const auto& group : m_mcmVisibleFileByGroupName)
	{
		if (index == fileIndex)
			return group.second;
		++index;
	}
	return std::string();
}

int CollectionManager::NumberOfActiveCollections(const std::string& groupName) const
{
	RecursiveLockGuard guard(m_collectionLock);
	return static_cast<int>(m_activeCollectionsByGroupName.count(groupName));
}

std::string CollectionManager::NameByIndexInGroup(const std::string& groupName, const int collectionIndex) const
{
	RecursiveLockGuard guard(m_collectionLock);
	const auto matches(m_activeCollectionsByGroupName.equal_range(groupName));
	int index(0);
	for (auto group = matches.first; group != matches.second; ++group)
	{
		if (index == collectionIndex)
		{
			// strip the group name and separator
			return group->second.substr(groupName.length() + 1);
		}
		++index;
	}
	return std::string();
}

std::string CollectionManager::DescriptionByIndexInGroup(const std::string& groupName, const int collectionIndex) const
{
	std::string collectionName(NameByIndexInGroup(groupName, collectionIndex));
	const auto matchedGroup(m_allGroupsByName.find(groupName));
	if (matchedGroup != m_allGroupsByName.cend() && !collectionName.empty())
	{
		const auto collection(matchedGroup->second->CollectionByName(collectionName));
		if (collection)
		{
			return collection->Description();
		}
	}
	return std::string();
}

std::string CollectionManager::MakeLabel(const std::string& groupName, const std::string& collectionName)
{
	std::ostringstream labelStream;
	labelStream << groupName << '/' << collectionName;
	return labelStream.str();
}

const Collection* CollectionManager::CollectionByLabel(const std::string& groupName, const std::string& collectionName) const
{
	const std::string label(MakeLabel(groupName, collectionName));
	RecursiveLockGuard guard(m_collectionLock);
	const auto matched(m_allCollectionsByLabel.find(label));
	if (matched != m_allCollectionsByLabel.cend())
	{
		return matched->second.get();
	}
	return nullptr;
}


bool CollectionManager::PolicyRepeat(const std::string& groupName, const std::string& collectionName) const
{
	const std::string label(MakeLabel(groupName, collectionName));
	RecursiveLockGuard guard(m_collectionLock);
	const auto matched(m_allCollectionsByLabel.find(label));
	if (matched != m_allCollectionsByLabel.cend())
	{
		return matched->second->Policy().Repeat();
	}
	return false;
}

bool CollectionManager::PolicyNotify(const std::string& groupName, const std::string& collectionName) const
{
	const std::string label(MakeLabel(groupName, collectionName));
	RecursiveLockGuard guard(m_collectionLock);
	const auto matched(m_allCollectionsByLabel.find(label));
	if (matched != m_allCollectionsByLabel.cend())
	{
		return matched->second->Policy().Notify();
	}
	return false;
}

CollectibleHandling CollectionManager::PolicyAction(const std::string& groupName, const std::string& collectionName) const
{
	const std::string label(MakeLabel(groupName, collectionName));
	RecursiveLockGuard guard(m_collectionLock);
	const auto matched(m_allCollectionsByLabel.find(label));
	if (matched != m_allCollectionsByLabel.cend())
	{
		return matched->second->Policy().Action();
	}
	return CollectibleHandling::Leave;
}

void CollectionManager::PolicySetRepeat(const std::string& groupName, const std::string& collectionName, const bool allowRepeats)
{
	const std::string label(MakeLabel(groupName, collectionName));
	RecursiveLockGuard guard(m_collectionLock);
	auto matched(m_allCollectionsByLabel.find(label));
	if (matched != m_allCollectionsByLabel.cend())
	{
		matched->second->Policy().SetRepeat(allowRepeats);
		matched->second->SetOverridesGroup(true);
	}
}

void CollectionManager::PolicySetNotify(const std::string& groupName, const std::string& collectionName, const bool notify)
{
	const std::string label(MakeLabel(groupName, collectionName));
	RecursiveLockGuard guard(m_collectionLock);
	auto matched(m_allCollectionsByLabel.find(label));
	if (matched != m_allCollectionsByLabel.cend())
	{
		matched->second->Policy().SetNotify(notify);
		matched->second->SetOverridesGroup(true);
	}
}

void CollectionManager::PolicySetAction(const std::string& groupName, const std::string& collectionName, const CollectibleHandling action)
{
	const std::string label(MakeLabel(groupName, collectionName));
	RecursiveLockGuard guard(m_collectionLock);
	auto matched(m_allCollectionsByLabel.find(label));
	if (matched != m_allCollectionsByLabel.cend())
	{
		matched->second->Policy().SetAction(action);
		matched->second->SetOverridesGroup(true);
	}
}

bool CollectionManager::GroupPolicyRepeat(const std::string& groupName) const
{
	RecursiveLockGuard guard(m_collectionLock);
	const auto matched(m_allGroupsByName.find(groupName));
	if (matched != m_allGroupsByName.cend())
	{
		return matched->second->Policy().Repeat();
	}
	return false;
}

bool CollectionManager::GroupPolicyNotify(const std::string& groupName) const
{
	RecursiveLockGuard guard(m_collectionLock);
	const auto matched(m_allGroupsByName.find(groupName));
	if (matched != m_allGroupsByName.cend())
	{
		return matched->second->Policy().Notify();
	}
	return false;
}

CollectibleHandling CollectionManager::GroupPolicyAction(const std::string& groupName) const
{
	RecursiveLockGuard guard(m_collectionLock);
	const auto matched(m_allGroupsByName.find(groupName));
	if (matched != m_allGroupsByName.cend())
	{
		return matched->second->Policy().Action();
	}
	return CollectibleHandling::Leave;
}

void CollectionManager::GroupPolicySetRepeat(const std::string& groupName, const bool allowRepeats)
{
	RecursiveLockGuard guard(m_collectionLock);
	const auto matched(m_allGroupsByName.find(groupName));
	if (matched != m_allGroupsByName.cend())
	{
		matched->second->Policy().SetRepeat(allowRepeats);
		matched->second->SyncDefaultPolicy();
	}
}

void CollectionManager::GroupPolicySetNotify(const std::string& groupName, const bool notify)
{
	RecursiveLockGuard guard(m_collectionLock);
	const auto matched(m_allGroupsByName.find(groupName));
	if (matched != m_allGroupsByName.cend())
	{
		matched->second->Policy().SetNotify(notify);
		matched->second->SyncDefaultPolicy();
	}
}

void CollectionManager::GroupPolicySetAction(const std::string& groupName, const CollectibleHandling action)
{
	RecursiveLockGuard guard(m_collectionLock);
	const auto matched(m_allGroupsByName.find(groupName));
	if (matched != m_allGroupsByName.cend())
	{
		matched->second->Policy().SetAction(action);
		matched->second->SyncDefaultPolicy();
	}
}

size_t CollectionManager::TotalItems(const std::string& groupName, const std::string& collectionName) const
{
	const std::string label(MakeLabel(groupName, collectionName));
	RecursiveLockGuard guard(m_collectionLock);
	const auto matched(m_allCollectionsByLabel.find(label));
	if (matched != m_allCollectionsByLabel.cend())
	{
		return matched->second->Count();
	}
	return 0;
}

size_t CollectionManager::ItemsObtained(const std::string& groupName, const std::string& collectionName) const
{
	const std::string label(MakeLabel(groupName, collectionName));
	RecursiveLockGuard guard(m_collectionLock);
	const auto matched(m_allCollectionsByLabel.find(label));
	if (matched != m_allCollectionsByLabel.cend())
	{
		return matched->second->Observed();
	}
	return 0;
}

std::string CollectionManager::StatusMessage(const std::string& groupName, const std::string& collectionName) const
{
	const std::string label(MakeLabel(groupName, collectionName));
	RecursiveLockGuard guard(m_collectionLock);
	const auto matched(m_allCollectionsByLabel.find(label));
	if (matched != m_allCollectionsByLabel.cend())
	{
		return matched->second->GetStatusMessage();
	}
	return "";
}

void CollectionManager::BuildDecisionTrees(const std::shared_ptr<CollectionGroup>& collectionGroup)
{
	for (const auto collection : collectionGroup->Collections())
	{
		const std::string label(MakeLabel(collectionGroup->Name(), collection->Name()));
		if (m_allCollectionsByLabel.insert(std::make_pair(label, collection)).second)
		{
			REL_MESSAGE("Parse OK for Collection {}", label.c_str());
		}
		else
		{
			REL_WARNING("Discarded duplicate Collection {}", label.c_str());
		}
	}
}

void CollectionManager::RecordCollectibleForm(const std::shared_ptr<Collection>& collection, const RE::TESForm* form,
	std::unordered_set<const RE::TESForm*>& uniqueMembers)
{
	DBG_VMESSAGE("Record {}/0x{:08x} as collectible", form->GetName(), form->GetFormID());
	m_collectionsByFormID.insert(std::make_pair(form->GetFormID(), collection));
	uniqueMembers.insert(form);
}

void CollectionManager::RecordCollectibleObjectTypes(const std::shared_ptr<Collection>& collection)
{
	for (const auto objectType : collection->ObjectTypes())
	{
		m_collectionsByObjectType.insert(std::make_pair(objectType, collection));
		DBG_VMESSAGE("Recorded {} as collectible ObjectType", GetObjectTypeName(objectType));
	}
}

void CollectionManager::ResolveMembership(void)
{
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Resolve Collection Membership");
#endif
	std::unordered_set<const RE::TESForm*> uniqueMembers;
	// record static members before resolving
	for (const auto& collection : m_allCollectionsByLabel)
	{
		for (const auto member : collection.second->Members())
		{
			RecordCollectibleForm(collection.second, member, uniqueMembers);
		}
	}

	auto processFormType = [&](const RE::FormType formType) {
		for (const auto form : RE::TESDataHandler::GetSingleton()->GetFormArray(formType))
		{
			if (!FormUtils::IsConcrete(form))
				continue;
			if (FormIsLeveledNPC(form))
				continue;

			for (const auto& collection : m_allCollectionsByLabel)
			{
				// Record collection membership for any that match this object - ignore whitelist
				// This resolution step does not incorporate Collections with CategoryRule as the
				// Form's ObjectType cannot be reliably determined during startup
				ConditionMatcher matcher(form);
				if (collection.second->IsStaticMatch(matcher))
				{
					// Any condition on this collection that has a scope has aggregated the valid scopes in the matcher
					collection.second->SetScopes(matcher.ScopesSeen());
					RecordCollectibleForm(collection.second, form, uniqueMembers);
				}
			}
		}
	};
	for (const auto& signature : SignatureCondition::ValidSignatures())
	{
		processFormType(signature.second);
	}
	// named objects processed in DataCase::CategorizeLootables or otherwise Lootable, but not valid in SignatureCondition
	std::vector<RE::FormType> extraFormTypes = {
		RE::FormType::Activator,
		RE::FormType::Ammo,
		RE::FormType::Flora,
		RE::FormType::Light,
		RE::FormType::NPC,
		RE::FormType::Projectile,
		RE::FormType::Scroll,
		RE::FormType::Tree,
	};
	for (const auto& extraFormType : extraFormTypes)
	{
		processFormType(extraFormType);
	}
	REL_MESSAGE("Collections contain {} unique objects", uniqueMembers.size());
}

// clear state before game reload, including reset of item state
void CollectionManager::Clear(void)
{
	REL_MESSAGE("Reset Collections");
	// m_collectionsByObjectType is not cleared - it is based on one-time load of JSON files and therefore invariant
	for (auto collection : m_allCollectionsByLabel)
	{
		collection.second->Reset();
	}

	// reset player inventory last-known-good
	m_lastInventoryCollectibles.clear();
	m_lastInventoryCheck = decltype(m_lastInventoryCheck)();
	m_addedItemQueue.clear();
}

// if this is a game reload, we must resolve membership after cosave data is applied
void CollectionManager::OnGameReload(void)
{
	RecursiveLockGuard guard(m_collectionLock);
	// Flush membership state to allow testing
	m_collectionsByFormID.clear();
	m_activeCollectionsByGroupName.clear();

	// resolve and print effective membership - no members for new game, cosave state may include members so print reconciled version
	ResolveMembership();
	PrintMembership();
}


void CollectionManager::AsJSON(nlohmann::json& j) const
{
	RecursiveLockGuard guard(m_collectionLock);
	j["groups"] = nlohmann::json::array();
	for (const auto& collectionGroup : m_allGroupsByName)
	{
		j["groups"].push_back(*collectionGroup.second);
	}
}

// reset Collection state from cosave data
void CollectionManager::UpdateFrom(const nlohmann::json& j)
{
	REL_MESSAGE("Cosave Collections\n{}", j.dump(2));
	RecursiveLockGuard guard(m_collectionLock);
	for (const nlohmann::json& group : j["groups"])
	{
		std::string groupName(group["name"].get<std::string>());
		auto existing(m_allGroupsByName.find(groupName));
		if (existing == m_allGroupsByName.cend())
		{
			REL_WARNING("Cosave contains unknown Collection Group {}", groupName);
			// TODO keep the data around? But what use will it be?
			continue;
		}
		existing->second->UpdateFrom(group);
	}
}

void to_json(nlohmann::json& j, const CollectionManager& collectionManager)
{
	collectionManager.AsJSON(j);
}

}
