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
#include "WorldState/AdventureTargets.h"
#include "WorldState/PlayerState.h"
#include "WorldState/VisitedPlaces.h"
#include "Data/LoadOrder.h"
#include "Utilities/utils.h"
#include "VM/papyrus.h"

#include <random>

namespace shse
{

std::string AdventureTargetName(const AdventureTargetType adventureTarget)
{
	switch (adventureTarget)
	{
		case AdventureTargetType::AnimalDen:	   return "Animal Den";
		case AdventureTargetType::AshSpawnLair:	   return "Ash Spawn Lair";
		case AdventureTargetType::AyleidRuin:	   return "Ayleid Ruin";
		case AdventureTargetType::BanditCamp:	   return "Bandit Camp";
		case AdventureTargetType::Cave:			   return "Cave";
		case AdventureTargetType::Clearable:	   return "Clearable";
		case AdventureTargetType::DragonLair:	   return "Dragon Lair";
		case AdventureTargetType::DragonPriestLair: return "Dragon Priest Lair";
		case AdventureTargetType::DraugrCrypt:	   return "Draugr Crypt";
		case AdventureTargetType::DwarvenRuin:	   return "Dwarven Ruin";
		case AdventureTargetType::Dungeon:		   return "Dungeon";
		case AdventureTargetType::FalmerHive:	   return "Falmer Hive";
		case AdventureTargetType::ForswornCamp:	   return "Forsworn Camp";
		case AdventureTargetType::Fort:			   return "Fort";
		case AdventureTargetType::GiantCamp:	   return "Giant Camp";
		case AdventureTargetType::GoblinDen:	   return "Goblin Den";
		case AdventureTargetType::Grove:		   return "Grove";
		case AdventureTargetType::HagravenNest:	   return "Hagraven Nest";
		case AdventureTargetType::Mine:			   return "Mine";
		case AdventureTargetType::NordicRuin:	   return "Nordic Ruin";
		case AdventureTargetType::OgreDen:		   return "Ogre Den";
		case AdventureTargetType::OrcStronghold:   return "Orc Stronghold";
		case AdventureTargetType::RieklingCamp:	   return "Riekling Camp";
		case AdventureTargetType::RuinedFort:	   return "Ruined Fort";
		case AdventureTargetType::Settlement:	   return "Settlement";
		case AdventureTargetType::Shipwreck:	   return "Shipwreck";
		case AdventureTargetType::VampireLair:	   return "Vampire Lair";
		case AdventureTargetType::WerebeastLair:   return "Werebeast Lair";
		case AdventureTargetType::WarlockLair:	   return "Warlock Lair";
		case AdventureTargetType::WitchmanLair:	   return "Witchman Lair";
		default: return "";
	};
}

AdventureEvent AdventureEvent::StartAdventure(const RE::TESWorldSpace* world, const RE::BGSLocation* location)
{
	return AdventureEvent(AdventureEventType::Started, world, location);
}

AdventureEvent AdventureEvent::CompleteAdventure()
{
	return AdventureEvent(AdventureEventType::Complete);
}

AdventureEvent AdventureEvent::AbandonAdventure()
{
	return AdventureEvent(AdventureEventType::Abandoned);
}

AdventureEvent::AdventureEvent(const AdventureEventType eventType, const RE::TESWorldSpace* world, const RE::BGSLocation* location) :
	m_eventType(eventType), m_world(world), m_location(location), m_gameTime(PlayerState::Instance().CurrentGameTime())
{
}

AdventureEvent::AdventureEvent(const AdventureEventType eventType) :
	m_eventType(eventType), m_world(nullptr), m_location(nullptr), m_gameTime(PlayerState::Instance().CurrentGameTime())
{
}

void AdventureEvent::AsJSON(nlohmann::json& j) const
{
	j["time"] = m_gameTime;
	j["event"] = int(m_eventType);
	if (m_world)
	{
		j["world"] = StringUtils::FromFormID(m_world->GetFormID());
	}
	if (m_location)
	{
		j["location"] = StringUtils::FromFormID(m_location->GetFormID());
	}
}

void to_json(nlohmann::json& j, const AdventureEvent& adventureEvent)
{
	adventureEvent.AsJSON(j);
}

std::unique_ptr<AdventureTargets> AdventureTargets::m_instance;

AdventureTargets& AdventureTargets::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<AdventureTargets>();
	}
	return *m_instance;
}

