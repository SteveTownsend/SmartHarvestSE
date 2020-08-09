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

#include <cmath>
#include <numbers>

#include "WorldState/LocationTracker.h"
#include "Collections/CollectionManager.h"
#include "Looting/ManagedLists.h"
#include "WorldState/AdventureTargets.h"
#include "WorldState/PartyMembers.h"
#include "WorldState/PlayerHouses.h"
#include "WorldState/PlayerState.h"
#include "WorldState/PopulationCenters.h"
#include "WorldState/VisitedPlaces.h"
#include "Looting/ScanGovernor.h"
#include "VM/papyrus.h"
#include "Utilities/utils.h"

namespace shse
{

std::unique_ptr<LocationTracker> LocationTracker::m_instance;

LocationTracker& LocationTracker::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<LocationTracker>();
	}
	return *m_instance;
}

LocationTracker::LocationTracker() : 
	m_playerCell(nullptr), m_priorCell(nullptr), m_playerLocation(nullptr), m_playerParentWorld(nullptr),
	m_tellPlayerIfCanLootAfterLoad(false)
{
}

// called when Player moves to a new WorldSpace, including on game load/reload
void LocationTracker::RecordMarkedPlaces()
{
	decltype(m_markedPlaces) markedPlaces;
	// record map marker position for worldspace locations
	for (const auto& formLocation : m_playerParentWorld->locationMap)
	{
		RE::BGSLocation* location(formLocation.second);
		RE::ObjectRefHandle markerRef(location->worldLocMarker);
		if (!markerRef)
		{
			DBG_VMESSAGE("Location {}/0x{:08x} has no map marker", location->GetName(), location->GetFormID());
			continue;
		}
		shse::Position markerPosition({ markerRef.get()->GetPositionX(), markerRef.get()->GetPositionY(), markerRef.get()->GetPositionZ() });
		if (markedPlaces.insert(std::make_pair(location, markerPosition)).second)
		{
			DBG_VMESSAGE("Location {}/0x{:08x} has marker at ({:0.2f},{:0.2f},{:0.2f})", location->GetName(), location->GetFormID(),
				markerRef.get()->GetPositionX(), markerRef.get()->GetPositionY(), markerRef.get()->GetPositionZ());
		}
		else
		{
			DBG_VMESSAGE("Location {}/0x{:08x} already recorded", location->GetName(), location->GetFormID());
		}
	}
	if (markedPlaces.empty())
	{
		DBG_VMESSAGE("No map markers within this worldspace");
		return;
	}
	// replace existing entries with new
	m_markedPlaces.swap(markedPlaces);
	size_t numPlaces(m_markedPlaces.size());
	DBG_MESSAGE("Build tree from {} LCTNs", numPlaces);

	// Build KD-tree for the location markers
	std::vector<double> pointData;
	pointData.reserve(3 * numPlaces);
	std::vector<alglib::ae_int_t> tagList;
	tagList.reserve(numPlaces);
	std::for_each(m_markedPlaces.cbegin(), m_markedPlaces.cend(), [&](const auto& posForm)
	{
		tagList.push_back(posForm.first->GetFormID());
		pointData.push_back(std::get<0>(posForm.second));
		pointData.push_back(std::get<1>(posForm.second));
		pointData.push_back(std::get<2>(posForm.second));
	});

	alglib::real_2d_array coordinates;
	coordinates.setcontent(numPlaces, 3, &pointData[0]);
	alglib::integer_1d_array tags;
	tags.setcontent(numPlaces, &tagList[0]);

	// use Euclidean norm 2 for 3D space
	alglib::kdtreebuildtagged(coordinates, tags, numPlaces, 3, 0, 2, m_markers);
}

