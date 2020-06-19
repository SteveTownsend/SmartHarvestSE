#pragma once

namespace shse {

	class Condition{
	public:
		virtual ~Condition();

		virtual bool operator()(const RE::TESForm* form, ObjectType objectType) const = 0;
		virtual nlohmann::json MakeJSON() const = 0;
		virtual void AsJSON(nlohmann::json& j) const = 0;
	protected:
		Condition();
	};

	class PluginCondition : public Condition {
	public:
		PluginCondition(const std::string& plugin);
		virtual bool operator()(const RE::TESForm* form, ObjectType objectType) const;
		virtual nlohmann::json MakeJSON() const;
		virtual void AsJSON(nlohmann::json& j) const override;

	private:
		const std::string m_plugin;
		RE::FormID m_formIDMask;
	};

	class KeywordsCondition : public Condition {
	public:
		KeywordsCondition(const std::vector<std::string>& keywords);
		virtual bool operator()(const RE::TESForm* form, ObjectType objectType) const;
		virtual nlohmann::json MakeJSON() const;
		virtual void AsJSON(nlohmann::json& j) const override;

	private:
		std::unordered_set<const RE::BGSKeyword*> m_keywords;
	};

	class SignaturesCondition : public Condition {
	public:
		SignaturesCondition(const std::vector<std::string>& signatures);
		virtual bool operator()(const RE::TESForm* form, ObjectType objectType) const;
		virtual nlohmann::json MakeJSON() const;
		virtual void AsJSON(nlohmann::json& j) const override;

	private:
		std::vector<RE::FormType> m_formTypes;
	};

	class LootCategoriesCondition : public Condition {
	public:
		LootCategoriesCondition(const std::vector<std::string>& lootCategories);
		virtual bool operator()(const RE::TESForm* form, ObjectType objectType) const;
		virtual nlohmann::json MakeJSON() const;
		virtual void AsJSON(nlohmann::json& j) const override;

	private:
		std::vector<ObjectType> m_categories;
	};

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
		virtual void AsJSON(nlohmann::json& j) const override;

	private:
		std::vector<std::unique_ptr<Condition>> m_conditions;
		Operator m_operator;
		unsigned int m_depth;
	};

	void to_json(nlohmann::json& j, const PluginCondition& p);
	void to_json(nlohmann::json& j, const KeywordsCondition& p);
	void to_json(nlohmann::json& j, const SignaturesCondition& p);
	void to_json(nlohmann::json& j, const LootCategoriesCondition& p);
	void to_json(nlohmann::json& j, const ConditionTree& p);
}


std::ostream& operator<<(std::ostream& os, const shse::Condition& condition);
