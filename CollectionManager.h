#pragma once

class CollectionManager {
public:
	static CollectionManager& Instance();
	void ProcessDefinitions(void);

private:
	bool LoadData(void);
	void BuildDecisionGraph(void);
#if _DEBUG
	void PrintCollections(void);
#endif

	static std::unique_ptr<CollectionManager> m_instance;
	std::unordered_map<std::string, std::unique_ptr<Condition>> m_filterByName;
	nlohmann::json m_collectionDefinitions;
};
