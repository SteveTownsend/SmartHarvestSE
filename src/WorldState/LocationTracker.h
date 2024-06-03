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

#include "alglib/alglibmisc.h"
#include "Utilities/utils.h"
#include "WorldState/PositionData.h"

namespace shse
{

class LocationTracker
{
private:
	void RecordAdjacentCells(const RE::TESObjectCELL* current);
	void RecordMarkedPlaces();

	bool IsAdjacent(RE::TESObjectCELL* cell) const;
	bool IsPlaceBlacklisted(const RE::FormID cellID, const RE::BGSLocation* location) const;
	void PlayerLocationRelativeToNearestMapMarker(const RE::BGSLocation* locationDone) const;
	const RE::BGSLocation* PlayerLocationRelativeToAdventureTarget(void) const;
	CompassDirection DirectionToDestinationFromStart(const AlglibPosition& start, const AlglibPosition& destination) const;
	const RE::TESWorldSpace* ParentWorld(const RE::TESObjectCELL* cell);
	RelativeLocationDescriptor NearestMapMarker(const AlglibPosition& refPos) const;
	RelativeLocationDescriptor MarkedLocationPosition(
		const Position targetPosition, const RE::BGSLocation* location, const AlglibPosition& refPos) const;
	inline double UnitsToMiles(const double units) const
	{
		return units * DistanceUnitInMiles;
	}
	CellOwnership GetCellOwnership(const RE::TESObjectCELL* cell) const;
	RE::TESForm* GetCellOwner(const RE::TESObjectCELL* cell) const;
	std::string PlaceName(const RE::TESForm*) const;
	bool IsPlacePlayerHome(const RE::FormID cellID, const RE::BGSLocation* location) const;
	bool IsPlaceLootable(const RE::FormID cellID, const RE::BGSLocation* location, const bool lootableIfRestricted, const bool allowIfRestrictedHome);
	bool IsPlaceWhitelisted(const RE::FormID cellID, const RE::BGSLocation* location) const;
	bool IsPlaceRestrictedLootSettlement(const RE::FormID cellID, const RE::BGSLocation* location) const;
	const RE::TESForm* CurrentPlayerPlaceCached() const;

	static std::unique_ptr<LocationTracker> m_instance;
	// 3x3 CELL adjacency check - 8 nearest CELLs are treated as adjacent to player's CELL, if exterior
	std::array<RE::TESObjectCELL*, 8> m_adjacentCells;
	RE::FormID m_playerCellID;
	std::int32_t m_playerCellX;
	std::int32_t m_playerCellY;
	bool m_playerIndoors;
	std::string m_playerPlaceName;
	bool m_tellPlayerIfCanLootAfterLoad;
	const RE::BGSLocation* m_playerLocation;
	const RE::TESWorldSpace* m_playerParentWorld;

	std::unordered_map<const RE::BGSLocation*, Position> m_markedPlaces;
	alglib::kdtree m_markers;
	mutable RecursiveLock m_locationLock;
	mutable std::atomic<bool> m_aiRunning;

	static constexpr double OnlyYards = 0.1;
	static constexpr double LittleWay = 0.3;
	static constexpr double HalfMile = 0.75;
	static constexpr double MileOrSo = 1.5;
	static constexpr double CoupleOfMiles = 3.0;
	static constexpr double SeveralMiles = 100.0;

public:
	static LocationTracker& Instance();
	LocationTracker();

	void Reset();
	bool Refresh();
	bool IsPlayerAtHome() const;
	void RecordCurrentPlace(const float gameTime);
	bool IsPlayerInLootablePlace(const bool lootableIfRestricted, const bool lootableIfRestrictedHome);
	decltype(m_adjacentCells) AdjacentCells() const;
	bool IsPlayerIndoors() const;
	bool IsPlayerInRestrictedLootSettlement() const;
	bool IsPlayerInFriendlyCell() const;
	const RE::TESForm* CurrentPlayerPlace();
	const RE::TESWorldSpace* CurrentPlayerWorld() const;
	bool IsPlayerInWhitelistedPlace() const;

	void DisplayPlayerLocation(void) const;
	void PrintPlayerLocation(const RE::BGSLocation* location) const;
	std::string NearbyLocationAsString(
		const RE::BGSLocation* location, const double milesAway, const CompassDirection heading, const bool historic) const;
	std::string LocationRelativeToNearestMapMarker(const AlglibPosition& position, const bool historic) const;
	void PrintAdventureTargetInfo(const RE::BGSLocation* location, const double milesAway, CompassDirection heading) const;
	void PrintDifferentWorld(const RE::TESWorldSpace* world) const;

	std::string ParentLocationName(const RE::BGSLocation* location) const;
	std::string Proximity(const double milesAway, CompassDirection heading) const;
	std::string ConversationalDistance(const double milesAway) const;
	std::string PlayerExactLocation() const;

	const RE::TESObjectCELL* PlayerCell() const;
};

}