AdventureTargets::AdventureTargets() : m_targetLocation(nullptr), m_targetWorld(nullptr)
{
}

void AdventureTargets::Reset()
{
	RecursiveLockGuard guard(m_adventureLock);
	m_adventureEvents.clear();
	m_targetLocation = nullptr;
	m_targetWorld = nullptr;

	m_unvisitedLocationsByWorld.clear();
	m_sortedWorlds.clear();
}

// Classify Locations by their keywords
void AdventureTargets::Categorize()
{
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	if (!dhnd)
		return;

	std::unordered_map<std::string, AdventureTargetType> targetByKeyword =
	{
		// Skyrim.esm
		{"LocSetCave", AdventureTargetType::Cave},
		{"LocSetCaveIce", AdventureTargetType::Cave},
		{"LocSetDwarvenRuin", AdventureTargetType::DwarvenRuin},
		{"LocSetMilitaryFort", AdventureTargetType::Fort},
		{"LocSetNordicRuin", AdventureTargetType::NordicRuin},
		{"LocTypeAnimalDen", AdventureTargetType::AnimalDen},
		{"LocTypeBanditCamp", AdventureTargetType::BanditCamp},
		{"LocTypeCity", AdventureTargetType::Settlement},
		{"LocTypeClearable", AdventureTargetType::Clearable},
		{"LocTypeDragonLair", AdventureTargetType::DragonLair},
		{"LocTypeDragonPriestLair", AdventureTargetType::DragonPriestLair},
		{"LocTypeDraugrCrypt", AdventureTargetType::DraugrCrypt},
		{"LocTypeDungeon", AdventureTargetType::Dungeon},
		{"LocTypeFalmerHive", AdventureTargetType::FalmerHive},
		{"LocTypeForswornCamp", AdventureTargetType::ForswornCamp},
		{"LocTypeGiantCamp", AdventureTargetType::GiantCamp},
		{"LocTypeHagravenNest", AdventureTargetType::HagravenNest},
		{"LocTypeMilitaryFort", AdventureTargetType::Fort},
		{"LocTypeMine", AdventureTargetType::Mine},
		{"LocTypeOrcStronghold", AdventureTargetType::OrcStronghold},
		{"LocTypeSettlement", AdventureTargetType::Settlement},
		{"LocTypeShipwreck", AdventureTargetType::Shipwreck},
		{"LocTypeSprigganGrove", AdventureTargetType::Grove},
		{"LocTypeTown", AdventureTargetType::Settlement},
		{"LocTypeVampireLair", AdventureTargetType::VampireLair},
		{"LocTypeWarlockLair", AdventureTargetType::WarlockLair},
		{"LocTypeWerewolfLair", AdventureTargetType::WerebeastLair},
		// Dragonborn.esm
		{"DLC2LocTypeAshSpawn", AdventureTargetType::AshSpawnLair},
		{"DLC2LocTypeRieklingCamp", AdventureTargetType::RieklingCamp},
		{"LocTypeWerebearLair", AdventureTargetType::WerebeastLair},
		// arnima.esm
		{"LocTypeArnimaOrcs", AdventureTargetType::OrcStronghold},
		{"LocTypeDirenniRuin", AdventureTargetType::AyleidRuin},
		{"LocTypeWitchmanLair", AdventureTargetType::WitchmanLair},
		// BSHeartland.esm
		{"CYRLocTypeAyleidRuin", AdventureTargetType::AyleidRuin},
		{"CYRLocTypeDaedra", AdventureTargetType::Grove},
		{"CYRLocTypeFortRuin", AdventureTargetType::RuinedFort},
		{"CYRLocTypeGoblinDen", AdventureTargetType::GoblinDen},
		{"CYRLocTypeOgreDen", AdventureTargetType::OgreDen},
		{"CYRLocTypeUndead", AdventureTargetType::WarlockLair},
		// Falskaar.esm
		{"FSLocTypeBanditCamp", AdventureTargetType::BanditCamp},
		{"FSLocTypeDungeon", AdventureTargetType::Dungeon},
		{"FSLocTypeFalmerHive", AdventureTargetType::FalmerHive},
		{"FSLocTypeGiantCamp", AdventureTargetType::GiantCamp},
		{"FSLocTypeSprigganGrove", AdventureTargetType::Grove},
		// Midwood Isle.esp
		{"LocSetAyleidRuinMidwoodIsle", AdventureTargetType::AyleidRuin},
		// Vigilant.esm
		{"zzzBMLocVampireDungeon", AdventureTargetType::VampireLair},
	};

	for (RE::BGSLocation* location : dhnd->GetFormArray<RE::BGSLocation>())
	{
		if (!location->GetFullNameLength())
		{
			DBG_MESSAGE("Skip unnamed Location 0x{:08x}", location->GetFormID());
			continue;
		}
		const RE::BGSLocation* current(location);
		RE::ObjectRefHandle markerRefr;
		if (!current->worldLocMarker)
		{
			DBG_MESSAGE("Location has no Map Marker {}/0x{:08x}", location->GetName(), location->GetFormID());
			// check for ACSR/LCSR 
			RE::FormID markerID(InvalidForm);
			for (const auto& csr : location->specialRefs)
			{
				static const RE::FormID MapMarkerLCRT = 0x10f63c;
				static const RE::FormID LocationCenterLCRT = 0x1bdf1;
				// ref-type is a keyword, we want Map Marker (preferred) or Location Center
				if (csr.type->GetFormID() == MapMarkerLCRT)
				{
					markerID = csr.refData.refID;
					break;
				}
				if (csr.type->GetFormID() == LocationCenterLCRT)
				{
					markerID = csr.refData.refID;
				}
			}
			if (markerID == InvalidForm)
			{
				DBG_MESSAGE("Location has no Marker {}/0x{:08x}", location->GetName(), location->GetFormID());
				continue;
			}
			RE::TESObjectREFR* refr(RE::TESForm::LookupByID<RE::TESObjectREFR>(markerID));
			if (!refr)
			{
				DBG_MESSAGE("Location has unresolvable Marker 0x{:08x}", markerID);
				continue;
			}
			markerRefr = refr->GetHandle();
		}
		else
		{
			markerRefr = current->worldLocMarker;
		}
		// Scan location keywords to check if it's a target type
		uint32_t numKeywords(location->GetNumKeywords());
		bool saved(false);
		for (uint32_t next = 0; next < numKeywords; ++next)
		{
			std::optional<RE::BGSKeyword*> keyword(location->GetKeywordAt(next));
			if (!keyword.has_value() || !keyword.value())
				continue;

			std::string keywordName(FormUtils::SafeGetFormEditorID(keyword.value()));
			const auto matched(targetByKeyword.find(keywordName));
			if (matched == targetByKeyword.cend())
				continue;
			m_locationsByType[int(matched->second)].insert(location);
			saved = true;
		}
		if (saved)
		{
			m_mapMarkerByLocation.insert({ location, markerRefr });
		}
	}
	// skip this one
	const RE::FormID AvoidanceExterior = 0x1b44a;
	// hard code this linkage, it's a mess otherwise
	const RE::FormID TamrielLocation = 0x130ff;
	const RE::FormID SkyrimWorldspace = 0x3C;
	// determine Worldspace for each Location - first scan Worldspace Location lists
	for (RE::TESWorldSpace* world : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESWorldSpace>())
	{
		if (world->GetFormID() == AvoidanceExterior || world->GetFullNameLength() == 0)
			continue;
		if (world->GetFormID() == SkyrimWorldspace)
		{
			RE::BGSLocation* tamriel(RE::TESForm::LookupByID<RE::BGSLocation>(TamrielLocation));
			if (tamriel)
			{
				const auto inserted(m_worldByLocation.insert({ tamriel, world }));
				if (inserted.second)
				{
					DBG_MESSAGE("Hard-coded worldspace {}/0x{:08x} for location {}/0x{:08x}",
						world->GetName(), world->GetFormID(), tamriel->GetName(), tamriel->GetFormID());
				}
			}
		}

		for (const auto location : world->locationMap)
		{
			// parent world has locations, OK to proceed
			const auto inserted(m_worldByLocation.insert({ location.second, world }));
			if (inserted.second)
			{
				DBG_MESSAGE("Found worldspace {}/0x{:08x} for location {}/0x{:08x}",
					world->GetName(), world->GetFormID(), location.second->GetName(), location.second->GetFormID());
			}
			else if (inserted.first->second != world)
			{
				DBG_WARNING("Found second worldspace {}/0x{:08x} for location {}/0x{:08x}",
					world->GetName(), world->GetFormID(), location.second->GetName(), location.second->GetFormID());
			}
		}
	}
	// now scan full Location list and match up unprocessed Location with the WorldSpace of its parents
	for (const RE::BGSLocation* target : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::BGSLocation>())
	{
		const RE::BGSLocation* location(target);
		while (location)
		{
			const auto matched(m_worldByLocation.find(location));
			if (matched != m_worldByLocation.cend())
			{
				if (target != location)
				{
					DBG_MESSAGE("Found worldspace {}/0x{:08x} for parent of location {}/0x{:08x}",
						matched->second->GetName(), matched->second->GetFormID(), target->GetName(), target->GetFormID());
					m_worldByLocation.insert({ target, matched->second });
				}
				break;
			}
			location = location->parentLoc;
		}
	}
#if _DEBUG
	AdventureTargetType adventureType(static_cast<AdventureTargetType>(0));
	for (const auto locationsForType : m_locationsByType)
	{
		if (locationsForType.empty())
		{
			DBG_WARNING("No Adventure Targets for type {}", AdventureTargetName(adventureType));
		}
		else
		{
			DBG_MESSAGE("{} Adventure Targets for type {}", locationsForType.size(), AdventureTargetName(adventureType));
			for (RE::BGSLocation* location : locationsForType)
			{
				DBG_MESSAGE("{}/0x{:08x}", location->GetName(), location->GetFormID());
			}
		}
		adventureType = static_cast<AdventureTargetType>(uint32_t(adventureType) + 1);
	}
#endif
}

