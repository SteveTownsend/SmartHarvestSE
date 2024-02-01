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

#include "Data/LoadOrder.h"
#include "Collections/Condition.h"
#include "Looting/objects.h"
#include "Utilities/Exception.h"
#include "Utilities/utils.h"

namespace shse
{

Condition::Condition() {}
Condition::~Condition() {}

// no static members
std::unordered_set<const RE::TESForm*> Condition::StaticMembers() const
{
	return std::unordered_set<const RE::TESForm*>();
}

nlohmann::json Condition::MakeJSON() const
{
	return nlohmann::json(*this);
}

bool CanBeCollected(RE::TESForm* form)
{
	RE::FormType formType(form->GetFormType());
	return formType == RE::FormType::AlchemyItem ||
		formType == RE::FormType::Armor ||
		formType == RE::FormType::Book ||
		formType == RE::FormType::Ingredient ||
		formType == RE::FormType::KeyMaster ||
		formType == RE::FormType::Misc ||
		formType == RE::FormType::SoulGem ||
		formType == RE::FormType::Weapon;
}

PluginCondition::PluginCondition(const std::vector<std::string>& plugins)
{
	for (const auto& plugin : plugins)
	{
		RE::FormID formIDMask(LoadOrder::Instance().GetFormIDMask(plugin));
		if (formIDMask == InvalidPlugin)
		{
			std::ostringstream err;
			err << "Unknown plugin: " << plugin;
			throw PluginError(err.str().c_str());
		}
		m_formIDMaskByPlugin.insert(std::make_pair(plugin, formIDMask));
	}
}

bool PluginCondition::operator()(const ConditionMatcher& matcher) const
{
	for (const auto plugin : m_formIDMaskByPlugin)
	{
		if (LoadOrder::Instance().ModOwnsForm(plugin.first, matcher.Form()->GetFormID()))
			return true;
	}
	return false;
}

void PluginCondition::AsJSON(nlohmann::json& j) const
{
	j["plugin"] = nlohmann::json::array();
	for (const auto plugin : m_formIDMaskByPlugin)
	{
		j["plugin"].push_back(plugin.first);
	}
}

FormListCondition::FormListCondition(const std::vector<std::pair<std::string, std::string>>& pluginFormList)
{
	for (const auto& entry : pluginFormList)
	{
		// schema enforces 8-char HEX format
		RE::FormID formID(StringUtils::ToFormID(entry.second));
		RE::BGSListForm* formList(RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSListForm>(LoadOrder::Instance().AsRaw(formID), entry.first));
		if (!formList)
		{
			REL_ERROR("FormListCondition cannot resolve FormList {}/0x{:08x}", entry.first.c_str(), formID);
			return;
		}
		DBG_VMESSAGE("Resolved FormList 0x{:08x}", formID);
		m_formLists.push_back(std::make_pair(formList, entry.first));
		FlattenMembers(formList);
	}
}

void FormListCondition::FlattenMembers(const RE::BGSListForm* formList)
{
	if (!formList)
		return;
	for (const auto candidate : formList->forms)
	{
		if (candidate->GetFormType() == RE::FormType::FormList)
		{
			FlattenMembers(candidate->As<RE::BGSListForm>());
		}
		else if (CanBeCollected(candidate))
		{
			DBG_VMESSAGE("FormList Member found {}/0x{:08x}", candidate->GetName(), candidate->GetFormID());
			m_listMembers.insert(candidate);
		}
	}
}

bool FormListCondition::operator()(const ConditionMatcher& matcher) const
{
	return m_listMembers.contains(matcher.Form());
}

void FormListCondition::AsJSON(nlohmann::json& j) const
{
	j["formList"] = nlohmann::json::array();
	for (const auto formList : m_formLists)
	{
		auto next(nlohmann::json::object());
		next["formID"] = StringUtils::FromFormID(formList.first->GetFormID());
		next["listPlugin"] = "";
		j["formList"].push_back(next);
	}
}

FormsCondition::FormsCondition(const std::vector<std::pair<std::string, std::vector<std::string>>>& pluginForms)
{
	for (const auto& entry : pluginForms)
	{
		std::vector<RE::TESForm*> newForms;
		newForms.reserve(entry.second.size());
		for (const auto nextID : entry.second)
		{
			// schema enforces 8-char HEX format
			RE::FormID formID(StringUtils::ToFormID(nextID));
			RE::TESForm* form(RE::TESDataHandler::GetSingleton()->LookupForm(LoadOrder::Instance().AsRaw(formID), entry.first));
			if (!form)
			{
				REL_ERROR("FormsCondition requires valid Forms, got {}/0x{:08x}", entry.first.c_str(), formID);
				return;
			}
			DBG_VMESSAGE("Resolved Form 0x{:08x}", form->GetFormID());
			newForms.push_back(form);
		}
		m_formsByPlugin.insert({ entry.first, newForms });
		m_allForms.insert(newForms.cbegin(), newForms.cend());
	}
}

std::unordered_set<const RE::TESForm*> FormsCondition::StaticMembers() const
{ 
	return m_allForms;
}

bool FormsCondition::operator()(const ConditionMatcher& matcher) const
{
	return m_allForms.contains(matcher.Form());
}

void FormsCondition::AsJSON(nlohmann::json& j) const
{
	j["forms"] = nlohmann::json::array();
	for (const auto pluginData : m_formsByPlugin)
	{
		auto next(nlohmann::json::object());
		next["plugin"] = pluginData.first;
		next["form"] = nlohmann::json::array();
		for (const auto form : pluginData.second)
		{
			next["form"].push_back(StringUtils::FromFormID(form->GetFormID()));
		}
	}
}

// This is O(n) in KYWD record count but only happens during startup, and there are not THAT many of them
KeywordCondition::KeywordCondition(const std::vector<std::string>& keywords)
{
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	if (!dhnd)
		return;

	// store keywords to match for this collection. Schema enforces uniqueness in input list.
	std::vector<std::string> keywordsLeft(keywords);
	for (const RE::BGSKeyword* keywordRecord : dhnd->GetFormArray<RE::BGSKeyword>())
	{
		auto matched(std::find_if(keywordsLeft.begin(), keywordsLeft.end(),
			[&](const std::string& keyword) -> bool { return FormUtils::SafeGetFormEditorID(keywordRecord) == keyword; }));
		if (matched != keywordsLeft.end())
		{
			m_keywords.insert(keywordRecord);
			DBG_VMESSAGE("BGSKeyword recorded for {}", FormUtils::SafeGetFormEditorID(keywordRecord).c_str());
			// eliminate the matched candidate input from JSON
			keywordsLeft.erase(matched);

			// If all candidates have been located, we are done
			if (keywordsLeft.empty())
				break;
		}
	}
	for (const std::string& badKeyword : keywordsLeft)
	{
		REL_WARNING("Collection has invalid KYWD {}", badKeyword.c_str());
	}
	if (!keywordsLeft.empty())
	{
		std::ostringstream err;
		err << "Unknown KYWD: " << keywordsLeft.front();
		throw KeywordError(err.str().c_str());
	}
}

bool KeywordCondition::operator()(const ConditionMatcher& matcher) const
{
	const RE::BGSKeywordForm* keywordHolder(matcher.Form()->As<RE::BGSKeywordForm>());
	if (!keywordHolder)
		return false;
	return std::find_if(m_keywords.cbegin(), m_keywords.cend(),
		[=](const RE::BGSKeyword* keyword) -> bool { return keywordHolder->HasKeyword(keyword); }) != m_keywords.cend();
}

void KeywordCondition::AsJSON(nlohmann::json& j) const
{
	j["keyword"] = nlohmann::json::array();
	for (const auto keyword : m_keywords)
	{
		j["keyword"].push_back(FormUtils::SafeGetFormEditorID(keyword));
	}
}

// This condition relies on objectType at scan time - it cannot be reliably determined at startup, so Collection
// Members are not resolved then.
CategoryCondition::CategoryCondition(const std::vector<std::string>& categories)
{
	for (const auto& category : categories)
	{
		ObjectType objectType(GetObjectTypeByTypeName(category));
		if (objectType != ObjectType::unknown)
		{
			m_objectTypes.push_back(objectType);
			DBG_VMESSAGE("Category {} mapped to ObjectType {}", category.c_str(), static_cast<int>(objectType));
		}
	}
}
bool CategoryCondition::operator()(const ConditionMatcher& matcher) const
{
	// short linear scan
	return std::find(m_objectTypes.cbegin(), m_objectTypes.cend(), matcher.GetObjectType()) != m_objectTypes.cend();
}

void CategoryCondition::AsJSON(nlohmann::json& j) const
{
	j["category"] = nlohmann::json::array();
	for (const auto objectType : m_objectTypes)
	{
		j["category"].push_back(GetObjectTypeName(objectType));
	}
}

const std::unordered_map<std::string, RE::FormType> SignatureCondition::m_validSignatures = {
	{"ALCH", RE::FormType::AlchemyItem},
	{"ARMO", RE::FormType::Armor},
	{"BOOK", RE::FormType::Book},
	{"INGR", RE::FormType::Ingredient},
	{"KEYM", RE::FormType::KeyMaster},
	{"MISC", RE::FormType::Misc},
	{"SLGM", RE::FormType::SoulGem},
	{"WEAP", RE::FormType::Weapon}
};

const std::vector<RE::FormType> SignatureCondition::m_validFormTypes = {
	RE::FormType::AlchemyItem,
	RE::FormType::Armor,
	RE::FormType::Book,
	RE::FormType::Ingredient,
	RE::FormType::KeyMaster,
	RE::FormType::Misc,
	RE::FormType::SoulGem,
	RE::FormType::Weapon
};

// This is O(n) in TESV Record Type count but only happens during startup, and there are not many of them
SignatureCondition::SignatureCondition(const std::vector<std::string>& signatures)
{
	for (const auto& signature : signatures)
	{
		auto matched(m_validSignatures.find(signature));
		if (matched != m_validSignatures.cend())
		{
			m_formTypes.push_back(matched->second);
			DBG_VMESSAGE("Record Signature {} mapped to FormType {}", signature.c_str(), static_cast<int>(matched->second));
		}
	}
}

bool SignatureCondition::operator()(const ConditionMatcher& matcher) const
{
	// short linear scan
	return std::find(m_formTypes.cbegin(), m_formTypes.cend(), matcher.Form()->GetFormType()) != m_formTypes.cend();
}

std::string SignatureCondition::FormTypeAsSignature(const RE::FormType formType)
{
	// very short linear scan, for file dump
	for (const auto validSignature : m_validSignatures)
	{
		if (validSignature.second == formType)
			return validSignature.first;
	}
	return "";
}

const decltype(SignatureCondition::m_validSignatures) SignatureCondition::ValidSignatures()
{
	return m_validSignatures;
}

bool SignatureCondition::IsValidFormType(const RE::FormType formType)
{
	return std::find(m_validFormTypes.cbegin(), m_validFormTypes.cend(), formType) != m_validFormTypes.cend();
}

void SignatureCondition::AsJSON(nlohmann::json& j) const
{
	j["signature"] = nlohmann::json::array();
	for (const auto formType : m_formTypes)
	{
		j["signature"].push_back(FormTypeAsSignature(formType));
	}
}

const std::unordered_map<std::string, INIFile::SecondaryType> ScopeCondition::m_validScopes = {
	{"deadBody", INIFile::SecondaryType::deadbodies},
	{"container", INIFile::SecondaryType::containers},
	{"looseItem", INIFile::SecondaryType::itemObjects}
};

ScopeCondition::ScopeCondition(const std::vector<std::string>& scopes)
{
	for (const auto& scope : scopes)
	{
		auto matched(m_validScopes.find(scope));
		if (matched != m_validScopes.cend())
		{
			m_scopes.push_back(matched->second);
			std::string target(INIFile::GetInstance()->SecondaryTypeString(matched->second));
			DBG_VMESSAGE("Scope {} mapped to FormType {}", scope.c_str(), target.c_str());
		}
	}
}

bool ScopeCondition::operator()(const ConditionMatcher& matcher) const
{
	// Scope is aggregated during form filtering game-data load, for use in live checking
	if (matcher.Scope() == INIFile::SecondaryType::NONE2)
	{
		for (auto scope : m_scopes)
			matcher.AddScope(scope);
		return true;
	}
	// very short linear scan
	return std::find(m_scopes.cbegin(), m_scopes.cend(), matcher.Scope()) != m_scopes.cend();
}

std::string ScopeCondition::SecondaryTypeAsScope(const INIFile::SecondaryType scope)
{
	// very short linear scan, for file dump
	for (const auto validScope : m_validScopes)
	{
		if (validScope.second == scope)
			return validScope.first;
	}
	return "";
}

void ScopeCondition::AsJSON(nlohmann::json& j) const
{
	j["scope"] = nlohmann::json::array();
	for (const auto scope : m_scopes)
	{
		j["scope"].push_back(SecondaryTypeAsScope(scope));
	}
}

NameMatchCondition::NameMatchCondition(const bool isNPC, const std::string& matchIf, const std::vector<std::string>& names) : m_isNPC(isNPC)
{
	m_matchIf = NameMatchTypeByName(matchIf);
	if (m_matchIf != NameMatchType::Invalid)
	{
		DBG_VMESSAGE("Match {} mapped to NameMatchType {}", matchIf, static_cast<int>(m_matchIf));
	}
	// skip empty values
	std::copy_if(std::cbegin(names), std::cend(names), std::back_inserter(m_names),
		[](const std::string& name) -> bool { return !name.empty(); });
}

bool NameMatchCondition::operator()(const ConditionMatcher& matcher) const
{
	std::string formName(matcher.Form()->GetName());
	if (formName.empty())
		return false;
	if (m_isNPC && matcher.Form()->GetFormType() != RE::FormType::NPC)
		return false;
	if (!m_isNPC && matcher.Form()->GetFormType() == RE::FormType::NPC)
		return false;

	for (const auto& name : m_names)
	{
		switch (m_matchIf) {
		case NameMatchType::Contains:
			// any substring, could be StartsWith or Equals
			if (formName.find(name) != std::string::npos)
				return true;
			break;
		case NameMatchType::StartsWith:
			if (formName.starts_with(name))
				return true;
			break;
		case NameMatchType::Equals:
			if (name == formName)
				return true;
			break;
		case NameMatchType::NotEquals:
			if (name == formName)
				return false;
			break;
		case NameMatchType::Omits:
			// any substring, could be StartsWith or Equals
			if (formName.find(name) != std::string::npos)
				return false;
			break;
		default:
			break;
		}
	}
	return ResultIfAllNamesFail();
}

void NameMatchCondition::AsJSON(nlohmann::json& j) const
{
	nlohmann::json nameMatch(nlohmann::json::object());
	nameMatch["matchIf"] = NameMatchTypeName(m_matchIf);
	nameMatch["names"] = m_names;
	j["nameMatch"] = nameMatch;
}

FilterTree::FilterTree(const Operator op, const unsigned int depth) : m_operator(op), m_depth(depth)
{
}

void FilterTree::AddCondition(std::unique_ptr<Condition> condition)
{
	m_conditions.push_back(std::move(condition));
}

bool FilterTree::operator()(const ConditionMatcher& matcher) const
{
	for (const auto& condition : m_conditions)
	{
		bool checkThis(condition->operator()(matcher));
		if (m_operator == Operator::And)
		{
			// All must match
			if (!checkThis)
				return false;
		}
		else
		{
			if (checkThis)
				return true;
		}
	}
	// we scanned the entire list - this implies success for AND, failure for OR
	return m_operator == Operator::And;
}

std::unordered_set<const RE::TESForm*> FilterTree::StaticMembers() const
{
	std::unordered_set<const RE::TESForm*> aggregated;
	for (const auto& condition : m_conditions)
	{
		auto forms(condition->StaticMembers());
		aggregated.insert(forms.cbegin(), forms.cend());
	}
	return aggregated;
}

void FilterTree::AsJSON(nlohmann::json& j) const
{
	nlohmann::json tree(nlohmann::json::object());
	tree["operator"] = std::string(m_operator == Operator::And ? "AND" : "OR");
	tree["condition"] = nlohmann::json::array();
	for (const auto& condition : m_conditions)
	{
		tree["condition"].push_back(nlohmann::json(*condition));
	}
	if (m_depth == 0)
	{
		j["rootFilter"] = tree;
	}
	else
	{
		j["subFilter"] = tree;
	}
}

void CategoryRule::SetCondition(std::unique_ptr<CategoryCondition> condition)
{
	m_condition = std::move(condition);
}

bool CategoryRule::operator()(const ConditionMatcher& matcher) const
{
	 return m_condition->operator()(matcher);
}

void CategoryRule::AsJSON(nlohmann::json& j) const
{
	m_condition->AsJSON(j);
}

void to_json(nlohmann::json& j, const Condition& condition)
{
	condition.AsJSON(j);
}

void to_json(nlohmann::json& j, const CategoryRule& categoryRule)
{
	categoryRule.AsJSON(j);
}

ConditionMatcher::ConditionMatcher(const RE::TESForm* form) :
	m_form(form), m_scope(INIFile::SecondaryType::NONE2), m_objectType(ObjectType::unknown)
{
}

ConditionMatcher::ConditionMatcher(const RE::TESForm* form, const INIFile::SecondaryType scope, const ObjectType objectType) :
	m_form(form), m_scope(scope), m_objectType(objectType)
{
}

// accumulates scopes seen in FilterTree during game-data load, to optimize collectible check during periodic REFR scan
void ConditionMatcher::AddScope(const INIFile::SecondaryType scope) const
{
	if (std::find(m_scopesSeen.cbegin(), m_scopesSeen.cend(), scope) == m_scopesSeen.cend())
	{
		m_scopesSeen.push_back(scope);
	}
}

}

std::ostream& operator<<(std::ostream& os, const shse::Condition& condition)
{
	os << condition.MakeJSON().dump(2);
	return os;
}
