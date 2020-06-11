#pragma once

class Condition {
public:
	virtual ~Condition();

	virtual bool operator()(const RE::TESForm* form, ObjectType objectType) const = 0;
	virtual nlohmann::json MakeJSON() const = 0;
protected:
	Condition();
};

class PluginCondition : public Condition {
public:
	PluginCondition(const std::string& plugin);
	virtual bool operator()(const RE::TESForm* form, ObjectType objectType) const;
	virtual nlohmann::json MakeJSON() const;

private:
	friend void to_json(nlohmann::json& j, const PluginCondition& p);

	const std::string m_plugin;
	RE::FormID m_formIDMask;
};

void to_json(nlohmann::json& j, const PluginCondition& p);

class KeywordsCondition : public Condition {
public:
	KeywordsCondition(const std::vector<std::string>& keywords);
	virtual bool operator()(const RE::TESForm* form, ObjectType objectType) const;
	virtual nlohmann::json MakeJSON() const;

private:
	friend void to_json(nlohmann::json& j, const KeywordsCondition& p);

	std::unordered_set<const RE::BGSKeyword*> m_keywords;
};

void to_json(nlohmann::json& j, const KeywordsCondition& p);

class SignaturesCondition : public Condition {
public:
	SignaturesCondition(const std::vector<std::string>& signatures);
	virtual bool operator()(const RE::TESForm* form, ObjectType objectType) const;
	virtual nlohmann::json MakeJSON() const;

private:
	friend void to_json(nlohmann::json& j, const SignaturesCondition& p);

	std::vector<RE::FormType> m_formTypes;
};

void to_json(nlohmann::json& j, const SignaturesCondition& p);

class LootCategoriesCondition : public Condition {
public:
	LootCategoriesCondition(const std::vector<std::string>& lootCategories);
	virtual bool operator()(const RE::TESForm* form, ObjectType objectType) const;
	virtual nlohmann::json MakeJSON() const;

private:
	friend void to_json(nlohmann::json& j, const LootCategoriesCondition& p);

	std::vector<ObjectType> m_categories;
};

void to_json(nlohmann::json& j, const LootCategoriesCondition& p);

class ConditionTree : public Condition {
public:
	enum class Operator {
		And,
		Or
	};

	ConditionTree(const Operator op, const unsigned int depth);
	virtual bool operator()(const RE::TESForm* form, ObjectType objectType) const;
	void AddCondition(std::unique_ptr<Condition> condition);
	virtual nlohmann::json MakeJSON() const;

private:
	friend void to_json(nlohmann::json& j, const ConditionTree& p);

	std::vector<std::unique_ptr<Condition>> m_conditions;
	Operator m_operator;
	unsigned int m_depth;
};

void to_json(nlohmann::json& j, const ConditionTree& p);

std::ostream& operator<<(std::ostream& os, const Condition& condition);
