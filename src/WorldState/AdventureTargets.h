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
	DwarvenRuin,
	Dungeon,
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
	static AdventureEvent StartAdventure(const RE::TESWorldSpace* world, const RE::BGSLocation* location);
	static AdventureEvent CompleteAdventure();
	static AdventureEvent AbandonAdventure();

	void AsJSON(nlohmann::json& j) const;

private:
	AdventureEvent(const AdventureEventType eventType, const RE::TESWorldSpace* world, const RE::BGSLocation* location);
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
	inline std::string AdventureTypeName(const size_t adventureType) const { return AdventureTargetNameByIndex(size_t(m_validAdventureTypes[adventureType])); }
	size_t ViableWorldCount(const size_t adventureType) const;
	std::string ViableWorldNameByIndexInView(const size_t worldIndex) const;
	void SelectCurrentDestination(const size_t worldIndex);
	void CheckReachedCurrentDestination(const RE::BGSLocation* newLocation);
	void AbandonCurrentDestination();
	const RE::TESWorldSpace* TargetWorld(void) const;
	const RE::BGSLocation* TargetLocation(void) const;
	RE::ObjectRefHandle TargetMapMarker(void) const;
	bool HasActiveTarget(void) const;

	void AsJSON(nlohmann::json& j) const;
	void UpdateFrom(const nlohmann::json& j);

private:
	static std::unique_ptr<AdventureTargets> m_instance;
	std::array<std::unordered_set<RE::BGSLocation*>, int(AdventureTargetType::MAX)> m_locationsByType;
	mutable std::vector<AdventureTargetType> m_validAdventureTypes;

	std::unordered_map<const RE::BGSLocation*, RE::TESWorldSpace*> m_worldByLocation;
	std::unordered_map<const RE::BGSLocation*, RE::ObjectRefHandle> m_mapMarkerByLocation;

	mutable std::unordered_map<const RE::TESWorldSpace*, std::unordered_set<const RE::BGSLocation*>> m_unvisitedLocationsByWorld;
	mutable std::vector<const RE::TESWorldSpace*> m_sortedWorlds;

	std::vector<AdventureEvent> m_adventureEvents;

	const RE::BGSLocation* m_targetLocation;
	const RE::TESWorldSpace* m_targetWorld;
	mutable RecursiveLock m_adventureLock;
};

void to_json(nlohmann::json& j, const AdventureTargets& visitedPlaces);

}
