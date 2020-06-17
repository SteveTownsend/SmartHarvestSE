#include "PrecompiledHeaders.h"
#include "LocationTracker.h"
#include "ExcludedLocations.h"
#include "PlayerHouses.h"
#include "PopulationCenters.h"
#include "tasks.h"

std::unique_ptr<LocationTracker> LocationTracker::m_instance;

LocationTracker& LocationTracker::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<LocationTracker>();
	}
	return *m_instance;
}

LocationTracker::LocationTracker() : m_playerCell(nullptr), m_playerLocation(nullptr), m_playerCellSelfOwned(false)
{
}

bool LocationTracker::IsCellSelfOwned() const
{
	RecursiveLockGuard guard(m_locationLock);
	return m_playerCellSelfOwned;
}

RE::TESForm* LocationTracker::GetCellOwner(RE::TESObjectCELL* cell) const
{
	for (RE::BSExtraData& extraData : cell->extraList)
	{
		if (extraData.GetType() == RE::ExtraDataType::kOwnership)
		{
			DBG_VMESSAGE("GetCellOwner Hit %08x", reinterpret_cast<RE::ExtraOwnership&>(extraData).owner->formID);
			return reinterpret_cast<RE::ExtraOwnership&>(extraData).owner;
		}
	}
	return nullptr;
}

bool LocationTracker::IsCellPlayerOwned(RE::TESObjectCELL* cell) const
{
	if (!cell)
		return false;
	RE::TESForm* owner = GetCellOwner(cell);
	if (!owner)
		return false;
	if (owner->formType == RE::FormType::NPC)
	{
		const RE::TESNPC* npc = owner->As<RE::TESNPC>();
		RE::TESNPC* playerBase = RE::PlayerCharacter::GetSingleton()->GetActorBase();
		return (npc && npc == playerBase);
	}
	else if (owner->formType == RE::FormType::Faction)
	{
		RE::TESFaction* faction = owner->As<RE::TESFaction>();
		if (faction)
		{
			if (RE::PlayerCharacter::GetSingleton()->IsInFaction(faction))
				return true;

			return false;
		}
	}
	return false;
}

void LocationTracker::Reset(void)
{
	RecursiveLockGuard guard(m_locationLock);
	m_playerCell = nullptr;
	m_playerCellSelfOwned = false;
	m_playerLocation = nullptr;
}

// refresh state based on player's current position
void LocationTracker::Refresh()
{
	RE::PlayerCharacter* player(RE::PlayerCharacter::GetSingleton());
	if (!player)
	{
		DBG_MESSAGE("PlayerCharacter not available");
		return;
	}
	// handle player death. Obviously we are not looting on their behalf until a game reload or other resurrection event.
	// Assumes player non-essential: if player is in God mode a little extra carry weight or post-death looting is not
	// breaking immersion.
	if (player->IsDead(true))
	{
		// Fire location change logic
		static const bool gameReload(true);
		SearchTask::ResetRestrictions(gameReload);
		return;
	}

	RecursiveLockGuard guard(m_locationLock);
	RE::BGSLocation* playerLocation(player->currentLocation);
	if (playerLocation != m_playerLocation)
	{
		std::string oldName(m_playerLocation ? m_playerLocation->GetName() : "unnamed");
		DBG_MESSAGE("Player left old location %s, now at %s", oldName.c_str(), playerLocation ? playerLocation->GetName() : "unnamed");
		bool wasExcluded(LocationTracker::Instance().IsPlayerInRestrictedLootSettlement());
		m_playerLocation = playerLocation;
		// Player changed location
		if (m_playerLocation)
		{
			// check if it is a player house, and if so whether it is new
			if (!PlayerHouses::Instance().Contains(m_playerLocation))
			{
				if (PlayerHouses::Instance().IsValidHouse(m_playerLocation))
				{
					// record as a player house and notify as it is a new one in this game load
					DBG_MESSAGE("Player House %s detected", m_playerLocation->GetName());
					PlayerHouses::Instance().Add(m_playerLocation);
				}
			}
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
			// check if this is a population center excluded from looting and if so, notify we entered it
			if (LocationTracker::Instance().IsPlayerInRestrictedLootSettlement())
			{
				static RE::BSFixedString populationCenterMsg(papyrus::GetTranslation(nullptr, RE::BSFixedString("$SHSE_POPULATED_CHECK")));
				if (!populationCenterMsg.empty())
				{
					std::string notificationText(populationCenterMsg);
					StringUtils::Replace(notificationText, "{LOCATIONNAME}", m_playerLocation->GetName());
					RE::DebugNotification(notificationText.c_str());
				}
			}
		}
		// check if we moved from a non-lootable location to a free-loot zone
		if (wasExcluded && !LocationTracker::Instance().IsPlayerInRestrictedLootSettlement())
		{
			static RE::BSFixedString populationCenterMsg(papyrus::GetTranslation(nullptr, RE::BSFixedString("$SHSE_UNPOPULATED_CHECK")));
			if (!populationCenterMsg.empty())
			{
				std::string notificationText(populationCenterMsg);
				StringUtils::Replace(notificationText, "{LOCATIONNAME}", oldName.c_str());
				RE::DebugNotification(notificationText.c_str());
			}
		}
	}

	// Reset blocked lists if player cell has changed
	// Player cell should never be empty
	RE::TESObjectCELL* playerCell(RE::PlayerCharacter::GetSingleton()->parentCell);
	if (playerCell != m_playerCell)
	{
		m_playerCell = playerCell;
		m_playerCellSelfOwned = IsCellPlayerOwned(m_playerCell);
		if (m_playerCell)
		{
			DBG_MESSAGE("Player cell updated to 0x%08x", m_playerCell->GetFormID());
		}
		else
		{
			DBG_MESSAGE("Player cell cleared");
		}
		// Fire limited location change logic
		static const bool gameReload(false);
		SearchTask::ResetRestrictions(gameReload);
	}
}