// This can only be called from MCM so thread-safe. Build view containing the viable Worlds/Locations for this type.
size_t AdventureTargets::ViableWorldCount(const size_t adventureType) const
{
	RecursiveLockGuard guard(m_adventureLock);
	m_unvisitedLocationsByWorld.clear();
	m_sortedWorlds.clear();
	for (const auto location : m_locationsByType[adventureType])
	{
		if (VisitedPlaces::Instance().IsKnown(location))
			continue;
		const auto worldIter(m_worldByLocation.find(location));
		if (worldIter == m_worldByLocation.cend())
			continue;
		const RE::TESWorldSpace* worldSpace(worldIter->second);
		auto worldLocations(m_unvisitedLocationsByWorld.find(worldSpace));
		if (worldLocations == m_unvisitedLocationsByWorld.cend())
		{
			worldLocations = m_unvisitedLocationsByWorld.insert({ worldSpace, {} }).first;
			m_sortedWorlds.push_back(worldSpace);
		}
		DBG_MESSAGE("Viable Location {}/0x{:08x} in WorldSpace {}/0x{:08x}",
			location->GetName(), location->GetFormID(), worldSpace->GetName(), worldSpace->GetFormID());
		worldLocations->second.insert(location);
	}
	std::sort(m_sortedWorlds.begin(), m_sortedWorlds.end(), [=](const RE::TESWorldSpace* lhs, const RE::TESWorldSpace* rhs) -> bool {
		std::string leftName(lhs->GetName());
		std::string rightName(rhs->GetName());
		return std::lexicographical_compare(leftName.cbegin(), leftName.cend(), rightName.cbegin(), rightName.cend());
	});
	return m_unvisitedLocationsByWorld.size();
}

