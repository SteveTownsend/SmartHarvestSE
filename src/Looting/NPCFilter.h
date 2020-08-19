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

namespace shse
{
class OrderedFilter {
public:
	OrderedFilter(const nlohmann::json& j);
	bool DeterminesLootability(const RE::TESNPC* npc, bool& isLootable) const;
	inline size_t Priority() const { return m_priority; }
private:
	std::unordered_set<const RE::BGSKeyword*> m_excludeKeywords;
	std::unordered_set<const RE::TESRace*> m_excludeRaces;
	std::unordered_set<const RE::TESFaction*> m_excludeFactions;
	std::unordered_set<const RE::BGSKeyword*> m_includeKeywords;
	std::unordered_set<const RE::TESRace*> m_includeRaces;
	std::unordered_set<const RE::TESFaction*> m_includeFactions;
	size_t m_priority;
};

struct OrderedFilterCompare {
bool operator() (const OrderedFilter* lhs, const OrderedFilter* rhs) const {
	return lhs->Priority() < rhs->Priority();
}
};

class NPCFilter {
public:
	static NPCFilter& Instance();
	NPCFilter();
	void Load();
	bool IsLootable(const RE::TESNPC* npc) const;

private:

	// no lock as all public functions are const once loaded
	static std::unique_ptr<NPCFilter> m_instance;

	bool m_active;
	bool m_logResults;
	bool m_defaultLoot;
	bool m_excludePlayerRace;
	std::set<const OrderedFilter*, OrderedFilterCompare> m_orderedFilters;
};

}
