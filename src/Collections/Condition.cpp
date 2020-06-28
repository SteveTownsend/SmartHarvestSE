#include "PrecompiledHeaders.h"

#include "Data/LoadOrder.h"
#include "Collections/Condition.h"
#include "Utilities/Exception.h"
#include "Utilities/utils.h"

namespace shse
{

Condition::Condition() {}
Condition::~Condition() {}

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
		if (formIDMask == InvalidForm)
		{
			std::ostringstream err;
			err << "Unknown plugin: " << plugin;
			throw PluginError(err.str().c_str());
		}
		m_formIDMaskByPlugin.insert(std::make_pair(plugin, formIDMask));
	}
}

bool PluginCondition::operator()(const RE::TESForm* form) const
{
	if (!form)
		return false;
	for (const auto plugin : m_formIDMaskByPlugin)
	{
		if (LoadOrder::Instance().ModOwnsForm(plugin.first, form->GetFormID()))
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

FormListCondition::FormListCondition(const std::string& plugin, const std::string& formListID)
{
	// schema enforces 8-char HEX format
	RE::FormID formID;
	std::stringstream ss;
	ss << std::hex << formListID;
	ss >> formID;
	RE::BGSListForm* formList(RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSListForm>(PluginUtils::AsRaw(formID), plugin));
	if (!formList)
	{
		REL_ERROR("Collection Condition requires a FormList 0x%08x", formID);
		return;
	}
	DBG_VMESSAGE("Resolved FormList 0x%08x", formID);
	m_formList = formList;
	FlattenMembers(m_formList);
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
			DBG_VMESSAGE("FormList Member found %s/0x%08x", candidate->GetName(), candidate->GetFormID());
			m_listMembers.insert(candidate);
		}
	}
}

bool FormListCondition::operator()(const RE::TESForm* form) const
{
	return m_listMembers.contains(form);
}

nlohmann::json FormListCondition::MakeJSON() const
{
	return nlohmann::json(*this);
}

void FormListCondition::AsJSON(nlohmann::json& j) const
{
	j["formList"] = nlohmann::json();
	j["listPlugin"] = m_plugin;
	std::ostringstream formStr;
	formStr << std::hex << std::setfill('0') << std::setw(8) << m_formList->GetFormID();
	j["formID"] = formStr.str();
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
			DBG_VMESSAGE("BGSKeyword recorded for %s", FormUtils::SafeGetFormEditorID(keywordRecord).c_str());
			// eliminate the matched candidate input from JSON
			keywordsLeft.erase(matched);

			// If all candidates have been located, we are done
			if (keywordsLeft.empty())
				break;
		}
	}
	for (const std::string& badKeyword : keywordsLeft)
	{
		DBG_WARNING("Collection has invalid KYWD %s", badKeyword.c_str());
	}
	if (!keywordsLeft.empty())
	{
		std::ostringstream err;
		err << "Unknown KYWD: " << keywordsLeft.front();
		throw KeywordError(err.str().c_str());
	}
}

bool KeywordCondition::operator()(const RE::TESForm* form) const
{
	const RE::BGSKeywordForm* keywordHolder(form->As<RE::BGSKeywordForm>());
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

// This is O(n) in TESV Record Type count but only happens during startup, and there are not many of them
SignatureCondition::SignatureCondition(const std::vector<std::string>& signatures)
{
	for (const auto& signature : signatures)
	{
		auto matched(m_validSignatures.find(signature));
		if (matched != m_validSignatures.cend())
		{
			m_formTypes.push_back(matched->second);
			DBG_VMESSAGE("Record Signature %s mapped to FormType %d", signature.c_str(), static_cast<int>(matched->second));
		}
	}
}

bool SignatureCondition::operator()(const RE::TESForm* form) const
{
	// short linear scan
	return std::find(m_formTypes.cbegin(), m_formTypes.cend(), form->GetFormType()) != m_formTypes.cend();
}

const decltype(SignatureCondition::m_validSignatures) SignatureCondition::ValidSignatures()
{
	return m_validSignatures;
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
		j["signature"].push_back(static_cast<int>(formType));
	}
}

ConditionTree::ConditionTree(const Operator op, const unsigned int depth) : m_operator(op), m_depth(depth)
{
}

bool ConditionTree::operator()(const RE::TESForm* form) const
{
	for (const auto& condition : m_conditions)
	{
		bool checkThis(condition->operator()(form));
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

void to_json(nlohmann::json& j, const PluginCondition& condition)
{
	condition.AsJSON(j);
}

void to_json(nlohmann::json& j, const FormListCondition& condition)
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

void to_json(nlohmann::json& j, const ConditionTree& condition)
{
	condition.AsJSON(j);
}

}

std::ostream& operator<<(std::ostream& os, const shse::Condition& condition)
{
	os << condition.MakeJSON().dump(2);
	return os;
}
