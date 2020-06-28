#pragma once

#include "Collections/Collection.h"
namespace shse
{

class CollectionFactory {
public:
	static CollectionFactory& Instance();
	std::shared_ptr<Collection> ParseCollection(const nlohmann::json& collection) const;

private:
	std::unique_ptr<PluginCondition> ParsePlugin(const nlohmann::json& pluginRule) const;
	std::unique_ptr<FormListCondition> ParseFormList(const nlohmann::json& formListRule) const;
	std::unique_ptr<KeywordCondition> ParseKeyword(const nlohmann::json& keywordRule) const;
	std::unique_ptr<SignatureCondition> ParseSignature(const nlohmann::json& signatureRule) const;
	std::unique_ptr<ConditionTree> ParseFilter(const nlohmann::json& tree, const unsigned int depth) const;
	CollectionPolicy ParsePolicy(const nlohmann::json& policy) const;

	std::unique_ptr<CollectionFactory> m_factory;
};

}
