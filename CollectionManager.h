#pragma once

namespace shse {

class CollectionManager {
public:
	static CollectionManager& Instance();

	CollectionManager();
	void ProcessDefinitions(void);
	void EnqueueAddedItems(const std::vector<std::pair<RE::FormID, ObjectType>>& looted);
	void EnqueueAddedItem(const RE::FormID formID, const ObjectType objectType);
	void ProcessAddedItems();
	inline bool IsReady() { return m_ready; }

private:
	bool LoadData(void);
	void BuildDecisionTrees(void);
	void AddToRelevantCollections(RE::FormID itemID, const ObjectType objectType);
	void ReconcileInventory();
	void PrintCollections(void);

	static std::unique_ptr<CollectionManager> m_instance;
	bool m_ready;
	std::unordered_map<std::string, std::unique_ptr<Collection>> m_collectionByName;
	nlohmann::json m_collectionDefinitions;
	RecursiveLock m_collectionLock;
	std::vector<std::pair<RE::FormID, ObjectType>> m_addedItemQueue;
	std::unordered_set<RE::FormID> m_checkedItems;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_lastInventoryCheck;
};

}
