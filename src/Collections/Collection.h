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

#include <tuple>

#include "Looting/ScanGovernor.h"
#include "Collections/Condition.h"
#include "WorldState/PlayerState.h"

namespace shse {

class CollectionGroup;

class CollectionPolicy {
public:
	CollectionPolicy(const CollectibleHandling action, const bool notify, const bool repeat) :
		m_action(action), m_notify(notify), m_repeat(repeat)
	{}

	inline CollectibleHandling Action() const { return m_action; }
	inline bool Notify() const { return m_notify; }
	inline bool Repeat() const { return m_repeat; }
	inline void SetAction(const CollectibleHandling action) { m_action = action; }
	inline void SetNotify(const bool notify) { m_notify = notify; }
	inline void SetRepeat(const bool repeat) { m_repeat = repeat; }

	void AsJSON(nlohmann::json& j) const;

	inline bool operator==(const CollectionPolicy& rhs) const {
		return m_action == rhs.m_action && m_notify == rhs.m_notify && m_repeat == rhs.m_repeat;
	}

private:
	CollectibleHandling m_action;
	bool m_notify;
	bool m_repeat;
};

void to_json(nlohmann::json& j, const CollectionPolicy& collection);

class Collection {
protected:
	size_t PlacedMembers(void) const;
	bool AddMemberID(const RE::TESForm* form) const;

	// inputs
	std::string m_name;
	std::string m_description;
	// may inherit Policy from Group or override
	CollectionPolicy m_effectivePolicy;
	bool m_overridesGroup;
	std::unique_ptr<ConditionTree> m_rootFilter;
	// derived
	std::unordered_map<const RE::TESForm*, float> m_observed;
	mutable std::unordered_set<const RE::TESForm*> m_members;
	std::vector<INIFile::SecondaryType> m_scopes;
	const CollectionGroup* m_owningGroup;

public:
	Collection(const CollectionGroup* owningGroup, const std::string& name, const std::string& description,
		const CollectionPolicy& policy,	const bool overridesGroup, std::unique_ptr<ConditionTree> filter);
	bool IsActive() const;
	bool MatchesFilter(const ConditionMatcher& matcher) const;
	virtual bool IsMemberOf(const RE::TESForm* form) const;
	bool InScopeAndCollectibleFor(const ConditionMatcher& matcher) const;
	inline const CollectionPolicy& Policy() const { return m_effectivePolicy; }
	inline CollectionPolicy& Policy() { return m_effectivePolicy; }
	inline void SetPolicy(const CollectionPolicy& policy) { m_effectivePolicy = policy; }
	inline bool OverridesGroup() const { return m_overridesGroup; }
	inline void SetOverridesGroup(const bool overridesGroup) { m_overridesGroup = overridesGroup; }
	inline size_t Count() { return m_members.size(); }
	inline size_t Observed() { return m_observed.size(); }
	void RecordItem(const RE::TESForm* form, const float gameTime);
	void Reset();

	nlohmann::json MakeJSON() const;
	void AsJSON(nlohmann::json& j) const;
	void UpdateFrom(const nlohmann::json& collectionState, const CollectionPolicy& defaultPolicy);
	void SetScopesFrom(const nlohmann::json& scopes);
	void SetMembersFrom(const nlohmann::json& members);

	std::string Name(void) const;
	std::string Description(void) const;
	std::string PrintDefinition(void) const;
	std::string PrintMembers(void) const;
	inline void SetScopes(const std::vector<INIFile::SecondaryType>& scopes) { m_scopes = scopes; }
	inline decltype(m_members) Members() const { return m_members; }
};

void to_json(nlohmann::json& j, const Collection& collection);

class CollectionGroup {
public:
	CollectionGroup(const std::string& name, const CollectionPolicy& policy, const bool useMCM, const nlohmann::json& collections);
	inline const std::vector<std::shared_ptr<Collection>>& Collections() const { return m_collections; }
	std::shared_ptr<Collection> CollectionByName(const std::string& collectionName) const;
	inline std::string Name() const { return m_name; }
	inline bool UseMCM() const { return m_useMCM; }

	void AsJSON(nlohmann::json& j) const;
	void UpdateFrom(const nlohmann::json& group);

	inline const CollectionPolicy& Policy() const { return m_policy; }
	inline CollectionPolicy& Policy() { return m_policy; }
	void SyncDefaultPolicy();

private:
	std::string m_name;
	CollectionPolicy m_policy;
	const bool m_useMCM;
	std::vector<std::shared_ptr<Collection>> m_collections;
};

void to_json(nlohmann::json& j, const CollectionGroup& collectionGroup);

}

std::ostream& operator<<(std::ostream& os, const shse::Collection& collection);