bool LocationTracker::IsPlayerAtHome() const
{
	return PlayerHouses::Instance().Contains(m_playerLocation);
}

bool LocationTracker::IsPlayerInLootablePlace()
{
	if (IsPlayerAtHome())
	{
		DBG_VMESSAGE("Player House: no looting");
		return false;
	}
	RecursiveLockGuard guard(m_locationLock);
	if (!m_playerCell)
	{
		REL_WARNING("Player cell not yet set up");
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
	const auto myCoordinates(m_playerCell->GetCoordinates());
	return std::abs(myCoordinates->cellX - checkCoordinates->cellX) <= 1 &&
		std::abs(myCoordinates->cellY - checkCoordinates->cellY) <= 1;
}

void LocationTracker::RecordAdjacentCells(RE::TESObjectCELL* cell)
{
	// if this is the same cell we last checked, the list of adjacent cells does not need rebuilding
	if (m_playerCell == cell)
		return;

	m_playerCell = cell;
	m_adjacentCells.clear();

	// for exterior cells, also check directly adjacent cells for lootable goodies. Restrict to cells in the same worldspace.
	if (!m_playerCell->IsInteriorCell())
	{
		DBG_MESSAGE("Check for adjacent cells to 0x%08x", m_playerCell->GetFormID());
		RE::TESWorldSpace* worldSpace(m_playerCell->worldSpace);
		if (worldSpace)
		{
			DBG_MESSAGE("Worldspace is %s/0x%08x", worldSpace->GetName(), worldSpace->GetFormID());
			for (const auto& worldCell : worldSpace->cellMap)
			{
				RE::TESObjectCELL* candidateCell(worldCell.second);
				// skip player cell, handled above
				if (candidateCell == m_playerCell)
				{
					DBG_MESSAGE("Player cell, already handled");
					continue;
				}
				// do not loot across interior/exterior boundary
				if (candidateCell->IsInteriorCell())
				{
					DBG_MESSAGE("Candidate cell 0x%08x flagged as interior", candidateCell->GetFormID());
					continue;
				}
				// check for adjacency on the cell grid
				if (!IsAdjacent(candidateCell))
				{
					DBG_MESSAGE("Skip non-adjacent cell 0x%08x", candidateCell->GetFormID());
					continue;
				}
				m_adjacentCells.push_back(candidateCell);
				DBG_MESSAGE("Record adjacent cell 0x%08x", candidateCell->GetFormID());
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

bool LocationTracker::IsPlayerInBlacklistedPlace() const
{
	RecursiveLockGuard guard(m_locationLock);
	// Player Location may be empty e.g. if we are in the wilderness
	// Player Cell should never be empty
	return ExcludedLocations::Instance().Contains(m_playerLocation) || ExcludedLocations::Instance().Contains(m_playerCell);
}

bool LocationTracker::IsPlayerInRestrictedLootSettlement() const
{
	RecursiveLockGuard guard(m_locationLock);
	if (!m_playerLocation)
		return false;
	return PopulationCenters::Instance().CannotLoot(m_playerLocation);
}
