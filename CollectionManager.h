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


private:
	bool LoadData(void);
	bool LoadCollectionsFromFile(
		const std::filesystem::path& defFile, nlohmann::json_schema::json_validator& validator);
	void BuildDecisionTrees(nlohmann::json& collectionDefinitions);
	void ResolveMembership(void);
	void AddToRelevantCollections(RE::FormID itemID);
	std::vector<RE::FormID> ReconcileInventory();
	void EnqueueAddedItem(const RE::FormID formID);

	static std::unique_ptr<CollectionManager> m_instance;
	bool m_ready;
	bool m_enabled;
	float m_gameTime;

	mutable RecursiveLock m_collectionLock;
	std::unordered_map<std::string, std::shared_ptr<Collection>> m_collectionByName;
	// Link each Form to the Collections in which it belongs
	std::unordered_multimap<RE::FormID, std::shared_ptr<Collection>> m_collectionsByFormID;
	std::unordered_set<RE::FormID> m_nonCollectionForms;

	std::vector<RE::FormID> m_addedItemQueue;
	std::unordered_set<RE::FormID> m_lastInventoryItems;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_lastInventoryCheck;
};

}