std::string AdventureTargets::ViableWorldNameByIndexInView(const size_t worldIndex) const
{
	RecursiveLockGuard guard(m_adventureLock);
	if (worldIndex >= m_sortedWorlds.size())
	{
		return "";
	}
	DBG_VMESSAGE("WorldSpace {} at index {}", m_sortedWorlds[worldIndex]->GetName(), worldIndex);
	std::string name(m_sortedWorlds[worldIndex]->GetName());
	if (m_sortedWorlds[worldIndex] == LocationTracker::Instance().CurrentPlayerWorld())
	{
		// highlight player's current world for targeting
		name.append("**");
	}
	return name;
}

void AdventureTargets::SelectCurrentDestination(const size_t worldIndex)
{
	RecursiveLockGuard guard(m_adventureLock);
	if (worldIndex >= m_sortedWorlds.size())
		return;

	const RE::TESWorldSpace* world(m_sortedWorlds[worldIndex]);
	const auto worldLocations(m_unvisitedLocationsByWorld.find(world));
	if (worldLocations == m_unvisitedLocationsByWorld.cend())
		return;
	std::vector<const RE::BGSLocation*> candidates(worldLocations->second.cbegin(), worldLocations->second.cend());
	std::random_device rd;  //Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_int_distribution<size_t> chooser(0, candidates.size() - 1);
	m_targetLocation = candidates[chooser(gen)];
	m_targetWorld = world;

	DBG_MESSAGE("Adventure started to random location {}/0x{:08x} of {} candidates in WorldSpace {}/0x{:08x}",
		m_targetLocation->GetName(), m_targetLocation->GetFormID(), candidates.size(), m_targetWorld->GetName(), m_targetWorld->GetFormID());
	m_adventureEvents.push_back(AdventureEvent::StartAdventure(m_targetWorld, m_targetLocation));
}