CompassDirection LocationTracker::DirectionToDestinationFromStart(const AlglibPosition& start, const AlglibPosition& destination) const
{
	// only uses (x, y) parts of the endpoints
	double deltaX(destination[0] - start[0]);
	double deltaY(destination[1] - start[1]);

	double degrees(std::atan2(deltaX, deltaY) * (180.0 / std::numbers::pi));
	// which arc of the compass does this fall into? The value is in the range [-180.0, 180.0]
	static const std::vector<std::pair<double, CompassDirection>> directions = {
		{-157.5, CompassDirection::South},
		{-112.5, CompassDirection::SouthWest},
		{-67.5, CompassDirection::West},
		{-22.5, CompassDirection::NorthWest},
		{22.5, CompassDirection::North},
		{67.5, CompassDirection::NorthEast},
		{112.5, CompassDirection::East},
		{157.5, CompassDirection::SouthEast},
		{-180., CompassDirection::South}
	};
	auto& direction = directions.cbegin();
	while (degrees > direction->first and direction != directions.cend())
	{
		++direction;
	}
	CompassDirection result(direction == directions.cend() ? CompassDirection::North : direction->second);
	DBG_MESSAGE("Head {} at {:0.2f} degrees from ({:0.2f},{:0.2f}) to ({:0.2f},{:0.2f})", CompassDirectionName(result).c_str(),
		degrees, start[0], start[1], destination[0], destination[1]);
	return result;
}

std::string LocationTracker::ParentLocationName(const RE::BGSLocation* location) const
{
	RE::BGSLocation* parent(location->parentLoc);
	while (parent && parent->GetName() == location->GetName())
	{
		parent = parent->parentLoc;
	}
	return parent ? parent->GetName() : std::string();
}

std::string LocationTracker::Proximity(const double milesAway, CompassDirection heading) const
{
	if (milesAway == 0.)
	{
		// player is physically at the place
		return "at";
	}
	else
	{
		// conversational description of player's position in relation to the place
		std::ostringstream proximity;
		proximity << ConversationalDistance(milesAway) << ' ' << CompassDirectionName(heading) << " of";
		return proximity.str();
	}
}

std::string LocationTracker::ConversationalDistance(const double milesAway) const
{
	if (milesAway < OnlyYards)
		return "only yards";
	if (milesAway < LittleWay)
		return "a little way";
	if (milesAway < HalfMile)
		return "about half a mile";
	if (milesAway < MileOrSo)
		return "a mile or so";
	if (milesAway < CoupleOfMiles)
		return "a couple of miles";
	return "several miles";
}

std::string LocationTracker::PlayerExactLocation() const
{
	if (!m_playerCell && !m_playerLocation)
		return "unknown";

	std::ostringstream locationStr;
	if (m_playerLocation)
	{
		locationStr << "at " << m_playerLocation->GetName() << "/0x" << StringUtils::FromFormID(m_playerLocation->GetFormID()) << ' ';
	}
	if (m_playerCell)
	{
		locationStr << "in Cell " << FormUtils::SafeGetFormEditorID(m_playerCell) << "/0x" << StringUtils::FromFormID(m_playerCell->GetFormID());
	}
	return locationStr.str();
}

void LocationTracker::PrintPlayerLocation(const RE::BGSLocation* location) const
{
	PrintNearbyLocation(location, 0., CompassDirection::MAX);
}

void LocationTracker::PrintNearbyLocation(const RE::BGSLocation* location, const double milesAway, CompassDirection heading) const
{
	static RE::BSFixedString locationText(papyrus::GetTranslation(nullptr, RE::BSFixedString("$SHSE_WHERE_AM_I")));
	if (!locationText.empty())
	{
		std::string locationMessage(locationText);
		StringUtils::Replace(locationMessage, "{PROXIMITY}", Proximity(milesAway, heading));
		StringUtils::Replace(locationMessage, "{LOCATION}", location->GetName());
		StringUtils::Replace(locationMessage, "{VICINITY}", ParentLocationName(location));
		if (!locationMessage.empty())
		{
			RE::DebugNotification(locationMessage.c_str());
		}
	}
}

