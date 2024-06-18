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

class ReferenceFilter
{
public:
	ReferenceFilter(DistanceToTarget& refs, IRangeChecker& rangeCheck, const bool respectDoors, const size_t limit);
	static Lootability CheckLootable(const RE::TESObjectREFR* refr);
	void FindLootableReferences();
	void FindAllCandidates();
	void FindActors();

private:
	typedef std::function<bool(const RE::TESObjectREFR*)> REFRPredicate;
	void FilterNearbyReferences();
	void RecordReference(RE::TESObjectREFR* refr);
	void RecordCellReferences(RE::TESObjectCELL* cell, const bool indoors);

	Lootability AnalyzeREFR(const RE::TESObjectREFR* refr, const bool dryRun) const;
	// predicates supported
	bool CanLoot(const RE::TESObjectREFR* refr) const;
	bool IsLootCandidate(const RE::TESObjectREFR* refr) const;
	bool IsFollowerOrDead(const RE::TESObjectREFR* refr) const;

	DistanceToTarget& m_refs;
	IRangeChecker& m_rangeCheck;
	const bool m_respectDoors;
	double m_nearestDoor;
	size_t m_limit;
	mutable REFRPredicate m_predicate;
};

}
