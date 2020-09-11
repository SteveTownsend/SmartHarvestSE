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

class PlayerHouses
{
public:
	static PlayerHouses& Instance();
	PlayerHouses();

	void Clear();
	bool Add(const RE::BGSLocation* location);
	bool AddCell(const RE::FormID cellID);
	bool Contains(const RE::BGSLocation* location) const;
	bool ContainsCell(const RE::FormID cellID) const;

	void SetKeyword(RE::BGSKeyword* keyword);
	void SetCell(const RE::TESObjectCELL* houseCell);
	bool IsValidHouse(const RE::BGSLocation* location) const;
	bool IsValidHouseCell(const RE::FormID cellID) const;

private:
	static std::unique_ptr<PlayerHouses> m_instance;
	RE::BGSKeyword* m_keyword;

	// LCTN forms
	std::unordered_set<RE::FormID> m_houses;
	// CELL forms
	std::unordered_set<RE::FormID> m_houseCells;
	// CELLS that are effectively player house but not in a properly-tagged LCTN
	std::unordered_set<RE::FormID> m_validHouseCells;
	mutable RecursiveLock m_housesLock;
};

}