void LocationTracker::PrintAdventureTargetInfo(const RE::BGSLocation* location, const double milesAway, CompassDirection heading) const
{
	static RE::BSFixedString locationText(papyrus::GetTranslation(nullptr, RE::BSFixedString("$SHSE_WHERE_IS_ADVENTURE_TARGET")));
	if (!locationText.empty())
	{
		std::string locationMessage(locationText);
		StringUtils::Replace(locationMessage, "{PROXIMITY}", Proximity(milesAway, heading));
		// stop obfuscating the destination once player gets close - map marker may be for a parent, not exactly colocated with target
		if (milesAway < OnlyYards)
		{
			StringUtils::Replace(locationMessage, "{TARGET}", location->GetName());
		}
		else
		{
			StringUtils::Replace(locationMessage, "{TARGET}", "the place my Adventurer's Instinct seeks");
		}
		if (!locationMessage.empty())
		{
			RE::DebugNotification(locationMessage.c_str());
		}
	}
}

void LocationTracker::PrintDifferentWorld(const RE::TESWorldSpace* world) const
{
	std::string locationMessage;
	static RE::BSFixedString locationText(papyrus::GetTranslation(nullptr, RE::BSFixedString("$SHSE_ADVENTURE_DIFFERENT_WORLD")));
	if (!locationText.empty())
	{
		locationMessage = locationText;
		StringUtils::Replace(locationMessage, "{WORLD}", world->GetName());
		if (!locationMessage.empty())
		{
			RE::DebugNotification(locationMessage.c_str());
		}
	}
}

void LocationTracker::DisplayPlayerLocation() const
{
	// display location relative to Adventure Target, or nearest marker if no Adventure in progress
	const RE::BGSLocation* locationDone(nullptr);
	if (AdventureTargets::Instance().TargetLocation())
	{
		locationDone = PlayerLocationRelativeToAdventureTarget();
	}
	// print current location if it's not the same as the target
	PlayerLocationRelativeToNearestMapMarker(locationDone);
}

void LocationTracker::PlayerLocationRelativeToNearestMapMarker(const RE::BGSLocation* locationDone) const
{
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Locate relative to map marker");
#endif
	AlglibPosition playerPos(PlayerState::Instance().GetAlglibPosition());
	RecursiveLockGuard guard(m_locationLock);
	if (m_playerLocation)
	{
		DBG_MESSAGE("Player ({:0.2f}, {:0.2f}) is in location {}/0x{:08x}", playerPos[0], playerPos[1],
			m_playerLocation->GetName(), m_playerLocation->GetFormID());
		if (m_playerLocation != locationDone)
		{
			PrintPlayerLocation(m_playerLocation);
		}
		return;
	}
	RelativeLocationDescriptor nearestMarker(NearestMapMarker(playerPos));
	if (nearestMarker == RelativeLocationDescriptor::Invalid())
	{
		REL_WARNING("Could not determine nearest map marker to player ({:0.2f}, {:0.2f})", playerPos[0], playerPos[1]);
		return;
	}
	CompassDirection heading(DirectionToDestinationFromStart(nearestMarker.EndPoint(), nearestMarker.StartPoint()));

	double milesAway(UnitsToMiles(nearestMarker.UnitsAway()));
	RE::BGSLocation* location(RE::TESForm::LookupByID<RE::BGSLocation>(nearestMarker.LocationID()));
	if (location)
	{
		DBG_MESSAGE("Player is {} of nearest Location map marker {}/0x{:08x} at distance {:0.2f} miles",
			CompassDirectionName(heading).c_str(), location->GetName(), nearestMarker.LocationID(), milesAway);
		if (m_playerLocation != locationDone)
		{
			PrintNearbyLocation(location, milesAway, heading);
		}
	}
	else
	{
		REL_WARNING("Could not determine Location for 0x{:08x}", nearestMarker.LocationID());
	}
}

