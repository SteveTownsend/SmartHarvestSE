#pragma once

#include <vector>

class PlayerCellHelper
{
public:
	static PlayerCellHelper& GetInstance() { return m_instance; }
	std::vector<RE::TESObjectREFR*> GetReferences(RE::TESObjectCELL* cell, const double radius);
	PlayerCellHelper() : m_cell(nullptr), m_radius(0.) {}

private:
	bool CanLoot(RE::TESObjectREFR* refr) const;
	bool WithinLootingRange(const RE::TESObjectREFR* refr) const;
	void GetCellReferences(const RE::TESObjectCELL* cell);
	void GetAdjacentCells(RE::TESObjectCELL* cell);
	bool IsAdjacent(RE::TESObjectCELL* cell) const;

	RE::TESObjectCELL* m_cell;
	std::vector<RE::TESObjectREFR*> m_targets;
	double m_radius;
	unsigned int m_eliminated;
	static PlayerCellHelper m_instance;
	static std::vector<RE::TESObjectCELL*> m_adjacentCells;
};
