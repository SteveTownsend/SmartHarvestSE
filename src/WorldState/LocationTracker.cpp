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
#include "Data/SettingsCache.h"
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
	m_playerCellID(InvalidForm), m_playerCellX(0), m_playerCellY(0), m_playerIndoors(false),
	m_tellPlayerIfCanLootAfterLoad(false), m_playerLocation(nullptr), m_playerParentWorld(nullptr),
	m_aiRunning(false)
{
}

// called when Player moves to a new WorldSpace, including on game load/reload
void LocationTracker::RecordMarkedPlaces()
{
	// replace existing entries with new
	m_markedPlaces = AdventureTargets::Instance().GetWorldMarkedPlaces(m_playerParentWorld);
	if (m_markedPlaces.empty())
	{
		DBG_VMESSAGE("No map markers within this worldspace");
		return;
	}
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
	coordinates.setcontent(static_cast<alglib::ae_int_t>(numPlaces), 3, &pointData[0]);
	alglib::integer_1d_array tags;
	tags.setcontent(static_cast<alglib::ae_int_t>(numPlaces), &tagList[0]);

	// use Euclidean norm 2 for 3D space
	alglib::kdtreebuildtagged(coordinates, tags, static_cast<alglib::ae_int_t>(numPlaces), 3, 0, 2, m_markers);
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
	auto direction = directions.cbegin();
	while (direction != directions.cend() && degrees > direction->first)
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
	if (m_playerCellID == InvalidForm && !m_playerLocation)
		return " at unknown location";

	std::ostringstream locationStr;
	if (m_playerLocation)
	{
		locationStr << " at " << m_playerLocation->GetName() << "/0x" << StringUtils::FromFormID(m_playerLocation->GetFormID()) << ' ';
	}
	if (m_playerCellID != InvalidForm)
	{
		RE::TESObjectCELL* cell(RE::TESForm::LookupByID<RE::TESObjectCELL>(m_playerCellID));
		locationStr << " in Cell " << FormUtils::SafeGetFormEditorID(cell) << "/0x" << StringUtils::FromFormID(m_playerCellID);
	}
	return locationStr.str();
}

void LocationTracker::PrintPlayerLocation(const RE::BGSLocation* location) const
{
	const bool historic(false);
	std::string locationStr(NearbyLocationAsString(location, 0., CompassDirection::MAX, historic));
	if (!locationStr.empty())
	{
		RE::DebugNotification(locationStr.c_str());
	}
}

std::string LocationTracker::NearbyLocationAsString(
	const RE::BGSLocation* location, const double milesAway, CompassDirection heading, const bool historic) const
{
	RE::BSFixedString locationText(papyrus::GetTranslation(nullptr, historic ? RE::BSFixedString("$SHSE_WHERE_WAS_I") : RE::BSFixedString("$SHSE_WHERE_AM_I")));
	std::string locationMessage(locationText);
	if (!locationMessage.empty())
	{
		StringUtils::Replace(locationMessage, "{PROXIMITY}", Proximity(milesAway, heading));
		StringUtils::Replace(locationMessage, "{LOCATION}", location->GetName());
		StringUtils::Replace(locationMessage, "{VICINITY}", ParentLocationName(location));
	}
	return locationMessage;
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
	// Do not execute if already in process (Lesser Power spam)
	bool running(false);
	if (!m_aiRunning.compare_exchange_strong(running, true))
	{
		REL_MESSAGE("Adventurer's Instinct already running, ignore request");
		return;
	}
	running = true;

	// display location relative to Adventure Target, or nearest marker if no Adventure in progress
	const RE::BGSLocation* locationDone(nullptr);
	if (AdventureTargets::Instance().TargetLocation())
	{
		locationDone = PlayerLocationRelativeToAdventureTarget();
	}
	// print current location if it's not the same as the target
	PlayerLocationRelativeToNearestMapMarker(locationDone);

	if (!m_aiRunning.compare_exchange_strong(running, false))
	{
		REL_ERROR("Adventurer's Instinct reset failed");
		return;
	}
}