// scan the KD-tree for nearest map marker to a reference position
RelativeLocationDescriptor LocationTracker::NearestMapMarker(const AlglibPosition& refPos) const
{
	alglib::real_1d_array coordinates;
	coordinates.setcontent(3, refPos.data());
	alglib::ae_int_t result(alglib::kdtreequeryknn(m_markers, coordinates, 1));
	if (result != 1)
	{
		REL_WARNING("Expected 1 match for nearest map marker, got {}", result);
		return RelativeLocationDescriptor::Invalid();
	}
	// get distance - overwrite the input to avoid another allocation
	kdtreequeryresultsdistances(m_markers, coordinates);
	double unitsAway(coordinates[0]);

	// get tag, into preallocated buffer
	alglib::integer_1d_array tags;
	alglib::ae_int_t dummy(0);
	tags.setcontent(1, &dummy);
	kdtreequeryresultstags(m_markers, tags);
	RE::FormID location(static_cast<RE::FormID>(tags[0]));

	// get coordinates for the nearest marker
	alglib::real_2d_array marker;
	marker.setlength(1, 3);
	kdtreequeryresultsx(m_markers, marker);

	AlglibPosition markerPos = { marker(0, 0), marker(0, 1), marker(0, 2) };
	return RelativeLocationDescriptor(refPos, markerPos, location, unitsAway);
}

double DistanceBetween(const AlglibPosition& pos1, const AlglibPosition& pos2)
{
	double dx(pos1[0] - pos2[0]);
	double dy(pos1[1] - pos2[1]);
	double dz(pos1[2] - pos2[2]);
	return sqrt(dx * dx + dy * dy + dz * dz);
}

RelativeLocationDescriptor LocationTracker::LocationMapMarker(
	const RE::ObjectRefHandle targetMarker, const RE::BGSLocation* location, const AlglibPosition& refPos) const
{
	const RE::TESObjectREFR* refr(targetMarker.get().get());
	AlglibPosition markerPos({ refr->GetPositionX(), refr->GetPositionY(), refr->GetPositionZ() });
	double unitsAway(DistanceBetween(markerPos, refPos));
	return RelativeLocationDescriptor(refPos, markerPos, location->GetFormID(), unitsAway);
}

const RE::BGSLocation* LocationTracker::PlayerLocationRelativeToAdventureTarget() const
{
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Locate relative to adventure target");
#endif
	const RE::TESWorldSpace* world(AdventureTargets::Instance().TargetWorld());
	const RE::BGSLocation* location(AdventureTargets::Instance().TargetLocation());
	RE::ObjectRefHandle targetMarker(AdventureTargets::Instance().TargetMapMarker());
	if (!location || !targetMarker)
	{
		// no adventure in progress, or target unmappable
		return nullptr;
	}

	// Adventure Target sensing only works outdoors
	if (m_playerCell->IsInteriorCell())
	{
		static RE::BSFixedString locationText(papyrus::GetTranslation(nullptr, RE::BSFixedString("$SHSE_GO_OUTSIDE_ADVENTURE_TARGET")));
		if (!locationText.empty())
		{
			std::string locationMessage(locationText);
			if (!locationMessage.empty())
			{
				RE::DebugNotification(locationMessage.c_str());
			}
		}
		return location;
	}

	RecursiveLockGuard guard(m_locationLock);
	if (world != m_playerParentWorld)
	{
		PrintDifferentWorld(world);
		return nullptr;
	}
	AlglibPosition playerPos(PlayerState::Instance().GetAlglibPosition());
	RelativeLocationDescriptor targetLocation(LocationMapMarker(targetMarker, location, playerPos));
	if (targetLocation == RelativeLocationDescriptor::Invalid())
	{
		REL_WARNING("Could not determine adventure target marker");
		return nullptr;
	}
	CompassDirection heading(DirectionToDestinationFromStart(targetLocation.EndPoint(), targetLocation.StartPoint()));

	double milesAway(UnitsToMiles(targetLocation.UnitsAway()));
	DBG_MESSAGE("Player is {} of adventure target marker {}/0x{:08x} at distance {:0.2f} miles",
		CompassDirectionName(heading).c_str(), location->GetName(), targetLocation.LocationID(), milesAway);
	PrintAdventureTargetInfo(location, milesAway, heading);
	return location;
}

