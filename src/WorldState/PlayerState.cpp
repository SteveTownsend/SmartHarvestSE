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

#include "WorldState/PlayerState.h"
#include "Data/DataCase.h"
#include "Data/LoadOrder.h"
#include "WorldState/LocationTracker.h"
#include "VM/EventPublisher.h"
#include "Looting/tasks.h"

namespace shse
{

std::unique_ptr<PlayerState> PlayerState::m_instance;

PlayerState& PlayerState::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<PlayerState>();
	}
	return *m_instance;
}

PlayerState::PlayerState() :
	m_perksAddLeveledItemsOnDeath(false),
	m_carryAdjustedForCombat(false),
	m_carryAdjustedForPlayerHome(false),
	m_carryAdjustedForDrawnWeapon(false),
	m_currentCarryWeightChange(0),
	m_sneaking(false),
	m_disableWhileMounted(false)
{
}

void PlayerState::Refresh()
{
	AdjustCarryWeight();

	// Reset blocked lists if sneak state has changed
	const bool sneaking(IsSneaking());
	if (m_sneaking != sneaking)
	{
		m_sneaking = sneaking;
		static const bool gameReload(false);
		SearchTask::ResetRestrictions(gameReload);
	}
}

void PlayerState::AdjustCarryWeight()
{
	INIFile* settings(INIFile::GetInstance());
	bool managePlayerHome(settings->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "UnencumberedInPlayerHome") != 0.0);
	bool manageIfWeaponDrawn(settings->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "UnencumberedIfWeaponDrawn") != 0.0);
	bool manageDuringCombat(settings->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "UnencumberedInCombat") != 0.0);
	// no op if this is not in use at all
	if (!manageDuringCombat && !manageIfWeaponDrawn && !managePlayerHome)
	{
		DBG_DMESSAGE("Carry weight not managed, skip checks");
		return;
	}

	RecursiveLockGuard guard(m_playerLock);
	int carryWeightChange(m_currentCarryWeightChange);
	if (managePlayerHome)
	{
		// when location changes to/from player house, adjust carry weight accordingly
		bool playerInOwnHouse(LocationTracker::Instance().IsPlayerAtHome());
		if (playerInOwnHouse != m_carryAdjustedForPlayerHome)
		{
			carryWeightChange += playerInOwnHouse ? InfiniteWeight : -InfiniteWeight;
			m_carryAdjustedForPlayerHome = playerInOwnHouse;
			DBG_MESSAGE("Carry weight delta after in-player-home adjustment %d", carryWeightChange);
		}
	}

	if (manageDuringCombat)
	{
	    bool playerInCombat(RE::PlayerCharacter::GetSingleton()->IsInCombat() && !RE::PlayerCharacter::GetSingleton()->IsDead(true));
		// when state changes in/out of combat, adjust carry weight accordingly
		if (playerInCombat != m_carryAdjustedForCombat)
		{
			carryWeightChange += playerInCombat ? InfiniteWeight : -InfiniteWeight;
			m_carryAdjustedForCombat = playerInCombat;
			DBG_MESSAGE("Carry weight delta after in-combat adjustment %d", carryWeightChange);
		}
	}

	if (manageIfWeaponDrawn)
	{
	    bool isWeaponDrawn(RE::PlayerCharacter::GetSingleton()->IsWeaponDrawn());
		// when state changes between drawn/sheathed, adjust carry weight accordingly
		if (isWeaponDrawn != m_carryAdjustedForDrawnWeapon)
		{
			carryWeightChange += isWeaponDrawn ? InfiniteWeight : -InfiniteWeight;
			m_carryAdjustedForDrawnWeapon = isWeaponDrawn;
			DBG_MESSAGE("Carry weight delta after drawn weapon adjustment %d", carryWeightChange);
		}
	}
	if (carryWeightChange != m_currentCarryWeightChange)
	{
		int requiredWeightDelta(carryWeightChange - m_currentCarryWeightChange);
		m_currentCarryWeightChange = carryWeightChange;
		// handle carry weight update via a script event
		DBG_MESSAGE("Adjust carry weight by delta %d", requiredWeightDelta);
		EventPublisher::Instance().TriggerCarryWeightDelta(requiredWeightDelta);
	}
}

