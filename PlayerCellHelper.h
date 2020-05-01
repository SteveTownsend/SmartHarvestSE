#pragma once

#include <vector>

class PlayerCellHelper
{
public:
	static PlayerCellHelper& GetInstance() { return m_instance; }
	void GetReferences(RE::TESObjectCELL* cell, std::vector<RE::TESObjectREFR*>* targets, const double radius);
	PlayerCellHelper() : m_cell(nullptr), m_radius(0.), m_targets(nullptr) {}

private:
	bool CanLoot(const RE::TESObjectREFR* refr) const;
	bool WithinLootingRange(const RE::TESObjectREFR* refr) const;
	void GetCellReferences(const RE::TESObjectCELL* cell);
	void GetAdjacentCells(RE::TESObjectCELL* cell);
	bool IsAdjacent(RE::TESObjectCELL* cell) const;

	RE::TESObjectCELL* m_cell;
	std::unordered_set<RE::FormID> m_normalRefrs;
	std::vector<RE::TESObjectREFR*>* m_targets;
	double m_radius;
	static PlayerCellHelper m_instance;
	static std::vector<RE::TESObjectCELL*> m_adjacentCells;
};