CellOwnership LocationTracker::GetCellOwnership(const RE::TESObjectCELL* cell) const
{
	if (!cell)
		return CellOwnership::NoOwner;
	RE::TESForm* owner = GetCellOwner(cell);
	if (!owner)
		return CellOwnership::NoOwner;
	if (owner->formType == RE::FormType::NPC)
	{
		const RE::TESNPC* npc = owner->As<RE::TESNPC>();
		RE::TESNPC* playerBase = RE::PlayerCharacter::GetSingleton()->GetActorBase();
		if (npc && npc == playerBase)
		{
			return CellOwnership::Player;
		}
		return CellOwnership::NPC;
	}
	else if (owner->formType == RE::FormType::Faction)
	{
		RE::TESFaction* faction = owner->As<RE::TESFaction>();
		if (faction && RE::PlayerCharacter::GetSingleton()->IsInFaction(faction))
		{
			return CellOwnership::PlayerFaction;
		}
		return CellOwnership::OtherFaction;
	}
	REL_WARNING("Owner 0x{:08x} exists but uncategorized in cell 0x{:08x}", owner->GetFormID(), cell->GetFormID());
	return CellOwnership::NoOwner;
}

RE::TESForm* LocationTracker::GetCellOwner(const RE::TESObjectCELL* cell) const
{
	for (const RE::BSExtraData& extraData : cell->extraList)
	{
		if (extraData.GetType() == RE::ExtraDataType::kOwnership)
		{
			DBG_VMESSAGE("GetCellOwner Hit {:08x}", reinterpret_cast<const RE::ExtraOwnership&>(extraData).owner->formID);
			return reinterpret_cast<const RE::ExtraOwnership&>(extraData).owner;
		}
	}
	return nullptr;
}

void LocationTracker::Reset()
{
	DBG_MESSAGE("Reset Location Tracking after reload");
	RecursiveLockGuard guard(m_locationLock);
	m_tellPlayerIfCanLootAfterLoad = true;
	m_playerCell = nullptr;
	m_priorCell = nullptr;
	m_adjacentCells.clear();
	m_playerLocation = nullptr;
	m_playerParentWorld = nullptr;
	m_markedPlaces.clear();
}

const RE::TESWorldSpace* LocationTracker::ParentWorld(const RE::TESObjectCELL* cell)
{
	const RE::TESWorldSpace* world(cell->worldSpace);
	const RE::TESWorldSpace* candidate(nullptr);
	std::vector<const RE::TESWorldSpace*> worlds;
	while (world)
	{
		if (world->locationMap.size() > 0)
		{
			// parent world has locations, OK to proceed
			DBG_MESSAGE("Found location-bearing parent world {}/0x{:08x} for cell 0x{:08x}", world->GetName(), world->GetFormID(), cell->GetFormID());
			candidate = world;
		}
		if (!world->parentWorld)
		{
			DBG_MESSAGE("Reached root of worldspace hierarchy {}/0x{:08x} for cell 0x{:08x}", world->GetName(), world->GetFormID(), cell->GetFormID());
			break;
		}
		world = world->parentWorld;
		if (std::find(worlds.cbegin(), worlds.cend(), world) != worlds.cend())
		{
			// cycle in worldspace graph, return best so far
			REL_ERROR("Cycle in worldspace graph at {}/0x{:08x} for cell 0x{:08x}", world->GetName(), world->GetFormID(), cell->GetFormID());
			break;
		}
		worlds.push_back(world);
	}
	return candidate;
}


