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
#include "WorldState/PlayerState.h"
#include "WorldState/Saga.h"
#include "Data/LoadOrder.h"

namespace shse
{

VisitedPlace::VisitedPlace(const RE::TESWorldSpace* worldspace, const RE::BGSLocation* location, const RE::TESObjectCELL* cell, const Position position, const float gameTime) :
	m_worldspace(worldspace), m_location(location), m_cell(cell), m_position(position), m_gameTime(gameTime)
{
}

std::string VisitedPlace::AsString() const
{
	std::ostringstream stream;
	stream << "I entered";
	if (m_location)
	{
		stream << ' ' << m_location->GetName();
	}
	else if (m_cell)
	{
		stream << " an unknown location";
	}
	if (m_worldspace)
	{
		stream << " in " << m_worldspace->GetName();
	}

	return stream.str();
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

void VisitedPlaces::RecordVisit(const RE::TESWorldSpace* worldspace, const RE::BGSLocation* location, const RE::TESObjectCELL* cell,
	const Position& position, const float gameTime)
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
		m_visited.emplace_back(worldspace, location, cell, position, gameTime);
		Saga::Instance().AddEvent(m_visited.back());
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
		// the list was ordered by game time before saving - player position recorded
		RecordVisit(worldspaceForm, locationForm, cellForm, Position(place["position"]), gameTime);
		if (locationForm)
		{
			m_knownLocations.insert(locationForm);
		}
	}
}

bool VisitedPlaces::IsKnown(const RE::BGSLocation* location) const
{
	RecursiveLockGuard guard(m_visitedLock);
	return m_knownLocations.contains(location);
}

void to_json(nlohmann::json& j, const VisitedPlaces& visitedPlaces)
{
	visitedPlaces.AsJSON(j);
}

}