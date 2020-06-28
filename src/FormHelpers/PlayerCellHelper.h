#pragma once

#include "Utilities/BoundedList.h"
#include "Looting/IRangeChecker.h"

class PlayerCellHelper
{
public:
	PlayerCellHelper(BoundedList<RE::TESObjectREFR*>& refs, const IRangeChecker& rangeCheck);
	void FindLootableReferences() const;
	void FindAllCandidates() const;

private:
	typedef std::function<bool(const RE::TESObjectREFR*)> REFRPredicate;
	void FilterNearbyReferences() const;
	bool FilterCellReferences(const RE::TESObjectCELL* cell) const;

	// predicates supported
	bool CanLoot(const RE::TESObjectREFR* refr) const;
	bool IsLootCandidate(const RE::TESObjectREFR* refr) const;

	double m_radius;
	mutable unsigned int m_eliminated;
	BoundedList<RE::TESObjectREFR*>& m_refs;
	mutable REFRPredicate m_predicate;
	const IRangeChecker& m_rangeCheck;
};
