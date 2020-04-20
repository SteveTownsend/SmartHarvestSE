#pragma once

#include <vector>

class PlayerCellHelper
{
public:
	PlayerCellHelper(RE::TESObjectCELL* cell, std::vector<RE::TESObjectREFR*>& targets, const double radius) : m_cell(cell), m_targets(targets), m_radius(radius) {}

	void GetReferences();

private:
	bool CanLoot(const RE::TESObjectREFR* refr) const;
	bool WithinLootingRange(const RE::TESObjectREFR* refr) const;
	void GetCellReferences(const RE::TESObjectCELL* cell);
	bool IsAdjacent(RE::TESObjectCELL* cell) const;

	RE::TESObjectCELL* m_cell;
	std::unordered_set<RE::FormID> m_normalRefrs;
	std::vector<RE::TESObjectREFR*>& m_targets;
	const double m_radius;
};
