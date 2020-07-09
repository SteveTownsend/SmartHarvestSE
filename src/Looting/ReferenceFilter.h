#pragma once

namespace shse
{

class ReferenceFilter
{
public:
	ReferenceFilter(DistanceToTarget& refs, IRangeChecker& rangeCheck, const bool respectDoors, const size_t limit);
	void FindLootableReferences();
	void FindAllCandidates();
	inline double DistanceToDoor() const { return m_nearestDoor; }

private:
	typedef std::function<bool(const RE::TESObjectREFR*)> REFRPredicate;
	void FilterNearbyReferences();
	void RecordCellReferences(const RE::TESObjectCELL* cell);

	// predicates supported
	bool CanLoot(const RE::TESObjectREFR* refr) const;
	bool IsLootCandidate(const RE::TESObjectREFR* refr) const;

	DistanceToTarget& m_refs;
	IRangeChecker& m_rangeCheck;
	const bool m_respectDoors;
	double m_nearestDoor;
	size_t m_limit;
	mutable REFRPredicate m_predicate;
};

}
