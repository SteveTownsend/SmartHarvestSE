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

#include <array>

#include "alglib/alglibmisc.h"
#include "Utilities/utils.h"

namespace shse
{

// x, y, z coordinates
typedef std::array<float, 3> Position;
typedef std::array<double, 3> AlglibPosition;

class RelativeLocationDescriptor
{
public:
	RelativeLocationDescriptor(const AlglibPosition startPoint, const AlglibPosition endPoint, const RE::FormID locationID, const double unitsAway) :
		m_startPoint(startPoint), m_endPoint(endPoint), m_locationID(locationID), m_unitsAway(unitsAway)
	{}
	inline AlglibPosition StartPoint() const { return m_startPoint; }
	inline AlglibPosition EndPoint() const { return m_endPoint; }
	inline RE::FormID LocationID() const { return m_locationID; }
	inline double UnitsAway() const { return m_unitsAway; }
	static RelativeLocationDescriptor Invalid() { return RelativeLocationDescriptor({ 0.,0.,0. }, { 0.,0.,0. }, 0, 0.0); }
	inline bool operator==(const RelativeLocationDescriptor& rhs) {
		return m_startPoint == rhs.m_startPoint && m_endPoint == rhs.m_endPoint;
	}

private:
	const AlglibPosition m_startPoint;
	const AlglibPosition m_endPoint;
	const RE::FormID m_locationID;	// represents end point
	const double m_unitsAway;
};

class LocationTracker
{
private:
	void RecordAdjacentCells();
	void RecordMarkedPlaces();

	bool IsAdjacent(RE::TESObjectCELL* cell) const;
	bool IsPlayerInBlacklistedPlace(const RE::TESObjectCELL* cell) const;
	CompassDirection DirectionToDestinationFromStart(const AlglibPosition& start, const AlglibPosition& destination) const;
	const RE::TESWorldSpace* ParentWorld(const RE::TESObjectCELL* cell);
	RelativeLocationDescriptor NearestMapMarker(const AlglibPosition& refPos) const;
	inline double UnitsToMiles(const double units) const
	{
		return units * DistanceUnitInMiles;
	}
	CellOwnership GetCellOwnership(const RE::TESObjectCELL* cell) const;
	RE::TESForm* GetCellOwner(const RE::TESObjectCELL* cell) const;

	static std::unique_ptr<LocationTracker> m_instance;
	std::vector<RE::TESObjectCELL*> m_adjacentCells;
	const RE::TESObjectCELL* m_priorCell;
	RE::TESObjectCELL* m_playerCell;
	bool m_tellPlayerIfCanLootAfterLoad;
	const RE::BGSLocation* m_playerLocation;
	const RE::TESWorldSpace* m_playerParentWorld;

	std::unordered_map<const RE::BGSLocation*, Position> m_markedPlaces;
	alglib::kdtree m_markers;
	mutable RecursiveLock m_locationLock;

public:
	static LocationTracker& Instance();
	LocationTracker();

	void Reset();
	bool Refresh();
	bool IsPlayerAtHome() const;
	bool IsPlayerInLootablePlace(const RE::TESObjectCELL* cell, const bool lootableIfRestricted);
	decltype(m_adjacentCells) AdjacentCells() const;
	bool IsPlayerIndoors() const;
	bool IsPlayerInRestrictedLootSettlement(const RE::TESObjectCELL* cell) const;
	bool IsPlayerInFriendlyCell() const;
	const RE::TESForm* CurrentPlayerPlace() const;
	bool IsPlayerInWhitelistedPlace(const RE::TESObjectCELL* cell) const;
	void DisplayLocationRelativeToMapMarker() const;
	void PrintPlayerLocation(const RE::BGSLocation* location) const;
	void PrintNearbyLocation(const RE::BGSLocation* location, const double milesAway, const CompassDirection heading) const;
	std::string ParentLocationName(const RE::BGSLocation* location) const;
	std::string Proximity(const double milesAway, CompassDirection heading) const;
	std::string ConversationalDistance(const double milesAway) const;

	const RE::TESObjectCELL* PlayerCell() const;
};

}
