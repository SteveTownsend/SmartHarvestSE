#include "PrecompiledHeaders.h"

#include "Collections/Condition.h"
#include "Collections/CollectionFactory.h"

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
	std::vector<std::string> plugins;
	plugins.reserve(pluginRule.size());
	std::transform(pluginRule.begin(), pluginRule.end(), std::back_inserter(plugins),
		[&](const nlohmann::json& next) { return next.get<std::string>(); });
	return std::make_unique<PluginCondition>(plugins);
}

std::unique_ptr<FormListCondition> CollectionFactory::ParseFormList(const nlohmann::json& formListRule) const
{
	return std::make_unique<FormListCondition>(
		formListRule["listPlugin"].get<std::string>(), formListRule["formID"].get<std::string>());
}

std::unique_ptr<KeywordCondition> CollectionFactory::ParseKeyword(const nlohmann::json& keywordRule) const
{
	std::vector<std::string> keywords;
	keywords.reserve(keywordRule.size());
	std::transform(keywordRule.begin(), keywordRule.end(), std::back_inserter(keywords),
		[&](const nlohmann::json& next) { return next.get<std::string>(); });
	return std::make_unique<KeywordCondition>(keywords);
}

std::unique_ptr<SignatureCondition> CollectionFactory::ParseSignature(const nlohmann::json& signatureRule) const
{
	std::vector<std::string> signatures;
	signatures.reserve(signatureRule.size());
	std::transform(signatureRule.begin(), signatureRule.end(), std::back_inserter(signatures),
		[&](const nlohmann::json& next) { return next.get<std::string>(); });
	return std::make_unique<SignatureCondition>(signatures);
}

std::unique_ptr<ScopeCondition> CollectionFactory::ParseScope(const nlohmann::json& scopeRule) const
{
	std::vector<std::string> scopes;
	scopes.reserve(scopeRule.size());
	std::transform(scopeRule.begin(), scopeRule.end(), std::back_inserter(scopes),
		[&](const nlohmann::json& next) { return next.get<std::string>(); });
	return std::make_unique<ScopeCondition>(scopes);
}

CollectionPolicy CollectionFactory::ParsePolicy(const nlohmann::json& policy) const
{
	return CollectionPolicy(ParseSpecialObjectHandling(policy["action"].get<std::string>()),
		policy["notify"].get<bool>(), policy["repeat"].get<bool>());
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
	for (const auto& condition : filter["condition"].items())
	{
		if (condition.key() == "subFilter")
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
		else if (condition.key() == "formList")
		{
			root->AddCondition(ParseFormList(condition.value()));
		}
		else if (condition.key() == "keyword")
		{
			root->AddCondition(ParseKeyword(condition.value()));
		}
		else if (condition.key() == std::string("signature"))
		{
			root->AddCondition(ParseSignature(condition.value()));
		}
		else if (condition.key() == std::string("scope"))
		{
			root->AddCondition(ParseScope(condition.value()));
		}
	}

	return root;
}

std::shared_ptr<Collection> CollectionFactory::ParseCollection(const nlohmann::json& collection) const
{
	return std::make_shared<Collection>(collection["name"].get<std::string>(),
		collection["description"].get<std::string>(), ParsePolicy(collection["policy"]), ParseFilter(collection["rootFilter"], 0));
}

}
