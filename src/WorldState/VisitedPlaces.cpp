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

VisitedPlace VisitedPlace::m_lastPlace(nullptr, nullptr, nullptr, InvalidPosition, 0.0);
const RE::TESWorldSpace* VisitedPlace::m_lastWorld(nullptr);
const RE::BGSLocation* VisitedPlace::m_lastLocation(nullptr);

void VisitedPlace::ResetSagaState()
{
	m_lastPlace = VisitedPlace(nullptr, nullptr, nullptr, InvalidPosition, 0.0);
	m_lastWorld = nullptr;
	m_lastLocation = nullptr;
}

VisitedPlace::VisitedPlace(const RE::TESWorldSpace* worldspace, const RE::BGSLocation* location, const RE::TESObjectCELL* cell, const Position position, const float gameTime) :
	m_worldspace(worldspace), m_location(location), m_cell(cell), m_position(position), m_gameTime(gameTime)
{
}

bool VisitedPlace::operator==(const VisitedPlace& rhs) const
{
	return m_worldspace == rhs.m_worldspace && m_location == rhs.m_location && m_cell == rhs.m_cell;
}

std::string VisitedPlace::AsString() const
{
	// skip redundant entries
	if (m_lastPlace == *this)
		return "";
	std::ostringstream stream;
	bool wrote(false);
	if (m_location != m_lastLocation)
	{
		std::string departed;
		if (m_lastLocation)
		{
			stream << "I left " << m_lastLocation->GetName();
			departed = m_lastLocation->GetName();
			m_lastLocation = nullptr;
			wrote = true;
		}
		if (m_location)
		{
			std::string newLocation(m_location->GetName());
			if (newLocation != departed)
			{
				if (departed.empty())
				{
					stream << "I ";
				}
				else
				{
					stream << " and ";
				}
				stream << "entered " << m_location->GetName();
				m_lastLocation = m_location;
				wrote = true;
			}
		}
	}
	else if (!m_location)
	{
		// event between locations: print position info relative to nearby Location
		static const bool historic(true);
		std::string locationStr(LocationTracker::Instance().LocationRelativeToNearestMapMarker(
			AlglibPosition({ m_position[0], m_position[1], m_position[2] }), true));
		if (locationStr.empty())
		{
			stream << "I was exploring";
		}
		else
		{
			stream << locationStr;
		}
		wrote = true;
	}
	// only output WorldSpace for the first event in scope
	if (m_worldspace != m_lastWorld)
	{
		if (m_worldspace)
		{
			stream << " in " << m_worldspace->GetName();
			wrote = true;
		}
		m_lastWorld = m_worldspace;
	}
	if (wrote)
	{
		stream << '.';
		return stream.str();
	}
	else
	{
		return "";
	}
	m_lastPlace = *this;
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