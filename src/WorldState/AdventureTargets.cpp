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
#include "WorldState/Saga.h"
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
		case AdventureTargetType::Dungeon:		   return "Dungeon";
		case AdventureTargetType::DwarvenRuin:	   return "Dwarven Ruin";
#if _DEBUG
		case AdventureTargetType::FakeForTesting:  return "Fake for Testing";
#endif
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
const RE::BGSLocation* AdventureEvent::m_lastTarget(nullptr);

void AdventureEvent::ResetSagaState()
{
	m_lastTarget = nullptr;
}

AdventureEvent AdventureEvent::StartedAdventure(const RE::TESWorldSpace* world, const RE::BGSLocation* location, const float gameTime)
{
	return AdventureEvent(AdventureEventType::Started, world, location, gameTime);
}

AdventureEvent AdventureEvent::CompletedAdventure(const float gameTime)
{
	return AdventureEvent(AdventureEventType::Complete, gameTime);
}

AdventureEvent AdventureEvent::AbandonedAdventure(const float gameTime)
{
	return AdventureEvent(AdventureEventType::Abandoned, gameTime);
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

AdventureEvent::AdventureEvent(const AdventureEventType eventType, const RE::TESWorldSpace* world, const RE::BGSLocation* location, const float gameTime) :
	m_eventType(eventType), m_world(world), m_location(location), m_gameTime(gameTime)
{
}

AdventureEvent::AdventureEvent(const AdventureEventType eventType, const RE::TESWorldSpace* world, const RE::BGSLocation* location) :
	m_eventType(eventType), m_world(world), m_location(location), m_gameTime(PlayerState::Instance().CurrentGameTime())
{
}
AdventureEvent::AdventureEvent(const AdventureEventType eventType, const float gameTime) :
	m_eventType(eventType), m_world(nullptr), m_location(nullptr), m_gameTime(gameTime)
{
}


AdventureEvent::AdventureEvent(const AdventureEventType eventType) :
	m_eventType(eventType), m_world(nullptr), m_location(nullptr), m_gameTime(PlayerState::Instance().CurrentGameTime())
{
}

std::string AdventureEvent::AsString() const
{
	std::ostringstream stream;
	if (m_eventType == AdventureEventType::Started)
	{
		stream << "My Adventure to " << m_location->GetName() << " in " << m_world->GetName() << " began.";
		m_lastTarget = m_location;
	}
	else if (m_eventType == AdventureEventType::Complete)
	{
		stream << "My Adventure";
		if (m_lastTarget)
		{
			stream << " to " << m_lastTarget->GetName();
			m_lastTarget = nullptr;
		}
		stream << " was complete.";
	}
	else if (m_eventType == AdventureEventType::Abandoned)
	{
		stream << "I abandoned my Adventure";
		if (m_lastTarget)
		{
			stream << " to " << m_lastTarget->GetName();
			m_lastTarget = nullptr;
		}
		stream << '.';
	}
	return stream.str();
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

Position AdventureTargets::GetInteriorCellPosition(const RE::TESObjectCELL* cell, const RE::BGSLocation* location) const
{
	// check for direct CELL linkage to outside world
	for (const auto cellRefr : cell->GetRuntimeData().references)
	{
		if (cellRefr && cellRefr->GetBaseObject() && cellRefr->GetBaseObject()->GetFormType() == RE::FormType::Door)
		{
			DBG_MESSAGE("Interior CELL 0x{:08x} has DOOR 0x{:08x}", cell->GetFormID(), cellRefr->GetFormID());
			// if teleport destination for DOOR is outdoors, return its position
			if (cellRefr->extraList.HasType<RE::ExtraTeleport>())
			{
				DBG_MESSAGE("DOOR 0x{:08x} has XTEL", cellRefr->GetFormID());
				const RE::ExtraTeleport* xtel(cellRefr->extraList.GetByType<RE::ExtraTeleport>());
				if (xtel && xtel->teleportData)
				{
					RE::TESObjectREFR* egressRefr(xtel->teleportData->linkedDoor.get().get());
					if (egressRefr)
					{
						DBG_MESSAGE("DOOR 0x{:08x} XTEL has linked door 0x{:08x}", cellRefr->GetFormID(), egressRefr->GetFormID());
						// Parent Cell is blank for WRLD persistent references, assumed exterior. If CELL is present it must be outdoors.
						if (!egressRefr->GetParentCell() || egressRefr->GetParentCell()->IsExteriorCell())
						{
							LinkLocationToWorld(location, egressRefr->GetWorldspace());
							DBG_MESSAGE("Interior CELL 0x{:08x} has egress via DOOR 0x{:08x} at Position ({:.2f},{:.2f},{:.2f})", cell->GetFormID(),
								egressRefr->GetFormID(), egressRefr->GetPositionX(), egressRefr->GetPositionY(), egressRefr->GetPositionZ());
							return Position({ egressRefr->GetPositionX(), egressRefr->GetPositionY(),egressRefr->GetPositionZ() });
						}
					}
					else
					{
						DBG_MESSAGE("DOOR 0x{:08x} XTEL linked door 0x{:08x} invalid", cellRefr->GetFormID(), xtel->teleportData->linkedDoor.native_handle());
					}
				}
			}
		}
	}
	return InvalidPosition;
}

Position AdventureTargets::GetRefHandlePosition(const RE::ObjectRefHandle handle, const RE::BGSLocation* location) const
{
	if (!handle)
		return InvalidPosition;
	const RE::TESObjectREFR* refr(handle.get().get());
	return GetRefrPosition(refr, location);
}

RE::TESWorldSpace* AdventureTargets::GetRefHandleWorld(const RE::ObjectRefHandle handle) const
{
	if (!handle)
		return nullptr;
	const RE::TESObjectREFR* refr(handle.get().get());
	return refr ? refr->GetWorldspace() : nullptr;
}

Position AdventureTargets::GetRefIDPosition(const RE::FormID refID, const RE::BGSLocation* location) const
{
	const RE::TESObjectREFR* refr(RE::TESForm::LookupByID<RE::TESObjectREFR>(refID));
	return GetRefrPosition(refr, location);
}

Position AdventureTargets::GetRefrPosition(const RE::TESObjectREFR* refr, const RE::BGSLocation* location) const
{
	if (!refr)
		return InvalidPosition;
	// Parent Cell is blank for WRLD persistent references, assumed exterior. If CELL is present it must be outdoors.
	if (!refr->GetParentCell() || refr->GetParentCell()->IsExteriorCell())
	{
		LinkLocationToWorld(location, refr->GetWorldspace());
		DBG_MESSAGE("Exterior REFR 0x{:08x} has Position ({:.2f},{:.2f},{:.2f})", refr->GetFormID(),
			refr->GetPositionX(), refr->GetPositionY(), refr->GetPositionZ());
		return Position({ refr->GetPositionX(), refr->GetPositionY(), refr->GetPositionZ() });
	}
	return GetInteriorCellPosition(refr->GetParentCell(), location);
}

void AdventureTargets::LinkLocationToWorld(const RE::BGSLocation* location, const RE::TESWorldSpace* world) const
{
	if (!location || !world)
		return;

	const auto inserted(m_worldByLocation.insert({ location, world }));
	if (inserted.second)
	{
		DBG_MESSAGE("Found worldspace {}/0x{:08x} for location {}/0x{:08x}",
			world->GetName(), world->GetFormID(), location->GetName(), location->GetFormID());
	}
	else if (inserted.first->second != world)
	{
		REL_WARNING("Found second worldspace {}/0x{:08x} for location {}/0x{:08x}, already have {}/0x{:08x}",
			world->GetName(), world->GetFormID(), location->GetName(), location->GetFormID(),
			inserted.first->second->GetName(), inserted.first->second->GetFormID());
	}
}

// Classify Locations by their keywords
void AdventureTargets::Categorize()
{
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	if (!dhnd)
		return;

	// determine Worldspace for each Location - first scan Worldspace Location lists
	// skip this one
	const RE::FormID AvoidanceExterior = 0x1b44a;
	// hard code this linkage, it's a mess otherwise
	const RE::FormID TamrielLocation = 0x130ff;
	const RE::FormID SkyrimWorldspace = 0x3C;
	for (RE::TESWorldSpace* world : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESWorldSpace>())
	{
		if (world->GetFormID() == AvoidanceExterior || world->GetFullNameLength() == 0)
			continue;
		if (world->GetFormID() == SkyrimWorldspace)
		{
			RE::BGSLocation* tamriel(RE::TESForm::LookupByID<RE::BGSLocation>(TamrielLocation));
			if (tamriel)
			{
				LinkLocationToWorld(tamriel, world);
			}
		}

		for (const auto location : world->locationMap)
		{
			// WRLD has locations, OK to proceed
			if (!location.second->GetFullNameLength())
			{
				DBG_MESSAGE("Skip unnamed WRLD Location 0x{:08x}", location.second->GetFormID());
				continue;
			}
			LinkLocationToWorld(location.second, world);
		}
	}

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
#if _DEBUG
		// Fake test value
		{"FakeForTesting", AdventureTargetType::FakeForTesting},
#endif
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

	// now scan full Location list and match each Location with its WorldSpace, and find Position as XYZ Coordinates
	for (const RE::BGSLocation* location : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::BGSLocation>())
	{
		if (!location->GetFullNameLength())
		{
			DBG_MESSAGE("Skip unnamed Location 0x{:08x}", location->GetFormID());
			continue;
		}
		// check for World Location Marker and infer Worldspace via its CELL
		RE::TESWorldSpace* worldSpace(nullptr);
		Position position(InvalidPosition);
		Position fallbackPosition(InvalidPosition);
		if (location->worldLocMarker)
		{
			position = GetRefHandlePosition(location->worldLocMarker, location);
			worldSpace = GetRefHandleWorld(location->worldLocMarker);
			if (worldSpace)
			{
				DBG_MESSAGE("World Location Marker 0x{:08x} implies worldspace {}/0x{:08x} for location {}/0x{:08x}", location->worldLocMarker.get()->GetFormID(),
					worldSpace->GetName(), worldSpace->GetFormID(), location->GetName(), location->GetFormID());
			}
			LinkLocationToWorld(location, worldSpace);
		}

		// check ACPR/LCPR, if WRLD is referenced anywhere in the parent-child tree that is the answer
		if (location->overrideData)
		{
			DBG_MESSAGE("LCPR found for location {}/0x{:08x}", location->GetName(), location->GetFormID());
			for (const auto& refr : location->overrideData->addedData)
			{
				RE::FormID parentID(refr.parentSpaceID);
				RE::TESForm* parentSpace(RE::TESForm::LookupByID(parentID));
				worldSpace = parentSpace->As<RE::TESWorldSpace>();
				if (worldSpace)
				{
					DBG_MESSAGE("LCPR 0x{:08x} implies worldspace {}/0x{:08x} for location {}/0x{:08x}", refr.refID,
						worldSpace->GetName(), worldSpace->GetFormID(), location->GetName(), location->GetFormID());
					LinkLocationToWorld(location, worldSpace);
				}
			}
		}
		else
		{
			DBG_MESSAGE("No LCPR for location {}/0x{:08x}", location->GetName(), location->GetFormID());
		}

		// Position may have been derived from World Location Marker
		if (position == InvalidPosition)
		{
			// check for ACSR/LCSR 
			for (const auto& csr : location->specialRefs)
			{
				static const RE::FormID MapMarkerLCRT = 0x10f63c;
				static const RE::FormID LocationCenterLCRT = 0x1bdf1;
				static const RE::FormID OutsideEntranceMarkerLCRT = 0x130fb;
				// location-ref-type (LCRT) is a keyword, we want Map Marker (preferred), Location Center or Outside Entrance
				if (csr.type->GetFormID() == MapMarkerLCRT)
				{
					Position newPosition(GetRefIDPosition(csr.refData.refID, location));
					if (newPosition != InvalidPosition)
					{
						position = newPosition;
						break;
					}
				}
				else if (fallbackPosition == InvalidPosition)
				{
					// check other LCRTs for a locatable parent CELL/WRLD
					RE::FormID parentID(csr.refData.parentSpaceID);
					RE::TESForm* parentSpace(RE::TESForm::LookupByID(parentID));
					const auto cell(parentSpace->As<RE::TESObjectCELL>());
					if (cell)
					{
						if (cell->IsInteriorCell())
						{
							Position newPosition(GetInteriorCellPosition(cell, location));
							if (newPosition != InvalidPosition)
							{
								DBG_MESSAGE("Location {}/0x{:08x} is Interior CELL 0x{:08x}", location->GetName(), location->GetFormID(), parentID);
								fallbackPosition = newPosition;
							}
						}
						else
						{
							Position newPosition(GetRefIDPosition(csr.refData.refID, location));
							if (newPosition != InvalidPosition)
							{
								DBG_MESSAGE("Location {}/0x{:08x} is in Exterior CELL 0x{:08x}", location->GetName(), location->GetFormID(), parentID);
								fallbackPosition = newPosition;
							}
						}
					}
					else if (parentSpace->As<RE::TESWorldSpace>())
					{
						Position newPosition(GetRefIDPosition(csr.refData.refID, location));
						if (newPosition != InvalidPosition)
						{
							DBG_MESSAGE("Location {}/0x{:08x} is in WRLD 0x{:08x}", location->GetName(), location->GetFormID(), parentID);
							fallbackPosition = newPosition;
						}
						LinkLocationToWorld(location, parentSpace->As<RE::TESWorldSpace>());
					}
					else
					{
						DBG_MESSAGE("Location {}/0x{:08x} center/entrance skipped", location->GetName(), location->GetFormID());
					}
				}
			}
		}
		if (position == InvalidPosition)
		{
			if (fallbackPosition == InvalidPosition)
			{
				DBG_MESSAGE("Location {}/0x{:08x} cannot be Placed in world", location->GetName(), location->GetFormID());
				continue;
			}
			position = fallbackPosition;
		}
		DBG_MESSAGE("Location {}/0x{:08x} at ({:.2f},{:.2f},{:.2f})", location->GetName(), location->GetFormID(),
			position[0], position[1], position[2]);

		// Scan location keywords to check if it's a target type
		uint32_t numKeywords(location->GetNumKeywords());
		for (uint32_t next = 0; next < numKeywords; ++next)
		{
			std::optional<RE::BGSKeyword*> keyword(location->GetKeywordAt(next));
			if (!keyword.has_value() || !keyword.value())
				continue;

			std::string keywordName(FormUtils::SafeGetFormEditorID(keyword.value()));
			const auto matched(targetByKeyword.find(keywordName));
			if (matched == targetByKeyword.cend())
				continue;
			m_locationsByType[size_t(matched->second)].insert(location);
		}
		// save all location markers, even if not a valid Adventure Target
		m_locationCoordinates.insert({ location, position });
	}

	// scan marked locations and record the Worldspace for each
	for (const auto& markedLocation : m_locationCoordinates)
	{
		const RE::BGSLocation* location(markedLocation.first);
		const auto& matched(m_worldByLocation.find(location));
		if (matched != m_worldByLocation.cend())
		{
			auto worldList(m_markedLocationsByWorld.find(matched->second));
			if (worldList == m_markedLocationsByWorld.end())
			{
				worldList = m_markedLocationsByWorld.insert({ matched->second, {} }).first;
			}
			worldList->second.insert(location);
			DBG_MESSAGE("Mapped worldspace {}/0x{:08x} for location {}/0x{:08x}",
				worldList->first->GetName(), worldList->first->GetFormID(), location->GetName(), location->GetFormID());
		}
	}

	AdventureTargetType adventureType(static_cast<AdventureTargetType>(0));
	size_t targets(0);
	for (const auto locationsForType : m_locationsByType)
	{
		if (locationsForType.empty())
		{
			REL_WARNING("No Adventure Targets for type {}", AdventureTargetName(adventureType));
		}
		else
		{
			REL_MESSAGE("{} Adventure Targets for type {}", locationsForType.size(), AdventureTargetName(adventureType));
			targets += locationsForType.size();
			for (const RE::BGSLocation* location : locationsForType)
			{
				REL_MESSAGE("  {}/0x{:08x}", location->GetName(), location->GetFormID());
			}
		}
		adventureType = static_cast<AdventureTargetType>(uint32_t(adventureType) + 1);
	}
	REL_MESSAGE("{} Adventure Targets created from {} unique Locations", targets,
		RE::TESDataHandler::GetSingleton()->GetFormArray<RE::BGSLocation>().size());
}

std::string AdventureTargets::AdventureTypeName(const size_t adventureType) const
{
	if (adventureType >= m_validAdventureTypes.size())
	{
		return "";
	}
	return AdventureTargetNameByIndex(size_t(m_validAdventureTypes[adventureType]));
}

// filter adventure types to only show those with unknown locations
size_t AdventureTargets::AvailableAdventureTypes() const
{
	RecursiveLockGuard guard(m_adventureLock);
	m_validAdventureTypes.clear();
	size_t adventureType(0);
	for (const auto locations : m_locationsByType)
	{
		if (std::find_if(locations.cbegin(), locations.cend(), [=](const RE::BGSLocation* location) -> bool	{
				return !VisitedPlaces::Instance().IsKnown(location);
			}) != locations.cend())
		{
			// establish mapping between MCM index and original adventure type if there are unknown locations for this type
			m_validAdventureTypes.push_back(AdventureTargetType(adventureType));
		}
        ++adventureType;
	}
	return m_validAdventureTypes.size();
}

std::unordered_map<const RE::BGSLocation*, Position> AdventureTargets::GetWorldMarkedPlaces(const RE::TESWorldSpace* world) const
{
	RecursiveLockGuard guard(m_adventureLock);
	std::unordered_map<const RE::BGSLocation*, Position> markedPlaces;
	const auto& worldLocations(m_markedLocationsByWorld.find(world));
	if (worldLocations != m_markedLocationsByWorld.cend())
	{
		for (const RE::BGSLocation* location : worldLocations->second)
		{
			const auto& locationCoordinates(m_locationCoordinates.find(location));
			if (locationCoordinates != m_locationCoordinates.cend())
			{
				Position position(locationCoordinates->second);
				if (markedPlaces.insert(std::make_pair(location, position)).second)
				{
					DBG_VMESSAGE("Location {}/0x{:08x} at coordinates ({:0.2f},{:0.2f},{:0.2f})", location->GetName(), location->GetFormID(),
						position[0], position[1], position[2]);
				}
				else
				{
					DBG_VMESSAGE("Location {}/0x{:08x} already recorded", location->GetName(), location->GetFormID());
				}
			}
			else
			{
				DBG_VMESSAGE("Location {}/0x{:08x} has no map marker", location->GetName(), location->GetFormID());
			}
		}
	}
	return markedPlaces;
}

// This can only be called from MCM so thread-safe. Build view containing the viable Worlds/Locations for this type.
size_t AdventureTargets::ViableWorldCount(const size_t adventureType) const
{
	RecursiveLockGuard guard(m_adventureLock);
	m_unvisitedLocationsByWorld.clear();
	m_sortedWorlds.clear();
	if (adventureType >= m_validAdventureTypes.size())
		return 0;
	// map from MCM index for list of adventure types with valid worlds back to enumeration
	for (const RE::BGSLocation* location : m_locationsByType[size_t(m_validAdventureTypes[adventureType])])
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
	if (candidates.empty())
		return;
	std::random_device rd;  //Will be used to obtain a seed for the random number engine
	std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
	std::uniform_int_distribution<size_t> chooser(0, candidates.size() - 1);
	m_targetLocation = candidates[chooser(gen)];
	m_targetWorld = world;

	DBG_MESSAGE("Adventure started to random location {}/0x{:08x} of {} candidates in WorldSpace {}/0x{:08x}",
		m_targetLocation->GetName(), m_targetLocation->GetFormID(), candidates.size(), m_targetWorld->GetName(), m_targetWorld->GetFormID());
	RecordEvent(AdventureEvent::StartAdventure(m_targetWorld, m_targetLocation));
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
			RecordEvent(AdventureEvent::CompleteAdventure());
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
		RecordEvent(AdventureEvent::AbandonAdventure());
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
Position AdventureTargets::TargetPosition(void) const
{
	RecursiveLockGuard guard(m_adventureLock);
	const auto matched(m_locationCoordinates.find(m_targetLocation));
	if (matched != m_locationCoordinates.cend())
	{
		DBG_MESSAGE("Adventure location {}/0x{:08x} has Position ({:.2f},{:.2f},{:.2f})",
			m_targetLocation->GetName(), m_targetLocation->GetFormID(), matched->second[0], matched->second[1], matched->second[2]);
		return matched->second;
	}
	return InvalidPosition;
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

void AdventureTargets::RecordEvent(const AdventureEvent& event)
{
	m_adventureEvents.push_back(event);
	Saga::Instance().AddEvent(event);
}

// rehydrate from cosave data
void AdventureTargets::UpdateFrom(const nlohmann::json& j)
{
	REL_MESSAGE("Cosave Adventure Targets\n{}", j.dump(2));
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
		const auto eventWorldspace(adventureEvent.find("world"));
		const RE::TESWorldSpace* worldspaceForm(eventWorldspace != adventureEvent.cend() ?
			LoadOrder::Instance().RehydrateCosaveFormAs<RE::TESWorldSpace>(StringUtils::ToFormID(eventWorldspace->get<std::string>())) : nullptr);
		const auto eventLocation(adventureEvent.find("location"));
		const RE::BGSLocation* locationForm(eventLocation != adventureEvent.cend() ?
			LoadOrder::Instance().RehydrateCosaveFormAs<RE::BGSLocation>(StringUtils::ToFormID(eventLocation->get<std::string>())) : nullptr);
		// the list was already normalized before saving, no need to call RecordNew
		// player position recorded
		switch(eventType)
		{
		case AdventureEventType::Started:
			RecordEvent(AdventureEvent::StartedAdventure(worldspaceForm, locationForm, gameTime));
			break;
		case AdventureEventType::Complete:
			RecordEvent(AdventureEvent::CompletedAdventure(gameTime));
			break;
		case AdventureEventType::Abandoned:
			RecordEvent(AdventureEvent::AbandonedAdventure(gameTime));
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