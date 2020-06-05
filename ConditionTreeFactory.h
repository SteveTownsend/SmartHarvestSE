#pragma once

class Condition {
public:
	virtual ~Condition();

	virtual bool operator()(const RE::TESForm* form, ObjectType objectType) const = 0;
	virtual std::ostream& Print(std::ostream& os) const = 0; 
protected:
	Condition();
};

class PluginCondition : public Condition {
public:
	PluginCondition(const std::string& plugin);
	virtual bool operator()(const RE::TESForm* form, ObjectType objectType) const;
	virtual std::ostream& Print(std::ostream& os) const;

private:
	const std::string m_plugin;
};

class KeywordsCondition : public Condition {
public:
	KeywordsCondition(const std::vector<std::string>& keywords);
	virtual bool operator()(const RE::TESForm* form, ObjectType objectType) const;
	virtual std::ostream& Print(std::ostream& os) const;

private:
	std::unordered_set<const RE::BGSKeyword*> m_keywords;
};

class SignaturesCondition : public Condition {
public:
	SignaturesCondition(const std::vector<std::string>& signatures);
	virtual bool operator()(const RE::TESForm* form, ObjectType objectType) const;
	virtual std::ostream& Print(std::ostream& os) const;

private:
	std::vector<RE::FormType> m_formTypes;
};

class LootCategoriesCondition : public Condition {
public:
	LootCategoriesCondition(const std::vector<std::string>& lootCategories);
	virtual bool operator()(const RE::TESForm* form, ObjectType objectType) const;
	virtual std::ostream& Print(std::ostream& os) const;

private:
	std::vector<ObjectType> m_categories;
};

class ConditionTree : public Condition {
public:
	enum class Operator {
		And,
		Or
	};

	ConditionTree(const Operator op, const unsigned int nestingLevel);
	virtual bool operator()(const RE::TESForm* form, ObjectType objectType) const;
	void AddCondition(std::unique_ptr<Condition> condition);
	virtual std::ostream& Print(std::ostream& os) const;

private:
	std::vector<std::unique_ptr<Condition>> m_conditions;
	Operator m_operator;
	unsigned int m_nestingLevel;
};

class ConditionTreeFactory {
public:
	static ConditionTreeFactory& Instance();
	std::unique_ptr<Condition> ParseTree(const nlohmann::json& rootFilter, const unsigned int nestingLevel);
private:
	std::unique_ptr<Condition> ParsePlugin(const nlohmann::json& pluginRule);
	std::unique_ptr<Condition> ParseKeywords(const nlohmann::json& keywordRule);
	std::unique_ptr<Condition> ParseSignatures(const nlohmann::json& signatureRule);
	std::unique_ptr<Condition> ParseLootCategories(const nlohmann::json& lootCategoryRule);

	std::unique_ptr<ConditionTreeFactory> m_factory;
};

std::ostream& operator<<(std::ostream& os, const Condition& condition)
{
	return condition.Print(os);
}
