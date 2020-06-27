#pragma once

namespace shse {

class CollectionManager {
public:
	static CollectionManager& Instance();

	CollectionManager();
	void ProcessDefinitions(void);
	std::pair<bool, SpecialObjectHandling> IsCollectible(const RE::TESForm* form);
	void Refresh() const;
	void UpdateGameTime(const float gameTime);
	void CheckEnqueueAddedItem(const RE::FormID formID);
	void ProcessAddedItems();
	inline bool IsActive() const { return m_enabled && m_ready; }
	inline bool IsAvailable() const { return m_ready; }
	void OnGameReload(void);
	void PrintDefinitions(void) const;
	void PrintMembership(void) const;
	int NumberOfFiles(void) const;
	std::string GroupNameByIndex(const int fileIndex) const;
	std::string GroupFileByIndex(const int fileIndex) const;
	int NumberOfCollections(const std::string& groupName) const;
	std::string NameByGroupIndex(const std::string& groupName, const int collectionIndex) const;
	static std::string MakeLabel(const std::string& groupName, const std::string& collectionName);
	bool PolicyRepeat(const std::string& groupName, const std::string& collectionName) const;
	bool PolicyNotify(const std::string& groupName, const std::string& collectionName) const;
	SpecialObjectHandling PolicyAction(const std::string& groupName, const std::string& collectionName) const;
	size_t TotalItems(const std::string& groupName, const std::string& collectionName) const;
	size_t ItemsObtained(const std::string& groupName, const std::string& collectionName) const;

private:
	bool LoadData(void);
	bool LoadCollectionsFromFile(
		const std::filesystem::path& defFile, const std::string& groupName, nlohmann::json_schema::json_validator& validator);
	void BuildDecisionTrees(nlohmann::json& collectionDefinitions, const std::string& groupName);
	void ResolveMembership(void);
	void AddToRelevantCollections(RE::FormID itemID);
	std::vector<RE::FormID> ReconcileInventory();
	void EnqueueAddedItem(const RE::FormID formID);

	static std::unique_ptr<CollectionManager> m_instance;
	bool m_ready;
	bool m_enabled;
	float m_gameTime;

	mutable RecursiveLock m_collectionLock;
	std::unordered_map<std::string, std::shared_ptr<Collection>> m_allCollectionsByLabel;
	std::multimap<std::string, std::string> m_collectionsByGroupName;
	std::unordered_map<std::string, std::string> m_fileNamesByGroupName;
	// Link each Form to the Collections in which it belongs
	std::unordered_multimap<RE::FormID, std::shared_ptr<Collection>> m_collectionsByFormID;
	std::unordered_set<RE::FormID> m_nonCollectionForms;

	std::vector<RE::FormID> m_addedItemQueue;
	std::unordered_set<RE::FormID> m_lastInventoryItems;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_lastInventoryCheck;
};

}
