#pragma once

#include "Looting/IRangeChecker.h"

namespace shse
{

class PlayerCellHelper
{
public:
	PlayerCellHelper(DistanceToTarget& refs, const IRangeChecker& rangeCheck);
	void FindLootableReferences() const;
	void FindAllCandidates() const;
	inline double DistanceToDoor() const { return m_nearestDoor; }

private:
	typedef std::function<bool(const RE::TESObjectREFR*)> REFRPredicate;
	void FilterNearbyReferences() const;
	void FilterCellReferences(const RE::TESObjectCELL* cell) const;

	// predicates supported
	bool CanLoot(const RE::TESObjectREFR* refr) const;
	bool IsLootCandidate(const RE::TESObjectREFR* refr) const;

	double m_radius;
	mutable unsigned int m_eliminated;
	DistanceToTarget& m_refs;
	mutable REFRPredicate m_predicate;
	const IRangeChecker& m_rangeCheck;
	mutable double m_nearestDoor;
};

}