// refresh state based on player's current position
bool LocationTracker::Refresh()
{
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Location Data Refresh");
#endif
	RE::PlayerCharacter* player(RE::PlayerCharacter::GetSingleton());
	if (!player)
	{
		DBG_MESSAGE("PlayerCharacter not available");
		return false;
	}
	// handle player death. Obviously we are not looting on their behalf until a game reload or other resurrection event.
	// Assumes player non-essential: if player is in God mode a little extra carry weight or post-death looting is not
	// breaking immersion.
	if (player->IsDead(true))
	{
		return false;
	}

	RecursiveLockGuard guard(m_locationLock);

	// Reset blocked lists if player cell has changed
	bool playerMoved(false);
	RE::TESObjectCELL* playerCell(RE::PlayerCharacter::GetSingleton()->parentCell);
	if (playerCell != m_playerCell)
	{
		// CELL change only triggers visited-place update if we went from outdoors to indoors or vice versa
		playerMoved = !m_playerCell || !playerCell || m_playerCell->IsInteriorCell() != playerCell->IsInteriorCell();
		m_playerCell = playerCell;
		if (m_playerCell)
		{
			DBG_MESSAGE("Player cell updated to 0x{:08x}", m_playerCell->GetFormID());
			RecordAdjacentCells();

			// player cell is valid - check for worldspace update
			const RE::TESWorldSpace* parentWorld(ParentWorld(m_playerCell));
			if (parentWorld && parentWorld != m_playerParentWorld)
			{
				m_playerParentWorld = parentWorld;
				DBG_MESSAGE("Player Parent WorldSpace updated to {}/0x{:08x}", m_playerParentWorld->GetName(), m_playerParentWorld->GetFormID());

				RecordMarkedPlaces();
			}
		}
		else
		{
			DBG_MESSAGE("Player cell cleared");
		}
		// Fire limited location change logic on cell update
		static const bool gameReload(false);
		PluginFacade::Instance().ResetState(gameReload);
	}

	// Scan and Location tracking should not run if Player cell is not yet filled in
	if (!m_playerCell)
		return false;

	const RE::BGSLocation* playerLocation(player->currentLocation);
	if (playerLocation != m_playerLocation || m_tellPlayerIfCanLootAfterLoad)
	{
        // record all Location changes - Location may be blank now but we want to know we left a Location
		playerMoved = true;

		// Output messages for any looting-restriction place change
		static const bool allowIfRestricted(false);

		std::string oldName(m_playerLocation ? m_playerLocation->GetName() : "unnamed");
		bool couldLootInPrior(LocationTracker::Instance().IsPlayerInLootablePlace(m_priorCell, allowIfRestricted));
		DBG_MESSAGE("Player was at location {}, lootable = {}, now at {}", oldName.c_str(),
			couldLootInPrior ? "true" : "false", playerLocation ? playerLocation->GetName() : "unnamed");
		m_playerLocation = playerLocation;

		// location change may mark the end of an in-progress 'Venture into the Unknown'
		AdventureTargets::Instance().CheckReachedCurrentDestination(m_playerLocation);

		// Player changed location - may be a standalone Cell with m_playerLocation nullptr e.g. 000HatredWell
		bool tellPlayer(INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "NotifyLocationChange") != 0);

		// check if it is a new player house
		if (!IsPlayerAtHome())
		{
			if (m_playerLocation && PlayerHouses::Instance().IsValidHouse(m_playerLocation))
			{
				// record as a player house and notify as it is a new one in this game load
				DBG_MESSAGE("Player House {} detected", m_playerLocation->GetName());
				PlayerHouses::Instance().Add(m_playerLocation);
			}
		}
		// Display messages about location auto-loot restrictions, if set in config
		if (tellPlayer)
		{
			// notify entry to player home unless this was a menu reset, regardless of whether it's new to us
			if (IsPlayerAtHome())
			{
				static RE::BSFixedString playerHouseMsg(papyrus::GetTranslation(nullptr, RE::BSFixedString("$SHSE_HOUSE_CHECK")));
				if (!playerHouseMsg.empty())
				{
					std::string notificationText(playerHouseMsg);
					StringUtils::Replace(notificationText, "{HOUSENAME}", m_playerLocation->GetName());
					RE::DebugNotification(notificationText.c_str());
				}
			}
			// Check if location is excluded from looting and if so, notify we entered it
			if ((couldLootInPrior || m_tellPlayerIfCanLootAfterLoad) && !LocationTracker::Instance().IsPlayerInLootablePlace(PlayerCell(), allowIfRestricted))
			{
				DBG_MESSAGE("Player Location {} no-loot message", playerLocation ? playerLocation->GetName() : "unnamed");
				static RE::BSFixedString restrictedPlaceMsg(papyrus::GetTranslation(nullptr, RE::BSFixedString("$SHSE_POPULATED_CHECK")));
				if (!restrictedPlaceMsg.empty())
				{
					std::string notificationText(restrictedPlaceMsg);
					StringUtils::Replace(notificationText, "{LOCATIONNAME}", playerLocation ? playerLocation->GetName() : "unnamed");
					RE::DebugNotification(notificationText.c_str());
				}
			}
		}

		// Display messages about location auto-loot restrictions, if set in config
		if (tellPlayer)
		{
			// check if we moved from a non-lootable place to a lootable place
			if ((!couldLootInPrior || m_tellPlayerIfCanLootAfterLoad) && LocationTracker::Instance().IsPlayerInLootablePlace(PlayerCell(), allowIfRestricted))
			{
				DBG_MESSAGE("Player Location {} OK-loot message", oldName.c_str());
				static RE::BSFixedString unrestrictedPlaceMsg(papyrus::GetTranslation(nullptr, RE::BSFixedString("$SHSE_UNPOPULATED_CHECK")));
				if (!unrestrictedPlaceMsg.empty())
				{
					std::string notificationText(unrestrictedPlaceMsg);
					StringUtils::Replace(notificationText, "{LOCATIONNAME}", oldName.c_str());
					RE::DebugNotification(notificationText.c_str());
				}
			}
		}

		// once any change is processed, reset sentinel for game reload
		m_tellPlayerIfCanLootAfterLoad = false;
	}

	if (playerMoved)
	{
		RecordCurrentPlace(PlayerState::Instance().CurrentGameTime());
	}
	return true;
}

