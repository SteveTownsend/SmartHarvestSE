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

#include "WorldState/LocationTracker.h"

namespace shse
{

class VisitedPlace {
public:
	VisitedPlace(const RE::TESWorldSpace* worldspace, const RE::BGSLocation* location, const RE::FormID cellID, const Position position, const float gameTime);
	inline float GameTime() const { return m_gameTime; }
	static void ResetSagaState();
	std::string AsString() const;

	void AsJSON(nlohmann::json& j) const;

	inline const RE::TESWorldSpace* Worldspace() const { return m_worldspace; }
	inline const RE::BGSLocation* Location() const { return m_location; }
	inline const RE::FormID CellID() const { return m_cellID; }
	inline const Position GetPosition() const { return m_position; }

private:
	bool operator==(const VisitedPlace& rhs) const;
	const RE::TESWorldSpace* m_worldspace;
	const RE::BGSLocation* m_location;
	RE::FormID m_cellID;
	Position m_position;
	float m_gameTime;
	static VisitedPlace m_lastPlace;
	static const RE::TESWorldSpace* m_lastWorld;
	static const RE::BGSLocation* m_lastLocation;
};

void to_json(nlohmann::json& j, const VisitedPlace& visitedPlace);

class VisitedPlaces
{
public:
	static VisitedPlaces& Instance();
	VisitedPlaces();

	void Reset();
	void RecordVisit(const RE::TESWorldSpace* worldspace, const RE::BGSLocation* location, const RE::FormID cellID,
		const Position& position, const float gameTime);

	void AsJSON(nlohmann::json& j) const;
	void UpdateFrom(const nlohmann::json& j);

	bool IsKnown(const RE::BGSLocation*) const;

private:
	static std::unique_ptr<VisitedPlaces> m_instance;
	std::vector<VisitedPlace> m_visited;
	std::unordered_set<const RE::BGSLocation*> m_knownLocations;
	mutable RecursiveLock m_visitedLock;
};

void to_json(nlohmann::json& j, const VisitedPlaces& visitedPlaces);

}
