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
#include "WorldState/PositionData.h"

namespace shse
{

enum class AdventureTargetType : uint32_t {
	AnimalDen = 0,
	AshSpawnLair,
	AyleidRuin,
	BanditCamp,
	Cave,
	Clearable,
	DragonLair,
	DragonPriestLair,
	DraugrCrypt,
	Dungeon,
	DwarvenRuin,
#if _DEBUG
	FakeForTesting,
#endif
	FalmerHive,
	ForswornCamp,
	Fort,
	GiantCamp,
	GoblinDen,
	Grove,
	HagravenNest,
	Mine,
	NordicRuin,
	OgreDen,
	OrcStronghold,
	RieklingCamp,
	RuinedFort,
	Settlement,
	Shipwreck,
	VampireLair,
	WerebeastLair,
	WarlockLair,
	WitchmanLair,
	MAX
};

std::string AdventureTargetName(const AdventureTargetType adventureTarget);
inline std::string AdventureTargetNameByIndex(const size_t index) {
	return AdventureTargetName(AdventureTargetType(std::min(index, size_t(AdventureTargetType::MAX))));
}

enum class AdventureEventType : uint32_t {
	Started = 0,
	Complete,
	Abandoned
};

class AdventureEvent {
public:
	static AdventureEvent StartedAdventure(const RE::TESWorldSpace* world, const RE::BGSLocation* location, const float gameTime);
	static AdventureEvent CompletedAdventure(const float gameTime);
	static AdventureEvent AbandonedAdventure(const float gameTime);

	static AdventureEvent StartAdventure(const RE::TESWorldSpace* world, const RE::BGSLocation* location);
	static AdventureEvent CompleteAdventure();
	static AdventureEvent AbandonAdventure();

	void AsJSON(nlohmann::json& j) const;

private:
	AdventureEvent(const AdventureEventType eventType, const RE::TESWorldSpace* world, const RE::BGSLocation* location, const float gameTime);
	AdventureEvent(const AdventureEventType eventType, const RE::TESWorldSpace* world, const RE::BGSLocation* location);
	AdventureEvent(const AdventureEventType eventType, const float gameTime);
	AdventureEvent(const AdventureEventType eventType);

	const AdventureEventType m_eventType;
	const RE::TESWorldSpace* m_world;
	const RE::BGSLocation* m_location;
	const float m_gameTime;
};

void to_json(nlohmann::json& j, const AdventureEvent& adventureEvent);

class AdventureTargets
{
public:
	static AdventureTargets& Instance();
	AdventureTargets();

	void Reset();
	void Categorize();

	size_t AvailableAdventureTypes() const;
	// use mapping list to convert MCM index to true index
	std::string AdventureTypeName(const size_t adventureType) const;
	size_t ViableWorldCount(const size_t adventureType) const;
	std::string ViableWorldNameByIndexInView(const size_t worldIndex) const;
	void SelectCurrentDestination(const size_t worldIndex);
	void CheckReachedCurrentDestination(const RE::BGSLocation* newLocation);
	void AbandonCurrentDestination();
	const RE::TESWorldSpace* TargetWorld(void) const;
	const RE::BGSLocation* TargetLocation(void) const;
	Position TargetPosition(void) const;
	bool HasActiveTarget(void) const;
	std::unordered_map<const RE::BGSLocation*, Position> GetWorldMarkedPlaces(const RE::TESWorldSpace* world) const;
	void AsJSON(nlohmann::json& j) const;
	void UpdateFrom(const nlohmann::json& j);

private:
	void LinkLocationToWorld(const RE::BGSLocation* location, const RE::TESWorldSpace* world) const;
	Position GetInteriorCellPosition(const RE::TESObjectCELL* cell, const RE::BGSLocation* location) const;
	Position GetRefHandlePosition(const RE::ObjectRefHandle handle, const RE::BGSLocation* location) const;
	RE::TESWorldSpace* GetRefHandleWorld(const RE::ObjectRefHandle handle) const;
	Position GetRefIDPosition(const RE::FormID refID, const RE::BGSLocation* location) const;
	Position GetRefrPosition(const RE::TESObjectREFR* refr, const RE::BGSLocation* location) const;

	static std::unique_ptr<AdventureTargets> m_instance;
	std::array<std::unordered_set<const RE::BGSLocation*>, int(AdventureTargetType::MAX)> m_locationsByType;
	mutable std::vector<AdventureTargetType> m_validAdventureTypes;

	mutable std::unordered_map<const RE::BGSLocation*, const RE::TESWorldSpace*> m_worldByLocation;
	std::unordered_map<const RE::BGSLocation*, Position> m_locationCoordinates;

	std::unordered_map<const RE::TESWorldSpace*, std::unordered_set<const RE::BGSLocation*>> m_markedLocationsByWorld;
	mutable std::unordered_map<const RE::TESWorldSpace*, std::unordered_set<const RE::BGSLocation*>> m_unvisitedLocationsByWorld;
	mutable std::vector<const RE::TESWorldSpace*> m_sortedWorlds;

	std::vector<AdventureEvent> m_adventureEvents;

	const RE::BGSLocation* m_targetLocation;
	const RE::TESWorldSpace* m_targetWorld;
	mutable RecursiveLock m_adventureLock;
};

void to_json(nlohmann::json& j, const AdventureTargets& visitedPlaces);

}
