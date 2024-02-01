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

#include "Collections/Condition.h"
#include "Collections/CollectionFactory.h"
#include "Collections/CollectionManager.h"
#include "Utilities/utils.h"

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
	std::transform(pluginRule.cbegin(), pluginRule.cend(), std::back_inserter(plugins),
		[&](const nlohmann::json& next) { return next.get<std::string>(); });
	return std::make_unique<PluginCondition>(plugins);
}

std::unique_ptr<FormListCondition> CollectionFactory::ParseFormList(const nlohmann::json& formListRule) const
{
	std::vector<std::pair<std::string, std::string>> pluginFormLists;
	pluginFormLists.reserve(formListRule.size());
	std::transform(formListRule.cbegin(), formListRule.cend(), std::back_inserter(pluginFormLists),
		[&](const nlohmann::json& next) { return std::make_pair(next["listPlugin"], next["formID"]); });
	return std::make_unique<FormListCondition>(pluginFormLists);
}

std::unique_ptr<FormsCondition> CollectionFactory::ParseForms(const nlohmann::json& formsRule) const
{
	return std::make_unique<FormsCondition>(JSONUtils::ParseFormsType(formsRule));
}

std::unique_ptr<KeywordCondition> CollectionFactory::ParseKeyword(const nlohmann::json& keywordRule) const
{
	std::vector<std::string> keywords;
	keywords.reserve(keywordRule.size());
	std::transform(keywordRule.cbegin(), keywordRule.cend(), std::back_inserter(keywords),
		[&](const nlohmann::json& next) { return next.get<std::string>(); });
	return std::make_unique<KeywordCondition>(keywords);
}

std::unique_ptr<SignatureCondition> CollectionFactory::ParseSignature(const nlohmann::json& signatureRule) const
{
	std::vector<std::string> signatures;
	signatures.reserve(signatureRule.size());
	std::transform(signatureRule.cbegin(), signatureRule.cend(), std::back_inserter(signatures),
		[&](const nlohmann::json& next) { return next.get<std::string>(); });
	return std::make_unique<SignatureCondition>(signatures);
}

std::unique_ptr<ScopeCondition> CollectionFactory::ParseScope(const nlohmann::json& scopeRule) const
{
	std::vector<std::string> scopes;
	scopes.reserve(scopeRule.size());
	std::transform(scopeRule.cbegin(), scopeRule.cend(), std::back_inserter(scopes),
		[&](const nlohmann::json& next) { return next.get<std::string>(); });
	return std::make_unique<ScopeCondition>(scopes);
}

std::unique_ptr<NameMatchCondition> CollectionFactory::ParseNameMatch(const nlohmann::json& nameMatchRule) const
{
	std::vector<std::string> names;
	names.reserve(nameMatchRule["names"].size());
	std::transform(std::cbegin(nameMatchRule["names"]), std::cend(nameMatchRule["names"]), std::back_inserter(names),
		[&](const nlohmann::json& next) { return next.get<std::string>(); });
	return std::make_unique<NameMatchCondition>(nameMatchRule["isNPC"].get<bool>(), nameMatchRule["matchIf"].get<std::string>(), names);
}

CollectionPolicy CollectionFactory::ParsePolicy(const nlohmann::json& policy) const
{
	return CollectionPolicy(ParseCollectibleHandling(policy["action"].get<std::string>()),
		policy["notify"].get<bool>(), policy["repeat"].get<bool>());
}

std::unique_ptr<FilterTree> CollectionFactory::ParseFilter(const nlohmann::json& filter, const unsigned int depth) const
{
	FilterTree::Operator op;
	if (filter["operator"].get<std::string>() == "AND")
	{
		op = FilterTree::Operator::And;
	}
	else
	{
		op = FilterTree::Operator::Or;
	}
	std::unique_ptr<FilterTree> root(std::make_unique<FilterTree>(op, depth));
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
		else if (condition.key() == "forms")
		{
			root->AddCondition(ParseForms(condition.value()));
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
		else if (condition.key() == std::string("nameMatch"))
		{
			root->AddCondition(ParseNameMatch(condition.value()));
		}
	}

	return root;
}

std::unique_ptr<CategoryRule> CollectionFactory::ParseCategory(const nlohmann::json& categoryRule) const
{
	std::unique_ptr<CategoryRule> rule(std::make_unique<CategoryRule>());
	std::vector<std::string> categories;
	categories.reserve(categoryRule.size());
	std::transform(categoryRule.cbegin(), categoryRule.cend(), std::back_inserter(categories),
		[&](const nlohmann::json& next) { return next.get<std::string>(); });
	rule->SetCondition(std::make_unique<CategoryCondition>(categories));
	return rule;
}

std::shared_ptr<Collection> CollectionFactory::ParseCollection(
	const CollectionGroup* owningGroup, const nlohmann::json& collection, const CollectionPolicy& defaultPolicy) const
{
	const auto policy(collection.find("policy"));
	const std::string name(collection["name"].get<std::string>());
	bool overridesPolicy(policy != collection.cend());
	DBG_VMESSAGE("Collection {}, overrides Policy = {}", name.c_str(), overridesPolicy ? "true" : "false");
	if (collection.find("rootFilter") != collection.cend())
	{
		return std::make_shared<ConditionCollection>(owningGroup, name, collection["description"].get<std::string>(),
			policy != collection.cend() ? ParsePolicy(collection["policy"]) : defaultPolicy, overridesPolicy, ParseFilter(collection["rootFilter"], 0));
	}
	else if (collection.find("category") != collection.cend())
	{
		return std::make_shared<CategoryCollection>(owningGroup, name, collection["description"].get<std::string>(),
			policy != collection.cend() ? ParsePolicy(collection["policy"]) : defaultPolicy, overridesPolicy, ParseCategory(collection["category"]));
	}
	return std::shared_ptr<Collection>();
}

std::shared_ptr<CollectionGroup> CollectionFactory::ParseGroup(
	CollectionManager& manager, const nlohmann::json& group, const std::string& groupName) const
{
	return std::make_shared<CollectionGroup>(manager, groupName, ParsePolicy(group["groupPolicy"]), group["useMCM"].get<bool>(), group["collections"]);
}

}