void AdventureTargets::CheckReachedCurrentDestination(const RE::BGSLocation* newLocation)
{
	RecursiveLockGuard guard(m_adventureLock);
	if (m_targetLocation)
	{
		if (newLocation == m_targetLocation)
		{
			DBG_MESSAGE("Completed Adventure to location {}/0x{:08x} in WorldSpace {}/0x{:08x}",
				m_targetLocation->GetName(), m_targetLocation->GetFormID(), m_targetWorld->GetName(), m_targetWorld->GetFormID());
			m_adventureEvents.push_back(AdventureEvent::CompleteAdventure());
			static RE::BSFixedString arrivalMsg(papyrus::GetTranslation(nullptr, RE::BSFixedString("$SHSE_ADVENTURE_ARRIVED")));
			if (!arrivalMsg.empty())
			{
				std::string notificationText(arrivalMsg);
				StringUtils::Replace(notificationText, "{TARGET}", m_targetLocation->GetName());
				RE::DebugNotification(notificationText.c_str());
			}
			m_targetLocation = nullptr;
			m_targetWorld = nullptr;
		}
	}
}

void AdventureTargets::AbandonCurrentDestination()
{
	RecursiveLockGuard guard(m_adventureLock);
	if (m_targetLocation)
	{
		DBG_MESSAGE("Abandoned Adventure to location {}/0x{:08x} in WorldSpace {}/0x{:08x}",
			m_targetLocation->GetName(), m_targetLocation->GetFormID(), m_targetWorld->GetName(), m_targetWorld->GetFormID());
		m_targetLocation = nullptr;
		m_targetWorld = nullptr;
		m_adventureEvents.push_back(AdventureEvent::AbandonAdventure());
	}
}