std::string LocationTracker::LocationRelativeToNearestMapMarker(const AlglibPosition& position, const bool historic) const
{
	std::string locationStr;
	RelativeLocationDescriptor nearestMarker(NearestMapMarker(position));
	if (nearestMarker.equals(RelativeLocationDescriptor::Invalid()))
	{
		REL_WARNING("Could not determine nearest map marker to position ({:0.2f}, {:0.2f})", position[0], position[1]);
		return locationStr;
	}
	CompassDirection heading(DirectionToDestinationFromStart(nearestMarker.EndPoint(), nearestMarker.StartPoint()));

	double milesAway(UnitsToMiles(nearestMarker.UnitsAway()));
	RE::BGSLocation* location(RE::TESForm::LookupByID<RE::BGSLocation>(nearestMarker.LocationID()));
	if (location)
	{
		DBG_MESSAGE("Position is {} of nearest Location map marker {}/0x{:08x} at distance {:0.2f} miles",
			CompassDirectionName(heading).c_str(), location->GetName(), nearestMarker.LocationID(), milesAway);
		locationStr = NearbyLocationAsString(location, milesAway, heading, historic);
	}
	else
	{
		REL_WARNING("Could not determine Location for 0x{:08x}", nearestMarker.LocationID());
	}
	return locationStr;
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
	if (m_playerLocation != locationDone)
	{
		static const bool historic(false);
		std::string locationStr(LocationRelativeToNearestMapMarker(playerPos, historic));
		if (!locationStr.empty())
		{
			RE::DebugNotification(locationStr.c_str());
		}
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
	DBG_MESSAGE("Nearest Map Marker to ({:0.2f},{:0.2f},{:0.2f}) has formID 0x{:08x}", refPos[0], refPos[1], refPos[2], location);

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
	double distance(sqrt((dx * dx) + (dy * dy) + (dz * dz)));
	DBG_MESSAGE("pos1({:0.2f},{:0.2f},{:0.2f}), pos2({:0.2f},{:0.2f},{:0.2f}), distance {:0.2f} units",
		pos1[0], pos1[1], pos1[2], pos2[0], pos2[1], pos2[2], distance);
	return distance;
}

RelativeLocationDescriptor LocationTracker::MarkedLocationPosition(
	const Position targetPosition, const RE::BGSLocation* location, const AlglibPosition& refPos) const
{
	AlglibPosition markerPos({ targetPosition[0], targetPosition[1], targetPosition[2] });
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
	Position targetPosition(AdventureTargets::Instance().TargetPosition());
	if (!location || targetPosition == InvalidPosition)
	{
		// no adventure in progress, or target unmappable
		return nullptr;
	}

	RecursiveLockGuard guard(m_locationLock);
	if (m_playerCellID == InvalidForm)
	{
		// setup in progress
		return nullptr;
	}
	// Adventure Target sensing only works outdoors
	if (m_playerIndoors)
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

	if (world != m_playerParentWorld)
	{
		PrintDifferentWorld(world);
		return nullptr;
	}
	AlglibPosition playerPos(PlayerState::Instance().GetAlglibPosition());
	RelativeLocationDescriptor targetLocation(MarkedLocationPosition(targetPosition, location, playerPos));
	if (targetLocation.equals(RelativeLocationDescriptor::Invalid()))
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
	RE::TESForm* owner = const_cast<RE::TESObjectCELL*>(cell)->GetOwner();
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
			DBG_VMESSAGE("GetCellOwner Hit 0x{:08x}", static_cast<const RE::ExtraOwnership&>(extraData).owner->formID);
			return static_cast<const RE::ExtraOwnership&>(extraData).owner;
		}
	}
	return nullptr;
}

void LocationTracker::Reset()
{
	DBG_MESSAGE("Reset Location Tracking after reload");
	RecursiveLockGuard guard(m_locationLock);
	m_tellPlayerIfCanLootAfterLoad = true;
	m_playerCellID = InvalidForm;
	m_playerIndoors = false;
	m_playerPlaceName.clear();
    m_playerCellX = 0;
    m_playerCellY = 0;
	m_adjacentCells.fill(nullptr);
	m_playerLocation = nullptr;
	m_playerParentWorld = nullptr;
	m_markedPlaces.clear();
}

