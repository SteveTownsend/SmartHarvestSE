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

#include "Data/iniSettings.h"

namespace shse {

	class ConditionMatcher;

	bool CanBeCollected(RE::TESForm* form);

	class Condition {
	public:
		virtual ~Condition();

		virtual std::unordered_set<const RE::TESForm*> StaticMembers() const;
		virtual bool operator()(const ConditionMatcher& matcher) const = 0;
		nlohmann::json MakeJSON() const;
		virtual void AsJSON(nlohmann::json& j) const = 0;
	protected:
		Condition();
	};

	class PluginCondition : public Condition {
	public:
		PluginCondition(const std::vector<std::string>& plugins);
		virtual bool operator()(const ConditionMatcher& matcher) const;
		virtual void AsJSON(nlohmann::json& j) const override;

	private:
		std::unordered_map<std::string, RE::FormID> m_formIDMaskByPlugin;
	};

	class FormListCondition : public Condition {
	public:
		FormListCondition(const std::vector<std::pair<std::string, std::string>>& pluginFormList);
		virtual bool operator()(const ConditionMatcher& matcher) const;
		virtual void AsJSON(nlohmann::json& j) const override;

	private:
		void FlattenMembers(const RE::BGSListForm* formList);
		std::vector<std::pair<RE::BGSListForm*, std::string>> m_formLists;
		std::unordered_set<const RE::TESForm*> m_listMembers;
	};

	class FormsCondition : public Condition {
	public:
		FormsCondition(const std::vector<std::pair<std::string, std::vector<std::string>>>& pluginForms);
		virtual std::unordered_set<const RE::TESForm*> StaticMembers() const override;
		virtual bool operator()(const ConditionMatcher& matcher) const;
		virtual void AsJSON(nlohmann::json& j) const override;

	private:
		std::unordered_set<const RE::TESForm*> m_allForms;
		std::unordered_map<std::string, std::vector<RE::TESForm*>> m_formsByPlugin;
	};

	class KeywordCondition : public Condition {
	public:
		KeywordCondition(const std::vector<std::string>& keywords);
		virtual bool operator()(const ConditionMatcher& matcher) const;
		virtual void AsJSON(nlohmann::json& j) const override;

	private:
		std::unordered_set<const RE::BGSKeyword*> m_keywords;
	};

	class CategoryCondition : public Condition {
	private:
		std::vector<ObjectType> m_objectTypes;

	public:
		// Store Form Types to match for this collection. Schema enforces uniqueness and validity in input list.
		// The list below must match the JSON schema and CommonLibSSE RE::FormType.

		CategoryCondition(const std::vector<std::string>& categories);
		virtual bool operator()(const ConditionMatcher& matcher) const;
		virtual void AsJSON(nlohmann::json& j) const override;
		inline std::vector<ObjectType> Types() const { return m_objectTypes; }
	};

	class SignatureCondition : public Condition {
	private:
		static const std::unordered_map<std::string, RE::FormType> m_validSignatures;
		static const std::vector<RE::FormType> m_validFormTypes;
		std::vector<RE::FormType> m_formTypes;

	public:
		// Store Form Types to match for this collection. Schema enforces uniqueness and validity in input list.
		// The list below must match the JSON schema and CommonLibSSE RE::FormType.

		SignatureCondition(const std::vector<std::string>& signatures);
		virtual bool operator()(const ConditionMatcher& matcher) const;
		static const decltype(m_validSignatures) ValidSignatures();
		static bool IsValidFormType(const RE::FormType formType);
		virtual void AsJSON(nlohmann::json& j) const override;
		static std::string FormTypeAsSignature(const RE::FormType formType);
	};

	class ScopeCondition : public Condition {
	public:
		ScopeCondition(const std::vector<std::string>& scopes);
		virtual bool operator()(const ConditionMatcher& matcher) const;
		virtual void AsJSON(nlohmann::json& j) const override;
		static std::string SecondaryTypeAsScope(const INIFile::SecondaryType scope);

	private:
		std::vector<INIFile::SecondaryType> m_scopes;
		static const std::unordered_map<std::string, INIFile::SecondaryType> m_validScopes;
	};

	class ItemRule : public Condition {
	public:
		virtual std::vector<ObjectType> GetObjectTypes(void) const = 0;
	};

	class FilterTree : public ItemRule {
	public:
		enum class Operator {
			And,
			Or
		};

		FilterTree(const Operator op, const unsigned int depth);
		virtual bool operator()(const ConditionMatcher& matcher) const;
		virtual void AsJSON(nlohmann::json& j) const override;
		void AddCondition(std::unique_ptr<Condition> condition);
		virtual std::unordered_set<const RE::TESForm*> StaticMembers() const override;
		virtual std::vector<ObjectType> GetObjectTypes(void) const override { return std::vector<ObjectType>(); }

	private:
		Operator m_operator;
		unsigned int m_depth;
		std::vector<std::unique_ptr<Condition>> m_conditions;
	};

	class CategoryRule : public ItemRule {
	public:
		virtual bool operator()(const ConditionMatcher& matcher) const;
		virtual void AsJSON(nlohmann::json& j) const override;
		void SetCondition(std::unique_ptr<CategoryCondition> condition);
		virtual std::vector<ObjectType> GetObjectTypes(void) const override { return m_condition->Types(); }
	private:
		std::unique_ptr<CategoryCondition> m_condition;
	};

	void to_json(nlohmann::json& j, const Condition& p);

	class ConditionMatcher {
	public:
		ConditionMatcher(const RE::TESForm* form);
		ConditionMatcher(const RE::TESForm* form, const INIFile::SecondaryType scope, const ObjectType objectType);
		inline const RE::TESForm* Form() const { return m_form; }
		inline const ObjectType GetObjectType() const { return m_objectType; }
		inline INIFile::SecondaryType Scope() const { return m_scope; }
		void AddScope(const INIFile::SecondaryType scope) const;
		inline const std::vector<INIFile::SecondaryType>& ScopesSeen() const { return m_scopesSeen; }

	private:
		const RE::TESForm* m_form;
		const ObjectType m_objectType;
		// INIFile::SecondaryType::NONE2 is a sentinel to indicate no filtering on scope, during game-data load
		const INIFile::SecondaryType m_scope;
		mutable std::vector<INIFile::SecondaryType> m_scopesSeen;
	};
}

std::ostream& operator<<(std::ostream& os, const shse::Condition& condition);
