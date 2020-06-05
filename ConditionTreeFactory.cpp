#include "PrecompiledHeaders.h"

Condition::Condition() {}
Condition::~Condition() {}

PluginCondition::PluginCondition(const std::string& plugin) : m_plugin(plugin)
{
}

bool PluginCondition::operator()(const RE::TESForm* form, ObjectType objectType) const
{
	// TODO efficiently check if form is in this plugin
	// brute force it for now
	return false;
}

std::ostream& PluginCondition::Print(std::ostream& os) const
{
	os << "Plugin: " << m_plugin;
	return os;
}

// This is O(n) in KYWD record count but only happens during startup, and there are not THAT many of them
KeywordsCondition::KeywordsCondition(const std::vector<std::string>& keywords)
{
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	if (!dhnd)
		return;

	// store keywords to match for this collection. Schema enforces uniqueness and validity in input list.
	std::vector<std::string> keywordsLeft(keywords);
	for (const RE::TESForm* form : dhnd->GetFormArray(RE::FormType::Keyword))
	{
		const RE::BGSKeyword* keywordRecord(form->As<RE::BGSKeyword>());
		auto matched(std::find_if(keywordsLeft.cbegin(), keywordsLeft.cend(),
			[&](const std::string& keyword) -> bool { return keywordRecord->GetName() == keyword; }));
		if (matched != keywords.cend())
		{
			keywordsLeft.erase(matched);
			m_keywords.insert(keywordRecord);
#if _DEBUG
			_MESSAGE("BGSKeyword recorded for %s", keywordRecord->GetName());
#endif
			// If all candidates have been located, we are done
			if (keywordsLeft.empty())
				break;
		}
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

std::ostream& KeywordsCondition::Print(std::ostream& os) const
{
	for (const auto keyword : m_keywords)
	{
		os << "Keyword: " << keyword->GetName() << '\n';
	}
	return os;
}

// This is O(n) in TESV Record Type count but only happens during startup, and there are not many of them
SignaturesCondition::SignaturesCondition(const std::vector<std::string>& signatures)
{
	// Store Form Types to match for this collection. Schema enforces uniqueness and validity in input list.
	// The list below must match the JSON schema and CommonLibSSE RE::FormType.
	std::unordered_map<std::string, RE::FormType> validSignatures = {
		{"ALCH", RE::FormType::AlchemyItem},
		{"BOOK", RE::FormType::Book},
		{"ARMO", RE::FormType::Armor},
		{"KEYM", RE::FormType::KeyMaster},
		{"MISC", RE::FormType::Misc},
		{"WEAP", RE::FormType::Weapon}
	};
	for (const auto & signature: signatures)
	{
		auto matched(validSignatures.find(signature));
		if (matched != validSignatures.cend())
		{
			m_formTypes.push_back(matched->second);
#if _DEBUG
			_MESSAGE("Record Signature %s mapped to FormType %d", signature, matched->second);
#endif
		}
	}
}

bool SignaturesCondition::operator()(const RE::TESForm* form, ObjectType objectType) const
{
	// short linear scan
	return std::find(m_formTypes.cbegin(), m_formTypes.cend(), form->GetFormType()) != m_formTypes.cend();
}

std::ostream& SignaturesCondition::Print(std::ostream& os) const
{
	for (const auto formType : m_formTypes)
	{
		os << "Form Type: " << static_cast<int>(formType) << '\n';
	}
	return os;
}

// This is O(n) in Loot Categories count but only happens during startup, and there are not many of them
LootCategoriesCondition::LootCategoriesCondition(const std::vector<std::string>& lootCategories)
{
	// Store Form Types to match for this collection. Schema enforces uniqueness and validity in input list.
	// The list below must match the JSON schema and ObjectType.
	std::unordered_map<std::string, ObjectType> validCategories = {
		{"book", ObjectType::book},
		{"critter", ObjectType::critter},
		{"skillbook", ObjectType::skillbook},
		{"spellbook", ObjectType::spellbook}
	};
	for (const auto& category : lootCategories)
	{
		auto matched(validCategories.find(category));
		if (matched != validCategories.cend())
		{
			m_categories.push_back(matched->second);
#if _DEBUG
			_MESSAGE("Loot Category %s mapped to %s", category, GetObjectTypeName(matched->second));
#endif
		}
	}
}

bool LootCategoriesCondition::operator()(const RE::TESForm* form, ObjectType objectType) const
{
	// short linear scan
	return std::find(m_categories.cbegin(), m_categories.cend(), objectType) != m_categories.cend();
}

std::ostream& LootCategoriesCondition::Print(std::ostream& os) const
{
	for (const auto category : m_categories)
	{
		os << "Loot Category: " << GetObjectTypeName(category) << '\n';
	}
	return os;
}

ConditionTree::ConditionTree(const Operator op, const unsigned int nestingLevel) : m_operator(op), m_nestingLevel(nestingLevel)
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

std::ostream& ConditionTree::Print(std::ostream& os) const
{
	// TODO print this thing
	return os;
}

static std::unique_ptr<ConditionTreeFactory> m_instance;

ConditionTreeFactory& ConditionTreeFactory::Instance()
{
	if (!m_instance)
	{
		m_instance.reset(new ConditionTreeFactory);
	}
	return *m_instance;
}

std::unique_ptr<Condition> ConditionTreeFactory::ParseTree(const nlohmann::json& rootFilter, const unsigned int nestingLevel)
{
	ConditionTree::Operator op;
	if (rootFilter["operator"] == "AND")
	{
		op = ConditionTree::Operator::And;
	}
	else
	{
		op = ConditionTree::Operator::Or;
	}
	std::unique_ptr<ConditionTree> root(std::make_unique<ConditionTree>(op, nestingLevel));
	for (const auto& condition : rootFilter["conditions"].items())
	{
		if (condition.key() == "subFilters")
		{
			for (const auto& subFilter : condition.value())
			{
				root->AddCondition(ParseTree(subFilter, nestingLevel+1));
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
		else if (condition.key() == "signatures")
		{
			root->AddCondition(ParseSignatures(condition.value()));
		}
		else if (condition.key() == "lootCategories")
		{
			root->AddCondition(ParseLootCategories(condition.value()));
		}
	}

	return root;
}

std::unique_ptr<Condition> ConditionTreeFactory::ParsePlugin(const nlohmann::json& pluginRule)
{
	return std::make_unique<PluginCondition>(pluginRule);
}

std::unique_ptr<Condition> ConditionTreeFactory::ParseKeywords(const nlohmann::json& keywordRule)
{
	std::vector<std::string> keywords(keywordRule.begin(), keywordRule.end());
	return std::make_unique<KeywordsCondition>(keywords);
}

std::unique_ptr<Condition> ConditionTreeFactory::ParseSignatures(const nlohmann::json& signatureRule)
{
	std::vector<std::string> keywords(signatureRule.begin(), signatureRule.end());
	return std::make_unique<SignaturesCondition>(signatureRule);
}

std::unique_ptr<Condition> ConditionTreeFactory::ParseLootCategories(const nlohmann::json& lootCategoryRule)
{
	std::vector<std::string> keywords(lootCategoryRule.begin(), lootCategoryRule.end());
	return std::make_unique<LootCategoriesCondition>(lootCategoryRule);
}
