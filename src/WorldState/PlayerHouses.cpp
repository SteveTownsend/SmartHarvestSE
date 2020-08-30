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
#include "PlayerHouses.h"

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
	m_validHouseCells.insert(houseCell);
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
	return location && m_houses.insert(location).second;
}

bool PlayerHouses::AddCell(const RE::TESObjectCELL* cell)
{
	RecursiveLockGuard guard(m_housesLock);
	return cell && m_houseCells.insert(cell).second;
}

// Check indeterminate status of the location, because a requested UI check is pending
bool PlayerHouses::Contains(const RE::BGSLocation* location) const
{
	RecursiveLockGuard guard(m_housesLock);
	return location && m_houses.contains(location);
}

// Check indeterminate status of the location, because a requested UI check is pending
bool PlayerHouses::ContainsCell(const RE::TESObjectCELL* cell) const
{
	RecursiveLockGuard guard(m_housesLock);
	return cell && m_houseCells.contains(cell);
}

bool PlayerHouses::IsValidHouse(const RE::BGSLocation* location) const
{
	RecursiveLockGuard guard(m_housesLock);
	return location && location->HasKeyword(m_keyword);
}

bool PlayerHouses::IsValidHouseCell(const RE::TESObjectCELL* cell) const
{
	RecursiveLockGuard guard(m_housesLock);
	return cell && m_validHouseCells.contains(cell);
}
