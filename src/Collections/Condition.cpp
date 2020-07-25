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
#include "Utilities/Exception.h"
#include "Utilities/utils.h"

namespace shse
{

Condition::Condition() {}
Condition::~Condition() {}

// default - no static members
std::unordered_set<const RE::TESForm*> Condition::StaticMembers() const
{
	return std::unordered_set<const RE::TESForm*>();
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

nlohmann::json PluginCondition::MakeJSON() const
{
	return nlohmann::json(*this);
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
		RE::FormID formID;
		std::stringstream ss;
		ss << std::hex << entry.second;
		ss >> formID;
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

nlohmann::json FormListCondition::MakeJSON() const
{
	return nlohmann::json(*this);
}

void FormListCondition::AsJSON(nlohmann::json& j) const
{
	j["formList"] = nlohmann::json::array();
	for (const auto formList : m_formLists)
	{
		std::ostringstream formStr;
		formStr << std::hex << std::setfill('0') << std::setw(8) << formList.first->GetFormID();
		auto next(nlohmann::json::object());
		next["formID"] = formStr.str();
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
			RE::FormID formID;
			std::stringstream ss;
			ss << std::hex << nextID;
			ss >> formID;
			RE::TESForm* form(RE::TESDataHandler::GetSingleton()->LookupForm(LoadOrder::Instance().AsRaw(formID), entry.first));
			if (!form)
			{
				REL_ERROR("FormsCondition requires valid Forms, got {}/0x{:08x}", entry.first.c_str(), formID);
				return;
			}
			DBG_VMESSAGE("Resolved Form 0x{:08x}", formID);
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

nlohmann::json FormsCondition::MakeJSON() const
{
	return nlohmann::json(*this);
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
			std::ostringstream formStr;
			formStr << std::hex << std::setfill('0') << std::setw(8) << form->GetFormID();
			next["form"].push_back(formStr.str());
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
		DBG_WARNING("Collection has invalid KYWD {}", badKeyword.c_str());
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

nlohmann::json KeywordCondition::MakeJSON() const
{
	return nlohmann::json(*this);
}

void KeywordCondition::AsJSON(nlohmann::json& j) const
{
	j["keyword"] = nlohmann::json::array();
	for (const auto keyword : m_keywords)
	{
		j["keyword"].push_back(FormUtils::SafeGetFormEditorID(keyword));
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

nlohmann::json SignatureCondition::MakeJSON() const
{
	return nlohmann::json(*this);
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

nlohmann::json ScopeCondition::MakeJSON() const
{
	return nlohmann::json(*this);
}

void ScopeCondition::AsJSON(nlohmann::json& j) const
{
	j["scope"] = nlohmann::json::array();
	for (const auto scope : m_scopes)
	{
		j["scope"].push_back(SecondaryTypeAsScope(scope));
	}
}

ConditionTree::ConditionTree(const Operator op, const unsigned int depth) : m_operator(op), m_depth(depth)
{
}

bool ConditionTree::operator()(const ConditionMatcher& matcher) const
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

void ConditionTree::AddCondition(std::unique_ptr<Condition> condition)
{
	m_conditions.push_back(std::move(condition));
}

nlohmann::json ConditionTree::MakeJSON() const
{
	return nlohmann::json(*this);
}

void ConditionTree::AsJSON(nlohmann::json& j) const
{
	j["operator"] = std::string(m_operator == shse::ConditionTree::Operator::And ? "AND" : "OR");
	j["condition"] = nlohmann::json::array();
	for (const auto& condition : m_conditions)
	{
		j["condition"].push_back(condition->MakeJSON());
	}
}

std::unordered_set<const RE::TESForm*> ConditionTree::StaticMembers() const
{
	std::unordered_set<const RE::TESForm*> aggregated;
	for (const auto& condition : m_conditions)
	{
		auto forms(condition->StaticMembers());
		aggregated.insert(forms.cbegin(), forms.cend());
	}
	return aggregated;
}

void to_json(nlohmann::json& j, const PluginCondition& condition)
{
	condition.AsJSON(j);
}

void to_json(nlohmann::json& j, const FormListCondition& condition)
{
	condition.AsJSON(j);
}

void to_json(nlohmann::json& j, const FormsCondition& condition)
{
	condition.AsJSON(j);
}

void to_json(nlohmann::json& j, const KeywordCondition& condition)
{
	condition.AsJSON(j);
}

void to_json(nlohmann::json& j, const SignatureCondition& condition)
{
	condition.AsJSON(j);
}

void to_json(nlohmann::json& j, const ScopeCondition& condition)
{
	condition.AsJSON(j);
}

void to_json(nlohmann::json& j, const ConditionTree& condition)
{
	condition.AsJSON(j);
}

ConditionMatcher::ConditionMatcher(const RE::TESForm* form) : m_form(form), m_scope(INIFile::SecondaryType::NONE2)
{
}

ConditionMatcher::ConditionMatcher(const RE::TESForm* form, const INIFile::SecondaryType scope) : m_form(form), m_scope(scope)
{
}

// accumulates scopes seen in ConditionTree during game-data load, to optimize collectible check during periodic REFR scan
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
