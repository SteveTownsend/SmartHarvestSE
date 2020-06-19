#include "PrecompiledHeaders.h"

namespace shse
{

std::unique_ptr<CollectionFactory> m_instance;

CollectionFactory& CollectionFactory::Instance()
{
	if (!m_instance)
	{
		m_instance.reset(new CollectionFactory);
	}
	return *m_instance;
}

std::unique_ptr<PluginCondition> CollectionFactory::ParsePlugin(const nlohmann::json& pluginRule) const
{
	return std::make_unique<PluginCondition>(pluginRule.get<std::string>());
}

std::unique_ptr<KeywordsCondition> CollectionFactory::ParseKeywords(const nlohmann::json& keywordRule) const
{
	std::vector<std::string> keywords;
	keywords.reserve(keywordRule.size());
	std::transform(keywordRule.begin(), keywordRule.end(), std::back_inserter(keywords),
		[&](const nlohmann::json& next) { return next.get<std::string>(); });
	return std::make_unique<KeywordsCondition>(keywords);
}

std::unique_ptr<SignaturesCondition> CollectionFactory::ParseSignatures(const nlohmann::json& signatureRule) const
{
	std::vector<std::string> signatures;
	signatures.reserve(signatureRule.size());
	std::transform(signatureRule.begin(), signatureRule.end(), std::back_inserter(signatures),
		[&](const nlohmann::json& next) { return next.get<std::string>(); });
	return std::make_unique<SignaturesCondition>(signatures);
}

std::unique_ptr<LootCategoriesCondition> CollectionFactory::ParseLootCategories(const nlohmann::json& lootCategoryRule) const
{
	std::vector<std::string> lootCategories;
	lootCategories.reserve(lootCategoryRule.size());
	std::transform(lootCategoryRule.begin(), lootCategoryRule.end(), std::back_inserter(lootCategories),
		[&](const nlohmann::json& next) { return next.get<std::string>(); });
	return std::make_unique<LootCategoriesCondition>(lootCategories);
}

std::unique_ptr<ConditionTree> CollectionFactory::ParseFilter(const nlohmann::json& filter, const unsigned int depth) const
{
	ConditionTree::Operator op;
	if (filter["operator"].get<std::string>() == "AND")
	{
		op = ConditionTree::Operator::And;
	}
	else
	{
		op = ConditionTree::Operator::Or;
	}
	std::unique_ptr<ConditionTree> root(std::make_unique<ConditionTree>(op, depth));
	for (const auto& condition : filter["conditions"].items())
	{
		if (condition.key() == "subFilters")
		{
			for (const auto& subFilter : condition.value())
			{
				root->AddCondition(ParseFilter(subFilter, depth + 1));
			}
		}
		else if (condition.key() == "plugin")
		{
			root->AddCondition(ParsePlugin(condition.value()));
		}
		else if (condition.key() == "keywords")
		{
			root->AddCondition(ParseKeywords(condition.value()));
		}
		else if (condition.key() == std::string("signatures"))
		{
			root->AddCondition(ParseSignatures(condition.value()));
		}
		else if (condition.key() == std::string("lootCategories"))
		{
			root->AddCondition(ParseLootCategories(condition.value()));
		}
	}

	return root;
}

std::unique_ptr<Collection> CollectionFactory::ParseCollection(const nlohmann::json& collection) const
{
	return std::make_unique<Collection>(collection["name"].get<std::string>(),
		collection["description"].get<std::string>(), ParseFilter(collection["rootFilter"], 0));
}

}
