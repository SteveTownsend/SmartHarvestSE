#include "PrecompiledHeaders.h"

Condition::Condition() {}
Condition::~Condition() {}

PluginCondition::PluginCondition(const std::string& plugin) : m_plugin(plugin)
{
	m_formIDMask = LoadOrder::Instance().GetFormIDMask(plugin);
	if (m_formIDMask == InvalidForm)
	{
		std::ostringstream err;
		err << "Unknown plugin: " << plugin;
		throw PluginError(err.str().c_str());
	}
}

bool PluginCondition::operator()(const RE::TESForm* form, ObjectType objectType) const
{
	// TODO efficiently check if form is in this plugin
	return false;
}

nlohmann::json PluginCondition::MakeJSON() const
{
	return nlohmann::json(*this);
}

void to_json(nlohmann::json& j, const PluginCondition& condition)
{
	j["plugin"] = condition.m_plugin;
}

// This is O(n) in KYWD record count but only happens during startup, and there are not THAT many of them
KeywordsCondition::KeywordsCondition(const std::vector<std::string>& keywords)
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

bool KeywordsCondition::operator()(const RE::TESForm* form, ObjectType objectType) const
{
	const RE::BGSKeywordForm* keywordHolder(form->As<RE::BGSKeywordForm>());
	if (!keywordHolder)
		return false;
	return std::find_if(m_keywords.cbegin(), m_keywords.cend(),
		[=](const RE::BGSKeyword* keyword) -> bool { return keywordHolder->HasKeyword(keyword); }) != m_keywords.cend();
}

nlohmann::json KeywordsCondition::MakeJSON() const
{
	return nlohmann::json(*this);
}

void to_json(nlohmann::json& j, const KeywordsCondition& condition)
{
	j["keywords"] = nlohmann::json::array();
	for (const auto keyword : condition.m_keywords)
	{
		j["keywords"].push_back(FormUtils::SafeGetFormEditorID(keyword));
	}
}

// This is O(n) in TESV Record Type count but only happens during startup, and there are not many of them
SignaturesCondition::SignaturesCondition(const std::vector<std::string>& signatures)
{
	// Store Form Types to match for this collection. Schema enforces uniqueness and validity in input list.
	// The list below must match the JSON schema and CommonLibSSE RE::FormType.
	std::unordered_map<std::string, RE::FormType> validSignatures = {
		{"ALCH", RE::FormType::AlchemyItem},
		{"ARMO", RE::FormType::Armor},
		{"BOOK", RE::FormType::Book},
		{"INGR", RE::FormType::Ingredient},
		{"KEYM", RE::FormType::KeyMaster},
		{"MISC", RE::FormType::Misc},
		{"SLGM", RE::FormType::SoulGem},
		{"WEAP", RE::FormType::Weapon}
	};
	for (const auto& signature : signatures)
	{
		auto matched(validSignatures.find(signature));
		if (matched != validSignatures.cend())
		{
			m_formTypes.push_back(matched->second);
			DBG_VMESSAGE("Record Signature %s mapped to FormType %d", signature.c_str(), static_cast<int>(matched->second));
		}
	}
}

bool SignaturesCondition::operator()(const RE::TESForm* form, ObjectType objectType) const
{
	// short linear scan
	return std::find(m_formTypes.cbegin(), m_formTypes.cend(), form->GetFormType()) != m_formTypes.cend();
}

nlohmann::json SignaturesCondition::MakeJSON() const
{
	return nlohmann::json(*this);
}

void to_json(nlohmann::json& j, const SignaturesCondition& condition)
{
	j["signatures"] = nlohmann::json::array();
	for (const auto formType : condition.m_formTypes)
	{
		j["signatures"].push_back(static_cast<int>(formType));
	}
}

// This is O(n) in Loot Categories count but only happens during startup, and there are not many of them
LootCategoriesCondition::LootCategoriesCondition(const std::vector<std::string>& lootCategories)
{
	// Store Form Types to match for this collection. Schema enforces uniqueness and validity in input list.
	// The list below must match the JSON schema and ObjectType.
	std::unordered_map<std::string, ObjectType> validCategories = {
		{"book", ObjectType::book},
		{"clutter", ObjectType::clutter},
		{"critter", ObjectType::critter},
		{"drink", ObjectType::drink},
		{"food", ObjectType::food},
		{"ingredient", ObjectType::ingredient},
		{"key", ObjectType::key},
		{"skillbook", ObjectType::skillbook},
		{"spellbook", ObjectType::spellbook}
	};

	for (const auto& category : lootCategories)
	{
		auto matched(validCategories.find(category));
		if (matched != validCategories.cend())
		{
			m_categories.push_back(matched->second);
			DBG_VMESSAGE("Loot Category %s mapped to %s", category.c_str(), GetObjectTypeName(matched->second).c_str());
		}
	}
}

bool LootCategoriesCondition::operator()(const RE::TESForm* form, ObjectType objectType) const
{
	// short linear scan
	return std::find(m_categories.cbegin(), m_categories.cend(), objectType) != m_categories.cend();
}

nlohmann::json LootCategoriesCondition::MakeJSON() const
{
	return nlohmann::json(*this);
}

void to_json(nlohmann::json& j, const LootCategoriesCondition& condition)
{
	j["lootCategories"] = nlohmann::json::array();
	for (const auto category : condition.m_categories)
	{
		j["lootCategories"].push_back(GetObjectTypeName(category));
	}
}

ConditionTree::ConditionTree(const Operator op, const unsigned int depth) : m_operator(op), m_depth(depth)
{
}

bool ConditionTree::operator()(const RE::TESForm* form, ObjectType objectType) const
{
	for (const auto& condition : m_conditions)
	{
		bool checkThis(condition->operator()(form, objectType));
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

void to_json(nlohmann::json& j, const ConditionTree& condition)
{
	j["operator"] = std::string(condition.m_operator == ConditionTree::Operator::And ? "AND" : "OR");
	j["conditions"] = nlohmann::json::array();
	for (const auto& condition : condition.m_conditions)
	{
		j["conditions"].push_back(condition->MakeJSON());
	}
}

std::ostream& operator<<(std::ostream& os, const Condition& condition)
{
	os << condition.MakeJSON().dump(2);
	return os;
}
