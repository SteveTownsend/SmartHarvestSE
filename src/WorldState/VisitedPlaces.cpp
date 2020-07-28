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

#include "WorldState/VisitedPlaces.h"

namespace shse
{

VisitedPlace::VisitedPlace(const RE::TESWorldSpace* worldspace, const RE::BGSLocation* location, const RE::TESObjectCELL* cell, const float gameTime) :
	m_worldspace(worldspace), m_location(location), m_cell(cell), m_gameTime(gameTime)
{
}

VisitedPlace& VisitedPlace::operator=(const VisitedPlace& rhs)
{
	if (this != &rhs)
	{
		m_worldspace = rhs.m_worldspace;
		m_location = rhs.m_location;
		m_cell = rhs.m_cell;
	}
	return *this;
}

void VisitedPlace::AsJSON(nlohmann::json& j) const
{
	j["time"] = m_gameTime;
	if (m_worldspace)
	{
		j["worldspace"] = m_worldspace->GetFormID();
	}
	if (m_location)
	{
		j["location"] = m_location->GetFormID();
	}
	if (m_cell)
	{
		j["cell"] = m_cell->GetFormID();
	}
}

void to_json(nlohmann::json& j, const VisitedPlace& visitedPlace)
{
	visitedPlace.AsJSON(j);
}

std::unique_ptr<VisitedPlaces> VisitedPlaces::m_instance;

VisitedPlaces& VisitedPlaces::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<VisitedPlaces>();
	}
	return *m_instance;
}

VisitedPlaces::VisitedPlaces()
{
}

void VisitedPlaces::Reset()
{
	RecursiveLockGuard guard(m_visitedLock);
	m_visited.clear();
}

void VisitedPlaces::RecordNew(const RE::TESWorldSpace* worldspace, const RE::BGSLocation* location, const RE::TESObjectCELL* cell, const float gameTime)
{
	bool isNew(false);
	RecursiveLockGuard guard(m_visitedLock);
	if (m_visited.empty())
	{
		isNew = true;
	}
	else
	{
		const VisitedPlace& currentPlace(m_visited.back());
		isNew = worldspace != currentPlace.Worldspace() || location != currentPlace.Location() || cell != currentPlace.Cell();
	}
	if (isNew)
	{ 
		m_visited.emplace_back(worldspace, location, cell, gameTime);
	}
}

void VisitedPlaces::AsJSON(nlohmann::json& j) const
{
	RecursiveLockGuard guard(m_visitedLock);
	j["visited"] = nlohmann::json::array();
	for (const auto& visited : m_visited)
	{
		j["visited"].push_back(visited);
	}
}

void to_json(nlohmann::json& j, const VisitedPlaces& visitedPlaces)
{
	visitedPlaces.AsJSON(j);
}

}