bool LocationTracker::IsPlayerAtHome() const
{
	return PlayerHouses::Instance().Contains(m_playerLocation);
}

void LocationTracker::RecordCurrentPlace(const float gameTime)
{
	VisitedPlaces::Instance().RecordNew(
		m_playerParentWorld, m_playerLocation, m_playerCell, gameTime);
}

bool LocationTracker::IsPlayerInLootablePlace(const RE::TESObjectCELL* cell, const bool lootableIfRestricted)
{
	RecursiveLockGuard guard(m_locationLock);
	// whitelist overrides all other considerations
	if (IsPlayerInWhitelistedPlace(cell))
	{
		DBG_DMESSAGE("Player location is on WhiteList");
		return true;
	}
	if (IsPlayerAtHome())
	{
		DBG_DMESSAGE("Player House: no looting");
		return false;
	}
	if (IsPlayerInBlacklistedPlace(cell))
	{
		DBG_DMESSAGE("Player location is on BlackList");
		return false;
	}
	if (!lootableIfRestricted && IsPlayerInRestrictedLootSettlement(cell))
	{
		DBG_DMESSAGE("Player location is restricted as population center");
		return false;
	}
	DBG_DMESSAGE("Player location {} OK to autoloot", m_playerLocation ? m_playerLocation->GetName() : "unnamed");
	return true;
}

// take a copy for thread safety
decltype(LocationTracker::m_adjacentCells) LocationTracker::AdjacentCells() const {
	RecursiveLockGuard guard(m_locationLock);
	return m_adjacentCells;
}

bool LocationTracker::IsAdjacent(RE::TESObjectCELL* cell) const
{
	// XCLC data available since both are exterior cells, by construction
	const auto checkCoordinates(cell->GetCoordinates());
	const auto myCoordinates(m_playerCell->GetCoordinates());
	return std::abs(myCoordinates->cellX - checkCoordinates->cellX) <= 1 &&
		std::abs(myCoordinates->cellY - checkCoordinates->cellY) <= 1;
}

