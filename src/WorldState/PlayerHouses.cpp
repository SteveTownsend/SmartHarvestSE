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
#include "PrecompiledHeaders.h"
#include "WorldState/PlayerHouses.h"
#include "Data/LoadOrder.h"

namespace shse
{

std::unique_ptr<PlayerHouses> PlayerHouses::m_instance;

PlayerHouses& PlayerHouses::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<PlayerHouses>();
	}
	return *m_instance;
}

PlayerHouses::PlayerHouses()
{
}

void PlayerHouses::AddLocationKeyword(RE::BGSKeyword* keyword)
{
	m_locationKeywords.insert(keyword);
}

void PlayerHouses::SetCell(const RE::TESObjectCELL* houseCell)
{
	m_validHouseCells.insert(houseCell->GetFormID());
}

void PlayerHouses::Clear()
{
	RecursiveLockGuard guard(m_housesLock);
	m_houses.clear();
	m_houseCells.clear();
}

bool PlayerHouses::Add(const RE::BGSLocation* location)
{
	RecursiveLockGuard guard(m_housesLock);
	return location && m_houses.insert(location->GetFormID()).second;
}

bool PlayerHouses::AddCell(const RE::FormID cellID)
{
	RecursiveLockGuard guard(m_housesLock);
	return cellID != InvalidForm && m_houseCells.insert(cellID).second;
}

// Check indeterminate status of the location, because a requested UI check is pending
bool PlayerHouses::Contains(const RE::BGSLocation* location) const
{
	RecursiveLockGuard guard(m_housesLock);
	return location && m_houses.contains(location->GetFormID());
}

// Check indeterminate status of the location, because a requested UI check is pending
bool PlayerHouses::ContainsCell(const RE::FormID cellID) const
{
	RecursiveLockGuard guard(m_housesLock);
	return m_houseCells.contains(cellID);
}

bool PlayerHouses::IsValidHouseLocation(const RE::BGSLocation* location) const
{
	if (!location)
		return false;
	RecursiveLockGuard guard(m_housesLock);
	for (auto keyword : m_locationKeywords)
	{
		if (location->HasKeyword(keyword))
			return true;
	}
	return false;
}

bool PlayerHouses::IsValidHouseCell(const RE::TESObjectCELL* cell) const
{
	constexpr RE::FormID PlayerFormId(0x7);
	constexpr RE::FormID PlayerFactionFormId(0xdb1);
	RecursiveLockGuard guard(m_housesLock);
	if (cell)
	{
		// CELL Ownership may change so recheck each time
		auto owner(const_cast<RE::TESObjectCELL*>(cell)->GetOwner());
		if (owner)
		{
			RE::FormID ownerID(owner->GetFormID());
			if (ownerID == PlayerFormId || ownerID == PlayerFactionFormId)
			{
				return true;
			}
		}
		// special cases
		return m_validHouseCells.contains(cell->GetFormID());
	}
	return false;
}

}