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
#pragma once

#include "Collections/Collection.h"
#include <tuple>

namespace shse {

typedef std::tuple<RE::TESBoundObject*, const INIFile::SecondaryType, const ObjectType> OwnedItem;

class CollectionManager {
public:
	static CollectionManager& Collectibles();
	static CollectionManager& ExcessInventory();
	static bool LoadSchema(void);

	CollectionManager() = delete;
	CollectionManager(const std::wstring& filePattern);
	void ProcessDefinitions(void);
	std::pair<bool, CollectibleHandling> TreatAsCollectible(const ConditionMatcher& matcher);
	std::pair<bool, CollectibleHandling> TreatAsCollectible(const ConditionMatcher& matcher, const bool recordDups);
	void Refresh() const;
	void CollectFromContainer(const RE::TESObjectREFR* refr);
	bool ItemIsCollectionCandidate(RE::TESBoundObject* item) const;
	void CheckEnqueueAddedItem(RE::TESBoundObject* form, const INIFile::SecondaryType scope, const ObjectType objectType);
	void ProcessAddedItems();
	inline bool IsMCMEnabled() const { return SettingsCache::Instance().CollectionsEnabled(); }
	inline bool IsAvailable() const { return m_ready; }
	void Clear(void);
	void OnGameReload(void);
	void RefreshSettings(void);
	void PrintMembership(void) const;
	void RecordCollectibleObjectTypes(const std::shared_ptr<Collection>& collection);
	// these functions only apply to MCM-visible Collection Groups
	int NumberOfFiles(void) const;
	std::string GroupNameByIndex(const int fileIndex) const;
	std::string GroupFileByIndex(const int fileIndex) const;
	// end of functions for MCM-visible Collection Groups
	int NumberOfActiveCollections(const std::string& groupName) const;
	std::string NameByIndexInGroup(const std::string& groupName, const int collectionIndex) const;
	std::string DescriptionByIndexInGroup(const std::string& groupName, const int collectionIndex) const;
	static std::string MakeLabel(const std::string& groupName, const std::string& collectionName);
	const Collection* CollectionByLabel(const std::string& groupName, const std::string& collectionName) const;
	Collection* MutableCollectionByLabel(const std::string& groupName, const std::string& collectionName);

	bool PolicyRepeat(const std::string& groupName, const std::string& collectionName) const;
	bool PolicyNotify(const std::string& groupName, const std::string& collectionName) const;
	CollectibleHandling PolicyAction(const std::string& groupName, const std::string& collectionName) const;
	void PolicySetRepeat(const std::string& groupName, const std::string& collectionName, const bool allowRepeats);
	void PolicySetNotify(const std::string& groupName, const std::string& collectionName, const bool notify);
	void PolicySetAction(const std::string& groupName, const std::string& collectionName, const CollectibleHandling action);

	bool GroupPolicyRepeat(const std::string& groupName) const;
	bool GroupPolicyNotify(const std::string& groupName) const;
	CollectibleHandling GroupPolicyAction(const std::string& groupName) const;
	void GroupPolicySetRepeat(const std::string& groupName, const bool allowRepeats);
	void GroupPolicySetNotify(const std::string& groupName, const bool notify);
	void GroupPolicySetAction(const std::string& groupName, const CollectibleHandling action);

	size_t TotalItems(const std::string& groupName, const std::string& collectionName) const;
	size_t ItemsObtained(const std::string& groupName, const std::string& collectionName) const;
	std::string StatusMessage(const std::string& groupName, const std::string& collectionName) const;

	void AsJSON(nlohmann::json& j) const;
	void UpdateFrom(const nlohmann::json& j);

private:
	bool LoadData(void);
	void LoadCollectionFiles(const std::wstring& pattern, nlohmann::json_schema::json_validator& validator);
	bool LoadCollectionGroup(
		const std::filesystem::path& defFile, const std::string& groupName, nlohmann::json_schema::json_validator& validator);
	void BuildDecisionTrees(const std::shared_ptr<CollectionGroup>& collectionGroup);
	void RecordCollectibleForm(const std::shared_ptr<Collection>& collection, const RE::TESForm* form,
		std::unordered_set<const RE::TESForm*>& uniqueMembers);
	void ResolveMembership(void);
	void AddToRelevantCollections(const ConditionMatcher& matcher, const float gameTime);
	void ReconcileInventory(std::vector<OwnedItem>& additions);
	void EnqueueAddedItem(RE::TESBoundObject*, const INIFile::SecondaryType scope, const ObjectType objectType);

	static constexpr size_t CollectedSpamLimit = 10;
	size_t m_notifications;

	static std::unique_ptr<CollectionManager> m_collectibles;
	static std::unique_ptr<CollectionManager> m_excessInventory;
	static nlohmann::json_schema::json_validator m_validator;

	// data loaded ok?
	bool m_ready;

	mutable RecursiveLock m_collectionLock;
	std::unordered_map<std::string, std::shared_ptr<Collection>> m_allCollectionsByLabel;
	mutable std::multimap<std::string, std::string> m_activeCollectionsByGroupName;
	std::unordered_map<std::string, std::string> m_mcmVisibleFileByGroupName;
	std::unordered_map<std::string, std::shared_ptr<CollectionGroup>> m_allGroupsByName;
	// Link each Form to the Collections in which it belongs
	std::unordered_multimap<RE::FormID, std::shared_ptr<Collection>> m_collectionsByFormID;
	std::unordered_multimap<ObjectType, std::shared_ptr<Collection>> m_collectionsByObjectType;

	std::vector<OwnedItem> m_addedItemQueue;
	std::unordered_set<const RE::TESForm*> m_lastInventoryCollectibles;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_lastInventoryCheck;
	std::unordered_set<RE::FormID> m_collectedOnThisScan;
	const std::wstring m_filePattern;
};

void to_json(nlohmann::json& j, const CollectionManager& collectionManager);

}