// this is only called when we updated player-cell with a valid value
void LocationTracker::RecordAdjacentCells()
{
	m_adjacentCells.clear();

	// for exterior cells, also check directly adjacent cells for lootable goodies. Restrict to cells in the same worldspace.
	if (!m_playerCell->IsInteriorCell())
	{
		DBG_VMESSAGE("Check for adjacent cells to 0x{:08x}", m_playerCell->GetFormID());
		RE::TESWorldSpace* worldSpace(m_playerCell->worldSpace);
		if (worldSpace)
		{
			DBG_VMESSAGE("Worldspace is {}/0x{:08x}", worldSpace->GetName(), worldSpace->GetFormID());
			for (const auto& worldCell : worldSpace->cellMap)
			{
				RE::TESObjectCELL* candidateCell(worldCell.second);
				// skip player cell, handled above
				if (candidateCell == m_playerCell)
				{
					DBG_VMESSAGE("Player cell, already handled");
					continue;
				}
				// do not loot across interior/exterior boundary
				if (candidateCell->IsInteriorCell())
				{
					DBG_VMESSAGE("Candidate cell 0x{:08x} flagged as interior", candidateCell->GetFormID());
					continue;
				}
				// check for adjacency on the cell grid
				if (!IsAdjacent(candidateCell))
				{
					DBG_DMESSAGE("Skip non-adjacent cell 0x{:08x}", candidateCell->GetFormID());
					continue;
				}
				m_adjacentCells.push_back(candidateCell);
				DBG_VMESSAGE("Record adjacent cell 0x{:08x}", candidateCell->GetFormID());
			}
		}
	}
}

bool LocationTracker::IsPlayerIndoors() const
{
	RecursiveLockGuard guard(m_locationLock);
	return m_playerCell && m_playerCell->IsInteriorCell();
}

const RE::TESObjectCELL* LocationTracker::PlayerCell() const
{
	RecursiveLockGuard guard(m_locationLock);
	return m_playerCell && m_playerCell->IsAttached() ? m_playerCell : nullptr;
}

bool LocationTracker::IsPlayerInWhitelistedPlace(const RE::TESObjectCELL* cell) const
{
	RecursiveLockGuard guard(m_locationLock);
	// Player Location may be empty e.g. if we are in the wilderness
	// Player Cell should never be empty
	return ManagedList::WhiteList().Contains(m_playerLocation) || ManagedList::WhiteList().Contains(cell);
}

bool LocationTracker::IsPlayerInBlacklistedPlace(const RE::TESObjectCELL* cell) const
{
	RecursiveLockGuard guard(m_locationLock);
	// Player Location may be empty e.g. if we are in the wilderness
	// Player Cell should never be empty
	return ManagedList::BlackList().Contains(m_playerLocation) || ManagedList::BlackList().Contains(cell);
}

bool LocationTracker::IsPlayerInRestrictedLootSettlement(const RE::TESObjectCELL* cell) const
{
	RecursiveLockGuard guard(m_locationLock);
	if (!m_playerLocation)
		return false;
	// whitelist check done before we get called
	return PopulationCenters::Instance().CannotLoot(m_playerLocation);
}

bool LocationTracker::IsPlayerInFriendlyCell() const
{
	RecursiveLockGuard guard(m_locationLock);
	// Player Location may be empty e.g. if we are in the wilderness
	// Player Cell should never be empty
	CellOwnership ownership(GetCellOwnership(m_playerCell));
	bool isFriendly(IsPlayerFriendly(ownership));
	DBG_DMESSAGE("Cell ownership {}, allow Ownerless looting = {}", CellOwnershipName(ownership).c_str(), isFriendly ? "true" : "false");
	return isFriendly;
}

const RE::TESForm* LocationTracker::CurrentPlayerPlace() const
{
	// Prefer location, cell only if in wilderness
	RecursiveLockGuard guard(m_locationLock);
	if (m_playerLocation)
		return m_playerLocation;
	return m_playerCell;
}

const RE::TESWorldSpace* LocationTracker::CurrentPlayerWorld() const
{
	RecursiveLockGuard guard(m_locationLock);
	return m_playerParentWorld;
}

}