bool PlayerState::CanLoot() const
{
	// disable auto-looting if we are inside player house - player 'current location' may be validly empty
	RE::PlayerCharacter* player(RE::PlayerCharacter::GetSingleton());
	if (!player)
	{
		DBG_MESSAGE("PlayerCharacter not available");
		return false;
	}
	if (player->IsDead(true))
	{
		DBG_MESSAGE("Player is dead");
		return false;
	}

	RecursiveLockGuard guard(m_playerLock);
	if (m_disableWhileMounted && player->IsOnMount())
	{
		DBG_MESSAGE("Player is mounted, but mounted autoloot forbidden");
		return false;
	}

	INIFile* settings(INIFile::GetInstance());
	const int disableDuringCombat = static_cast<int>(settings->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "disableDuringCombat"));
	if (disableDuringCombat != 0 && RE::PlayerCharacter::GetSingleton()->IsInCombat())
	{
		DBG_VMESSAGE("Player in combat, skip");
		return false;
	}

	const int disableWhileWeaponIsDrawn = static_cast<int>(settings->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "disableWhileWeaponIsDrawn"));
	if (disableWhileWeaponIsDrawn != 0 && player->IsWeaponDrawn())
	{
		DBG_VMESSAGE("Player weapon is drawn, skip");
		return false;
	}

	const int disableWhileConcealed = static_cast<int>(settings->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "DisableWhileConcealed"));
	if (disableWhileConcealed != 0 && IsMagicallyConcealed(player))
	{
		DBG_MESSAGE("Player is magically concealed, skip");
		return false;
	}
	return true;
}

// check perks that affect looting
void PlayerState::CheckPerks(const bool force)
{
	RecursiveLockGuard guard(m_playerLock);

	const auto timeNow(std::chrono::high_resolution_clock::now());
	const auto cutoffPoint(timeNow - std::chrono::milliseconds(static_cast<long long>(PerkCheckIntervalSeconds * 1000.0)));
	if (force || m_lastPerkCheck <= cutoffPoint)
	{
		m_perksAddLeveledItemsOnDeath = false;
		auto player(RE::PlayerCharacter::GetSingleton());
		if (player)
		{
			m_perksAddLeveledItemsOnDeath = DataCase::GetInstance()->PerksAddLeveledItemsOnDeath(player);
			DBG_MESSAGE("Leveled items added on death by perks? %s", m_perksAddLeveledItemsOnDeath ? "true" : "false");
		}
		m_lastPerkCheck = timeNow;
	}
}

// check perks that affect looting
bool PlayerState::PerksAddLeveledItemsOnDeath() const
{
	RecursiveLockGuard guard(m_playerLock);
	return m_perksAddLeveledItemsOnDeath;
}

// reset carry weight adjustments - scripts will handle the Player Actor Value, scan will reinstate as needed when we resume
void PlayerState::ResetCarryWeight(const bool reloaded)
{
	INIFile* settings(INIFile::GetInstance());
	bool managePlayerHome(settings->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "UnencumberedInPlayerHome") != 0.0);
	bool manageIfWeaponDrawn(settings->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "UnencumberedIfWeaponDrawn") != 0.0);
	bool manageDuringCombat(settings->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "UnencumberedInCombat") != 0.0);
	// do not adjust if this is not in use at all
	if (manageDuringCombat || manageIfWeaponDrawn || managePlayerHome)
	{
		RecursiveLockGuard guard(m_playerLock);
		DBG_MESSAGE("Reset carry weight delta %d, in-player-home=%s, in-combat=%s, weapon-drawn=%s", m_currentCarryWeightChange,
			m_carryAdjustedForPlayerHome ? "true" : "false", m_carryAdjustedForCombat ? "true" : "false", m_carryAdjustedForDrawnWeapon ? "true" : "false");
		m_carryAdjustedForCombat = false;
		m_carryAdjustedForPlayerHome = false;
		m_carryAdjustedForDrawnWeapon = false;
		if (m_currentCarryWeightChange != 0)
		{
			m_currentCarryWeightChange = 0;
			EventPublisher::Instance().TriggerResetCarryWeight();
		}
	}
	else
	{
		DBG_VMESSAGE("Reset carry weight skipped, it's not managed");
	}

	// reset location to force proper recalculation
	// TODO is this still needed?
	if (reloaded)
	{
		LocationTracker::Instance().Reset();
	}
}

// used for PlayerCharacter
bool PlayerState::IsMagicallyConcealed(RE::MagicTarget* target) const
{
	if (target->HasEffectWithArchetype(RE::EffectArchetypes::ArchetypeID::kInvisibility))
	{
		DBG_VMESSAGE("player invisible");
		return true;
	}
	if (target->HasEffectWithArchetype(RE::EffectArchetypes::ArchetypeID::kEtherealize))
	{
		DBG_VMESSAGE("player ethereal");
		return true;
	}
	return false;
}

bool PlayerState::IsSneaking() const
{
	return RE::PlayerCharacter::GetSingleton()->IsSneaking();
}

void PlayerState::ExcludeMountedIfForbidden(void)
{
	// check for 'Convenient Horses' in Load Order
	if (shse::LoadOrder::Instance().IncludesMod("Convenient Horses.esp"))
	{
		REL_MESSAGE("Block looting while mounted: Convenient Horses is active");
		m_disableWhileMounted = true;
	}
}

Position PlayerState::GetPosition() const
{
	const auto player(RE::PlayerCharacter::GetSingleton());
	return { player->GetPositionX(), player->GetPositionY(), player->GetPositionZ() };
}

AlglibPosition PlayerState::GetAlglibPosition() const
{
	const auto player(RE::PlayerCharacter::GetSingleton());
	return { player->GetPositionX(), player->GetPositionY(), player->GetPositionZ() };
}

}