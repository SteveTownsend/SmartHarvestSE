#pragma once

namespace shse
{

class CollectionFactory {
public:
	static CollectionFactory& Instance();
	std::unique_ptr<Collection> ParseCollection(const nlohmann::json& collection) const;

private:
	std::unique_ptr<PluginCondition> ParsePlugin(const nlohmann::json& pluginRule) const;
	std::unique_ptr<KeywordsCondition> ParseKeywords(const nlohmann::json& keywordRule) const;
	std::unique_ptr<SignaturesCondition> ParseSignatures(const nlohmann::json& signatureRule) const;
	std::unique_ptr<LootCategoriesCondition> ParseLootCategories(const nlohmann::json& lootCategoryRule) const;
	std::unique_ptr<ConditionTree> ParseFilter(const nlohmann::json& tree, const unsigned int depth) const;

	std::unique_ptr<CollectionFactory> m_factory;
};

}
