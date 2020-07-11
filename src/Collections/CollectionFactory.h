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
	std::unique_ptr<ScopeCondition> ParseScope(const nlohmann::json& scopeRule) const;
	std::unique_ptr<ConditionTree> ParseFilter(const nlohmann::json& tree, const unsigned int depth) const;
	CollectionPolicy ParsePolicy(const nlohmann::json& policy) const;

	std::unique_ptr<CollectionFactory> m_factory;
};

}
