#pragma once

class CollectionManager {
public:
	static CollectionManager& Instance();
	void ProcessDefinitions(void);
	void EnqueueAddedItems(const std::vector<std::pair<RE::FormID, ObjectType>>& looted);
	void EnqueueAddedItem(const RE::FormID formID, const ObjectType objectType);
	void ProcessAddedItems();

private:
	bool LoadData(void);
	void BuildDecisionTrees(void);
	void AddToRelevantCollections(RE::FormID itemID, const ObjectType objectType);
	void ReconcileInventory();
#if _DEBUG
	void PrintCollections(void);
#endif

	static std::unique_ptr<CollectionManager> m_instance;
	std::unordered_map<std::string, std::unique_ptr<Collection>> m_collectionByName;
	nlohmann::json m_collectionDefinitions;
	RecursiveLock m_collectionLock;
	std::vector<std::pair<RE::FormID, ObjectType>> m_addedItemQueue;
	std::unordered_set<RE::FormID> m_checkedItems;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_lastInventoryCheck;
};
