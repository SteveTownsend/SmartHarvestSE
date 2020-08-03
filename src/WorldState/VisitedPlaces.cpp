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

#include "WorldState/PlayerState.h"
#include "WorldState/VisitedPlaces.h"
#include "Data/LoadOrder.h"

namespace shse
{

VisitedPlace::VisitedPlace(const RE::TESWorldSpace* worldspace, const RE::BGSLocation* location, const RE::TESObjectCELL* cell, const float gameTime) :
	m_worldspace(worldspace), m_location(location), m_cell(cell), m_position(PlayerState::Instance().GetPosition()), m_gameTime(gameTime)
{
}

VisitedPlace::VisitedPlace(const RE::TESWorldSpace* worldspace, const RE::BGSLocation* location, const RE::TESObjectCELL* cell, const Position position, const float gameTime) :
	m_worldspace(worldspace), m_location(location), m_cell(cell), m_position(position), m_gameTime(gameTime)
{
}

VisitedPlace& VisitedPlace::operator=(const VisitedPlace& rhs)
{
	if (this != &rhs)
	{
		m_worldspace = rhs.m_worldspace;
		m_location = rhs.m_location;
		m_cell = rhs.m_cell;
		m_position = rhs.m_position;
		m_gameTime = rhs.m_gameTime;
	}
	return *this;
}

void VisitedPlace::AsJSON(nlohmann::json& j) const
{
	j["time"] = m_gameTime;
	if (m_worldspace)
	{
		j["worldspace"] = StringUtils::FromFormID(m_worldspace->GetFormID());
	}
	if (m_location)
	{
		j["location"] = StringUtils::FromFormID(m_location->GetFormID());
	}
	if (m_cell)
	{
		j["cell"] = StringUtils::FromFormID(m_cell->GetFormID());
	}
	j["position"] = nlohmann::json(m_position);
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
		if (location)
		{
			m_knownLocations.insert(location);
		}
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

// rehydrate from cosave data
void VisitedPlaces::UpdateFrom(const nlohmann::json& j)
{
	DBG_MESSAGE("Cosave Visited Places\n{}", j.dump(2));
	RecursiveLockGuard guard(m_visitedLock);
	m_visited.clear();
	m_visited.reserve(j["visited"].size());
	for (const nlohmann::json& place : j["visited"])
	{
		const float gameTime(place["time"].get<float>());
		const auto worldspace(place.find("worldspace"));
		const RE::TESWorldSpace* worldspaceForm(worldspace != place.cend() ?
			LoadOrder::Instance().RehydrateCosaveFormAs<RE::TESWorldSpace>(StringUtils::ToFormID(worldspace->get<std::string>())) : nullptr);
		const auto location(place.find("location"));
		const RE::BGSLocation* locationForm(location != place.cend() ?
			LoadOrder::Instance().RehydrateCosaveFormAs<RE::BGSLocation>(StringUtils::ToFormID(location->get<std::string>())) : nullptr);
		const auto cell(place.find("cell"));
		const RE::TESObjectCELL* cellForm(cell != place.cend() ?
			LoadOrder::Instance().RehydrateCosaveFormAs<RE::TESObjectCELL>(StringUtils::ToFormID(cell->get<std::string>())) : nullptr);
		// the list was already normalized before saving, no need to call RecordNew
		// player position recorded
		m_visited.emplace_back(worldspaceForm, locationForm, cellForm, Position(place["position"]), gameTime);
		if (locationForm)
		{
			m_knownLocations.insert(locationForm);
		}
	}
}

std::unordered_set<const RE::BGSLocation*> VisitedPlaces::RemoveVisited(const std::unordered_set<const RE::BGSLocation*>& candidates) const
{
	std::unordered_set<const RE::BGSLocation*> result;
	std::copy_if(candidates.cbegin(), candidates.cend(), std::inserter(result, result.end()),
		[=](const RE::BGSLocation* candidate) -> bool {
		return !m_knownLocations.contains(candidate);
	});
	return result;
}

void to_json(nlohmann::json& j, const VisitedPlaces& visitedPlaces)
{
	visitedPlaces.AsJSON(j);
}

}