const RE::TESWorldSpace* LocationTracker::ParentWorld(const RE::TESObjectCELL* cell)
{
	const RE::TESWorldSpace* world(cell->GetRuntimeData().worldSpace);
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
	// save current context
	const std::string originalPlaceName(m_playerPlaceName);
	const RE::FormID originalCellID(m_playerCellID);
	const RE::BGSLocation* originalLocation(m_playerLocation);

	const RE::TESObjectCELL* playerCell(RE::PlayerCharacter::GetSingleton()->parentCell);
	RE::FormID playerCellID(playerCell ? playerCell->GetFormID() : InvalidForm);
	bool indoorsNow(playerCell ? playerCell->IsInteriorCell() : false);
	if (playerCellID != originalCellID)
	{
		// CELL change only triggers visited-place update if we went from outdoors to indoors or vice versa
		playerMoved = originalCellID == InvalidForm || playerCellID == InvalidForm || m_playerIndoors != indoorsNow;
		m_playerCellID = playerCellID;
		m_playerIndoors = indoorsNow;
		if (playerCell)
		{
			if (m_playerIndoors)
			{
				DBG_MESSAGE("Player cell updated to 0x{:08x} indoors", m_playerCellID);
				m_playerCellX = 0;
				m_playerCellY = 0;
			}
			else
			{
				const auto playerCoordinates(const_cast<RE::TESObjectCELL*>(playerCell)->GetCoordinates());
				m_playerCellX = playerCoordinates->cellX;
				m_playerCellY = playerCoordinates->cellY;
				DBG_MESSAGE("Player cell updated to 0x{:08x} outdoors at ({},{})", m_playerCellID, m_playerCellX, m_playerCellY);
			}

			// player cell is valid - check for worldspace update
			const RE::TESWorldSpace* parentWorld(ParentWorld(playerCell));
			if (parentWorld && parentWorld != m_playerParentWorld)
			{
				m_playerParentWorld = parentWorld;
				DBG_MESSAGE("Player Parent WorldSpace updated to {}/0x{:08x}", m_playerParentWorld->GetName(), m_playerParentWorld->GetFormID());

				RecordMarkedPlaces();
			}

			RecordAdjacentCells(playerCell);
		}
		else
		{
			DBG_MESSAGE("Player cell cleared");
		}
		// Fire limited location change logic on cell update
		static const bool gameReload(false);
		PluginFacade::Instance().ResetTransientState(gameReload);
	}

	// Scan and Location tracking should not run if Player cell is not yet filled in
	if (m_playerCellID == InvalidForm)
		return false;

	const RE::BGSLocation* playerLocation(player->GetPlayerRuntimeData().currentLocation);
	if (m_playerCellID != originalCellID || playerLocation != originalLocation || m_tellPlayerIfCanLootAfterLoad)
	{
        // record all Location changes - Location may be blank now but we want to know we left a Location
		playerMoved = playerMoved || playerLocation != originalLocation || m_tellPlayerIfCanLootAfterLoad;

		// Output messages for any looting-restriction place change
		static const bool allowIfRestricted(false);
		bool couldLootInPrior(LocationTracker::Instance().IsPlaceLootable(originalCellID, originalLocation, allowIfRestricted, allowIfRestricted));
		m_playerLocation = playerLocation;
		m_playerPlaceName = PlaceName(CurrentPlayerPlaceCached());
		DBG_MESSAGE("Player was at {}, lootable = {}, now at {}", originalPlaceName,
			couldLootInPrior ? "true" : "false", m_playerPlaceName);

		// location change may mark the end of an in-progress 'Venture into the Unknown'
		AdventureTargets::Instance().CheckReachedCurrentDestination(m_playerLocation);

		// Player changed location - may be a standalone Cell with m_playerLocation nullptr e.g. 000HatredWell
		bool tellPlayer(SettingsCache::Instance().NotifyLocationChange());

		// check if new location/cell is a newly-visited player house
		if (!IsPlacePlayerHome(m_playerCellID, m_playerLocation))
		{
			if (m_playerLocation && PlayerHouses::Instance().IsValidHouseLocation(m_playerLocation))
			{
				// record Location as a player house and notify as it is a new one in this game load
				DBG_MESSAGE("Player House LCTN {}/0x{:08x} detected", m_playerLocation->GetName(), m_playerLocation->GetFormID());
				PlayerHouses::Instance().Add(m_playerLocation);
			}
			else if (m_playerCellID != InvalidForm && PlayerHouses::Instance().IsValidHouseCell(playerCell))
			{
				// record Cell as a player house and notify as it is a new one in this game load
				DBG_MESSAGE("Player House CELL {}/0x{:08x} detected", PlayerCell()->GetName(), m_playerCellID);
				PlayerHouses::Instance().AddCell(m_playerCellID);
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
					StringUtils::Replace(notificationText, "{HOUSENAME}", m_playerPlaceName);
					RE::DebugNotification(notificationText.c_str());
				}
			}
			// Check if location is excluded from looting and if so, notify we entered it
			if ((couldLootInPrior || m_tellPlayerIfCanLootAfterLoad) &&
				!LocationTracker::Instance().IsPlayerInLootablePlace(allowIfRestricted, allowIfRestricted))
			{
				DBG_MESSAGE("Player Location {} no-loot message", m_playerPlaceName);
				static RE::BSFixedString restrictedPlaceMsg(papyrus::GetTranslation(nullptr, RE::BSFixedString("$SHSE_POPULATED_CHECK")));
				if (!restrictedPlaceMsg.empty())
				{
					std::string notificationText(restrictedPlaceMsg);
					StringUtils::Replace(notificationText, "{LOCATIONNAME}", m_playerPlaceName);
					RE::DebugNotification(notificationText.c_str());
				}
			}
		}

		// Display messages about location auto-loot restrictions, if set in config
		if (tellPlayer)
		{
			// check if we moved from a non-lootable place to a lootable place
			if ((!couldLootInPrior || m_tellPlayerIfCanLootAfterLoad) &&
				LocationTracker::Instance().IsPlayerInLootablePlace(allowIfRestricted, allowIfRestricted))
			{
				DBG_MESSAGE("Player Location {} OK-loot message", originalPlaceName);
				static RE::BSFixedString unrestrictedPlaceMsg(papyrus::GetTranslation(nullptr, RE::BSFixedString("$SHSE_UNPOPULATED_CHECK")));
				if (!unrestrictedPlaceMsg.empty())
				{
					std::string notificationText(unrestrictedPlaceMsg);
					StringUtils::Replace(notificationText, "{LOCATIONNAME}", originalPlaceName);
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
	RecursiveLockGuard guard(m_locationLock);
	return IsPlacePlayerHome(m_playerCellID, m_playerLocation);
}

bool LocationTracker::IsPlacePlayerHome(const RE::FormID cellID, const RE::BGSLocation* location) const
{
	return PlayerHouses::Instance().Contains(location) || PlayerHouses::Instance().ContainsCell(cellID);
}

void LocationTracker::RecordCurrentPlace(const float gameTime)
{
	VisitedPlaces::Instance().RecordVisit(m_playerParentWorld, m_playerLocation, m_playerCellID,
		PlayerState::Instance().GetPosition(), gameTime);
}

bool LocationTracker::IsPlayerInLootablePlace(const bool lootableIfRestricted, const bool lootableIfRestrictedHome)
{
	RecursiveLockGuard guard(m_locationLock);
	return IsPlaceLootable(m_playerCellID, m_playerLocation, lootableIfRestricted, lootableIfRestrictedHome);
}

bool LocationTracker::IsPlaceLootable(
	const RE::FormID cellID, const RE::BGSLocation* location, const bool lootableIfRestricted, const bool lootableIfRestrictedHome)
{
	RecursiveLockGuard guard(m_locationLock);
	// whitelist overrides all other considerations
	DBG_DMESSAGE("Check autoloot for cell 0x{:08x}, location {}/0x{:08x}", cellID,
		location ? location->GetName() : "", location ? location->GetFormID() : InvalidForm);
	if (IsPlaceWhitelisted(cellID, location))
	{
		DBG_DMESSAGE("Player location is on WhiteList");
		return true;
	}
	if (!lootableIfRestrictedHome && IsPlacePlayerHome(cellID, location))
	{
		DBG_DMESSAGE("Player House: no looting");
		return false;
	}
	if (IsPlaceBlacklisted(cellID, location))
	{
		DBG_DMESSAGE("Player location is on BlackList");
		return false;
	}
	if (!lootableIfRestricted && IsPlaceRestrictedLootSettlement(cellID, location))
	{
		DBG_DMESSAGE("Player location is restricted as population center");
		return false;
	}
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
	const std::int32_t dx(abs(m_playerCellX - checkCoordinates->cellX));
	const std::int32_t dy(abs(m_playerCellY - checkCoordinates->cellY));
	if (dx <= 1 && dy <= 1)
	{
		DBG_MESSAGE("Cell 0x{:08x} at ({},{}) is adjacent to player", cell->GetFormID(), checkCoordinates->cellX, checkCoordinates->cellY);
		return true;
	}
	return false;
}

// this is only called when we updated player-cell with a valid value
void LocationTracker::RecordAdjacentCells(const RE::TESObjectCELL* current)
{
	m_adjacentCells.fill(nullptr);

	// For exterior cells, also check directly adjacent cells for lootable goodies.
	// Restrict to cells in the same worldspace, without walking parents.
	if (!m_playerIndoors)
	{
		DBG_VMESSAGE("Check for adjacent cells to 0x{:08x}", m_playerCellID);
		if (current->GetRuntimeData().worldSpace)
		{
			DBG_VMESSAGE("Worldspace is {}/0x{:08x}", current->GetRuntimeData().worldSpace->GetName(), current->GetRuntimeData().worldSpace->GetFormID());
			auto nextSlot = m_adjacentCells.begin();
			for (const auto& worldCell : current->GetRuntimeData().worldSpace->cellMap)
			{
				RE::TESObjectCELL* candidateCell(worldCell.second);
				// skip player cell, handled above
				if (candidateCell->GetFormID() == m_playerCellID)
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
				// check if adjacent to player CELL - do not record if either X/Y grid coordinate differs by more than 1
				if (IsAdjacent(candidateCell))
				{
					*nextSlot = candidateCell;
					++nextSlot;
				}
			}
		}
	}
}

bool LocationTracker::IsPlayerIndoors() const
{
	RecursiveLockGuard guard(m_locationLock);
	return m_playerIndoors;
}

// get current from game and check consistent with our stored state
RE::TESObjectCELL* LocationTracker::PlayerCell() const
{
	RecursiveLockGuard guard(m_locationLock);
	RE::TESObjectCELL* playerCell(RE::PlayerCharacter::GetSingleton()->parentCell);
	RE::FormID currentCellID(playerCell && playerCell->IsAttached() ? playerCell->GetFormID() : InvalidForm);
	return currentCellID == m_playerCellID ? playerCell : nullptr;
}

bool LocationTracker::IsPlayerInWhitelistedPlace() const
{
	RecursiveLockGuard guard(m_locationLock);
	return IsPlaceWhitelisted(m_playerCellID, m_playerLocation);
}

bool LocationTracker::IsPlaceWhitelisted(const RE::FormID cellID, const RE::BGSLocation* location) const
{
	RecursiveLockGuard guard(m_locationLock);
	// Player Location may be empty e.g. if we are in the wilderness
	// Player Cell should never be empty
	return ManagedList::WhiteList().Contains(location) || ManagedList::WhiteList().ContainsID(cellID);
}

bool LocationTracker::IsPlaceBlacklisted(const RE::FormID cellID, const RE::BGSLocation* location) const
{
	RecursiveLockGuard guard(m_locationLock);
	// Player Location may be empty e.g. if we are in the wilderness
	// Player Cell should never be empty
	return ManagedList::BlackList().Contains(location) || ManagedList::BlackList().ContainsID(cellID);
}

bool LocationTracker::IsPlayerInRestrictedLootSettlement() const
{
	RecursiveLockGuard guard(m_locationLock);
	// whitelist check done before we get called
	return IsPlaceRestrictedLootSettlement(m_playerCellID, m_playerLocation);
}

bool LocationTracker::IsPlaceRestrictedLootSettlement(const RE::FormID cellID, const RE::BGSLocation* location) const
{
	RecursiveLockGuard guard(m_locationLock);
	// whitelist check done before we get called
	return PopulationCenters::Instance().CannotLoot(cellID, location);
}

bool LocationTracker::IsPlayerInFriendlyCell() const
{
	RecursiveLockGuard guard(m_locationLock);
	// Player Location may be empty e.g. if we are in the wilderness
	// Player Cell should never be empty
	CellOwnership ownership(GetCellOwnership(PlayerCell()));
	bool isFriendly(IsPlayerFriendly(ownership));
	DBG_DMESSAGE("Cell ownership {}, allow Ownerless looting = {}", CellOwnershipName(ownership).c_str(), isFriendly ? "true" : "false");
	return isFriendly;
}

// Assumes lock held on entry
const RE::TESForm* LocationTracker::CurrentPlayerPlaceCached() const
{
	// Prefer location, cell only if in wilderness
	if (m_playerLocation)
		return m_playerLocation;
	return PlayerCell();
}

const RE::TESForm* LocationTracker::CurrentPlayerPlace()
{
	// Ensure current info is accurate and return the cached value
	RecursiveLockGuard guard(m_locationLock);
	Refresh();
	return CurrentPlayerPlaceCached();
}

std::string LocationTracker::PlaceName(const RE::TESForm* place) const
{
	std::string name;
	if (place)
	{
		name = place->GetName();
		if (name.empty())
		{
			name = StringUtils::FormIDString(place->GetFormID());
		}
	}
	return name.empty() ? "unnamed" : name;
}

const RE::TESWorldSpace* LocationTracker::CurrentPlayerWorld() const
{
	RecursiveLockGuard guard(m_locationLock);
	return m_playerParentWorld;
}

}