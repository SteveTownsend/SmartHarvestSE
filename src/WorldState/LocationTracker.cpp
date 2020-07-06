#include "PrecompiledHeaders.h"
#include "WorldState/LocationTracker.h"
#include "Looting/ManagedLists.h"
#include "WorldState/PlayerHouses.h"
#include "WorldState/PopulationCenters.h"
#include "Looting/tasks.h"
#include "VM/papyrus.h"
#include "Utilities/utils.h"

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
	m_playerCell(nullptr), m_priorCell(nullptr), m_playerLocation(nullptr), m_tellPlayerIfCanLootAfterLoad(false)
{
}

bool LocationTracker::CellOwnedByPlayerOrPlayerFaction(const RE::TESObjectCELL* cell) const
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

RE::TESForm* LocationTracker::GetCellOwner(const RE::TESObjectCELL* cell) const
{
	for (const RE::BSExtraData& extraData : cell->extraList)
	{
		if (extraData.GetType() == RE::ExtraDataType::kOwnership)
		{
			DBG_VMESSAGE("GetCellOwner Hit %08x", reinterpret_cast<const RE::ExtraOwnership&>(extraData).owner->formID);
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
	m_playerLocation = nullptr;
}

// refresh state based on player's current position
bool LocationTracker::Refresh()
{
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
	RE::TESObjectCELL* playerCell(RE::PlayerCharacter::GetSingleton()->parentCell);
	if (playerCell != m_playerCell)
	{
		m_playerCell = playerCell;
		if (m_playerCell)
		{
			DBG_MESSAGE("Player cell updated to 0x%08x", m_playerCell->GetFormID());
		}
		else
		{
			DBG_MESSAGE("Player cell cleared");
		}
		// Fire limited location change logic on cell update
		static const bool gameReload(false);
		SearchTask::ResetRestrictions(gameReload);
	}

	// Scan and Location tracking should not run if Player cell is not yet filled in
	if (!m_playerCell)
		return false;

	const RE::BGSLocation* playerLocation(player->currentLocation);
	if (playerLocation != m_playerLocation || m_tellPlayerIfCanLootAfterLoad)
	{
		// Output messages for any looting-restriction place change
		static const bool allowIfRestricted(false);

		std::string oldName(m_playerLocation ? m_playerLocation->GetName() : "unnamed");
		bool couldLootInPrior(LocationTracker::Instance().IsPlayerInLootablePlace(m_priorCell, allowIfRestricted));
		DBG_MESSAGE("Player was at location %s, lootable = %s, now at %s", oldName.c_str(),
			couldLootInPrior ? "true" : "false", playerLocation ? playerLocation->GetName() : "unnamed");
		m_playerLocation = playerLocation;
		// Player changed location
		bool tellPlayer(INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "NotifyLocationChange") != 0);
		if (m_playerLocation)
		{
			// check if it is a new player house
			if (!IsPlayerAtHome())
			{
				if (PlayerHouses::Instance().IsValidHouse(m_playerLocation))
				{
					// record as a player house and notify as it is a new one in this game load
					DBG_MESSAGE("Player House %s detected", m_playerLocation->GetName());
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
					DBG_MESSAGE("Player Location %s no-loot message", m_playerLocation->GetName());
					static RE::BSFixedString restrictedPlaceMsg(papyrus::GetTranslation(nullptr, RE::BSFixedString("$SHSE_POPULATED_CHECK")));
					if (!restrictedPlaceMsg.empty())
					{
						std::string notificationText(restrictedPlaceMsg);
						StringUtils::Replace(notificationText, "{LOCATIONNAME}", m_playerLocation->GetName());
						RE::DebugNotification(notificationText.c_str());
					}
				}
			}
		}

		// Display messages about location auto-loot restrictions, if set in config
		if (tellPlayer)
		{
			// check if we moved from a non-lootable place to a lootable place
			if ((!couldLootInPrior || m_tellPlayerIfCanLootAfterLoad) && LocationTracker::Instance().IsPlayerInLootablePlace(PlayerCell(), allowIfRestricted))
			{
				DBG_MESSAGE("Player Location %s OK-loot message", oldName.c_str());
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
	return true;
}

bool LocationTracker::IsPlayerAtHome() const
{
	return PlayerHouses::Instance().Contains(m_playerLocation);
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
	if (CellOwnedByPlayerOrPlayerFaction(cell))
	{
		DBG_DMESSAGE("Player or Player Faction own cell: no looting");
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
	DBG_DMESSAGE("Player location %s OK to autoloot", m_playerLocation ? m_playerLocation->GetName() : "unnamed");
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

const RE::TESForm* LocationTracker::CurrentPlayerPlace() const
{
	// Prefer location, cell only if in wilderness
	if (m_playerLocation)
		return m_playerLocation;
	return m_playerCell;
}

