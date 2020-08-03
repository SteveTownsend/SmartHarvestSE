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

class AdventureTargets
{
public:
	static AdventureTargets& Instance();
	AdventureTargets() {}

	void Categorize();

private:
	static std::unique_ptr<AdventureTargets> m_instance;
	std::array<std::unordered_set<RE::BGSLocation*>, int(AdventureTargetType::MAX)> m_locationsByType;
	std::unordered_map<const RE::BGSLocation*, RE::TESWorldSpace*> m_worldByLocation;
	mutable RecursiveLock m_adventureLock;
};
