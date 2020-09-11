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

PlayerHouses::PlayerHouses() : m_keyword(nullptr)
{
}

void PlayerHouses::SetKeyword(RE::BGSKeyword* keyword)
{
	m_keyword = keyword;
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

bool PlayerHouses::IsValidHouse(const RE::BGSLocation* location) const
{
	RecursiveLockGuard guard(m_housesLock);
	return location && location->HasKeyword(m_keyword);
}

bool PlayerHouses::IsValidHouseCell(const RE::FormID cellID) const
{
	RecursiveLockGuard guard(m_housesLock);
	return m_validHouseCells.contains(cellID);
}

}