const RE::TESWorldSpace* AdventureTargets::TargetWorld(void) const
{
	RecursiveLockGuard guard(m_adventureLock);
	return m_targetWorld;

}
const RE::BGSLocation* AdventureTargets::TargetLocation(void) const
{
	RecursiveLockGuard guard(m_adventureLock);
	return m_targetLocation;
}
RE::ObjectRefHandle AdventureTargets::TargetMapMarker(void) const
{
	RecursiveLockGuard guard(m_adventureLock);
	const auto matched(m_mapMarkerByLocation.find(m_targetLocation));
	if (matched != m_mapMarkerByLocation.cend())
	{
		return matched->second;
	}
	return RE::ObjectRefHandle();
}
bool AdventureTargets::HasActiveTarget(void) const
{
	RecursiveLockGuard guard(m_adventureLock);
	return m_targetLocation != nullptr;
}

void AdventureTargets::AsJSON(nlohmann::json& j) const
{
	RecursiveLockGuard guard(m_adventureLock);
	if (m_targetLocation)
	{
		j["currentLocation"] = StringUtils::FromFormID(m_targetLocation->GetFormID());
		j["currentWorld"] = StringUtils::FromFormID(m_targetWorld->GetFormID());
	}
	j["events"] = nlohmann::json::array();
	for (const auto& adventureEvent : m_adventureEvents)
	{
		j["events"].push_back(adventureEvent);
	}
}

// rehydrate from cosave data
void AdventureTargets::UpdateFrom(const nlohmann::json& j)
{
	DBG_MESSAGE("Cosave Adventure Targets\n{}", j.dump(2));
	RecursiveLockGuard guard(m_adventureLock);
	const auto worldspace(j.find("currentWorld"));
	m_targetWorld = worldspace != j.cend() ?
		LoadOrder::Instance().RehydrateCosaveFormAs<RE::TESWorldSpace>(StringUtils::ToFormID(worldspace->get<std::string>())) : nullptr;
	const auto location(j.find("currentLocation"));
	m_targetLocation = location != j.cend() ?
		LoadOrder::Instance().RehydrateCosaveFormAs<RE::BGSLocation>(StringUtils::ToFormID(location->get<std::string>())) : nullptr;
	// ensure minimal consistence
	if (bool(m_targetWorld) != bool(m_targetLocation))
	{
		m_targetWorld = nullptr;
		m_targetLocation = nullptr;
	}

	m_adventureEvents.reserve(j["events"].size());
	for (const nlohmann::json& adventureEvent : j["events"])
	{
		const float gameTime(adventureEvent["time"].get<float>());
		const AdventureEventType eventType(static_cast<AdventureEventType>(adventureEvent["event"].get<int>()));
		const auto worldspace(adventureEvent.find("world"));
		const RE::TESWorldSpace* worldspaceForm(worldspace != adventureEvent.cend() ?
			LoadOrder::Instance().RehydrateCosaveFormAs<RE::TESWorldSpace>(StringUtils::ToFormID(worldspace->get<std::string>())) : nullptr);
		const auto location(adventureEvent.find("location"));
		const RE::BGSLocation* locationForm(location != adventureEvent.cend() ?
			LoadOrder::Instance().RehydrateCosaveFormAs<RE::BGSLocation>(StringUtils::ToFormID(location->get<std::string>())) : nullptr);
		// the list was already normalized before saving, no need to call RecordNew
		// player position recorded
		switch(eventType)
		{
		case AdventureEventType::Started:
			m_adventureEvents.push_back(AdventureEvent::StartAdventure(worldspaceForm, locationForm));
			break;
		case AdventureEventType::Complete:
			m_adventureEvents.push_back(AdventureEvent::CompleteAdventure());
			break;
		case AdventureEventType::Abandoned:
			m_adventureEvents.push_back(AdventureEvent::AbandonAdventure());
			break;
		default:
			break;
		}
	}
}

void to_json(nlohmann::json& j, const AdventureTargets& adventureTargets)
{
	adventureTargets.AsJSON(j);
}

}