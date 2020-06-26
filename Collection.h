#pragma once

#include <tuple>
#include "tasks.h"

namespace shse {

class CollectionEntry {
public:
	CollectionEntry(const RE::TESForm* form, const float gameTime, const RE::TESForm* place) : m_form(form), m_gameTime(gameTime), m_place(place)
	{
	}

private:
	const RE::TESForm* m_form;
	const float m_gameTime;
	const RE::TESForm* m_place;
};

class CollectionPolicy {
public:
	CollectionPolicy(const SpecialObjectHandling action, const bool notify, const bool repeat) :
		m_action(action), m_notify(notify), m_repeat(repeat)
	{}

	inline SpecialObjectHandling Action() const { return m_action; }
	inline bool Notify() const { return m_notify; }
	inline bool Repeat() const { return m_repeat; }

	void AsJSON(nlohmann::json& j) const;

private:
	SpecialObjectHandling m_action;
	bool m_notify;
	bool m_repeat;
};

void to_json(nlohmann::json& j, const CollectionPolicy& collection);

class Collection {
public:
	Collection(const std::string& name, const std::string& description, const CollectionPolicy& policy, std::unique_ptr<ConditionTree> filter);
	bool MatchesFilter(const RE::TESForm* form) const;
	virtual bool IsMemberOf(const RE::TESForm* form) const;
	bool IsCollectibleFor(const RE::TESForm* form) const;
	bool AddMemberID(const RE::TESForm* form) const;
	inline const CollectionPolicy& Policy() const { return m_policy; }
	void RecordItem(const RE::FormID itemID, const RE::TESForm* form, const float gameTime, const RE::TESForm* place);
	void Reset();
	nlohmann::json MakeJSON() const;
	void AsJSON(nlohmann::json& j) const;
	std::string Name(void) const;
	std::string PrintDefinition(void) const;
	std::string PrintMembers(void) const;

protected:
	// inputs
	std::string m_name;
	std::string m_description;
	CollectionPolicy m_policy;
	std::unique_ptr<ConditionTree> m_rootFilter;
	// derived
	std::unordered_map<RE::FormID, CollectionEntry> m_observed;
	mutable std::unordered_set<RE::FormID> m_members;
};

void to_json(nlohmann::json& j, const Collection& collection);

}

std::ostream& operator<<(std::ostream& os, const shse::Collection& collection);
