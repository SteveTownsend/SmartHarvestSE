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
#include "WorldState/GameCalendar.h"
#include "Data/DataCase.h"
#include "Data/LoadOrder.h"
#include "Data/SettingsCache.h"
#include "FormHelpers/FormHelper.h"
#include "WorldState/LocationTracker.h"
#include "VM/TaskDispatcher.h"
#include "Looting/ScanGovernor.h"
#include "Utilities/utils.h"

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
	m_harvestedIngredientMultiplier(1.),
	m_carryAdjustedForCombat(false),
	m_carryAdjustedForPlayerHome(false),
	m_carryAdjustedForDrawnWeapon(false),
	m_currentCarryWeightChange(0),
	m_sneaking(false),
	m_ownershipRule(OwnershipRule::MAX),
	m_belongingsCheck(SpecialObjectHandling::MAX),
	m_disableWhileMounted(false),
	m_gameTime(0.0)
{
}

double PlayerState::SneakDistanceInterior() const
{
	static const double SneakMaxDistance(utils::GetGameSettingFloat("fSneakMaxDistance"));
	return SneakMaxDistance;
}

double PlayerState::SneakDistanceExterior() const
{
	static const double SneakMaxDistanceExterior(utils::GetGameSettingFloat("fSneakExteriorDistanceMult") * SneakDistanceInterior());
	return SneakMaxDistanceExterior;
}

void PlayerState::Refresh(const bool onMCMPush, const bool onGameReload)
{
	// re-evaluate perks if timer has popped - force after game reload or settings update
	const bool force(onGameReload || onMCMPush);
	CheckPerks(force);

	if (onGameReload || onMCMPush)
	{
		// recache settings after resetting carry weight delta based on current
		ResetCarryWeight();
		SettingsCache::Instance().Refresh();
	}
	else
	{
		// adjust carry-weight delta if it does not reflect current settings
		AdjustCarryWeight();
	}

	// Check excess inventory - always check known item updates, full review periodically and on possible state changes
	// Do not process excess inventory if scanning is not allowed for any reason
	// Player may be trying to manually sell items or doing other stuff that does not favour inventory manipulation
	// per https://github.com/SteveTownsend/SmartHarvestSE/issues/252
	if (PluginFacade::Instance().ScanAllowed())
	{
		ReviewExcessInventory(onGameReload || onMCMPush);
	}

	// Update state cache if sneak state or settings may have changed. Affected REFRs were not blacklisted so we will recheck them on next pass.
	const bool sneaking(RE::PlayerCharacter::GetSingleton()->IsSneaking());
	if (onGameReload || onMCMPush || m_sneaking != sneaking)
	{
		m_sneaking = sneaking;
		// no player detection by NPC is a prerequisite for autoloot of crime-to-activate items
		m_ownershipRule = sneaking ? SettingsCache::Instance().CrimeCheckSneaking() : SettingsCache::Instance().CrimeCheckNotSneaking();
		m_belongingsCheck = SettingsCache::Instance().PlayerBelongingsLoot();
		// revisit ore-veins blocked while sneaking
		if (!m_sneaking && SettingsCache::Instance().DisallowMiningIfSneaking())
		{
			DataCase::GetInstance()->UnblockReferences(Lootability::CannotMineIfSneaking);
		}
	}
	auto target(RE::PlayerCharacter::GetSingleton()->As<RE::MagicTarget>());
	m_slowedTime = target && target->HasEffectWithArchetype(RE::EffectSetting::Archetype::kSlowTime);
	if (m_slowedTime)
	{
		DBG_DMESSAGE("Player subject to SlowTime archetype effect");
	}
	// Check for any other SlowTime lookalikes
	else if (DataCase::GetInstance()->IsSlowTimeEffectActive())
	{
		DBG_DMESSAGE("Player subject to indirect SlowTime effect");
		m_slowedTime = true;
	}
}

void PlayerState::ReviewExcessInventory(bool force)
{
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed(force ? "Review Excess Inventory Full" : "Review Excess Inventory Delta");
#endif
	RecursiveLockGuard guard(m_playerLock);

	// full resync if forced or last full scan was long enough ago that we want to sync up, otherwise check deltas
	constexpr std::chrono::milliseconds ExcessInventoryIntervalMillis(15000LL);
	const decltype(m_lastExcessCheck) cutoffPoint(std::chrono::high_resolution_clock::now() - ExcessInventoryIntervalMillis);
	if (m_lastExcessCheck <= cutoffPoint)
	{
		force = true;
	}
	// prepare delta updates for processing on this pass - local copy is ignored on full review (force = true)
	InventoryUpdates updates;
	if (force)
	{
		m_updates.clear();
	}
	else
	{
		updates.swap(m_updates);
	}

	if (force || !updates.empty())
	{
		const ContainerLister lister(INIFile::SecondaryType::deadbodies, RE::PlayerCharacter::GetSingleton());
		DBG_MESSAGE("Check Excess Inventory force={}, {} deltas", force, m_updates.size());
		InventoryCache validItems = lister.CacheIfExcessHandlingEnabled(force, updates);
		if (force)
		{
			m_currentItems.swap(validItems);
		}
		else
		{
			// validItems contains current data for each item - upsert to rolling cache
			std::for_each(validItems.cbegin(), validItems.cend(), [&] (const auto& current) {
				auto insertion = m_currentItems.insert(current);
				if (!insertion.second)
				{
					DBG_MESSAGE("Update existing cache for {}/0x{:08x}", current.first->GetName(), current.first->GetFormID());
					insertion.first->second = current.second;
				}
				else
				{
					DBG_MESSAGE("Added cache entry for {}/0x{:08x}", current.first->GetName(), current.first->GetFormID());
				}
			});
		}
		DBG_DMESSAGE("Cached {} inventory items", m_currentItems.size());

		// record time if this is a full scan
		if (updates.empty())
		{
			m_lastExcessCheck = std::chrono::high_resolution_clock::now();
		}
	}
}

void PlayerState::AdjustCarryWeight()
{
	bool managePlayerHome(SettingsCache::Instance().UnencumberedInPlayerHome());
	bool manageIfWeaponDrawn(SettingsCache::Instance().UnencumberedIfWeaponDrawn());
	bool manageDuringCombat(SettingsCache::Instance().UnencumberedInCombat());

	RecursiveLockGuard guard(m_playerLock);
	int carryWeightChange(m_currentCarryWeightChange);
	DBG_DMESSAGE("Carry weight existing delta {}", m_currentCarryWeightChange);

	// may need to reset if this is not in use at all
	if (!manageDuringCombat && !manageIfWeaponDrawn && !managePlayerHome)
	{
		DBG_DMESSAGE("Carry weight not managed, reset any current delta");
		carryWeightChange = 0;
	}

	if (managePlayerHome)
	{
		// when location changes to/from player house, adjust carry weight accordingly
		bool playerInOwnHouse(LocationTracker::Instance().IsPlayerAtHome());
		if (playerInOwnHouse != m_carryAdjustedForPlayerHome)
		{
			carryWeightChange += playerInOwnHouse ? InfiniteWeight : -InfiniteWeight;
			m_carryAdjustedForPlayerHome = playerInOwnHouse;
			DBG_MESSAGE("Carry weight delta after in-player-home adjustment {}", carryWeightChange);
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
			DBG_MESSAGE("Carry weight delta after in-combat adjustment {}", carryWeightChange);
		}
	}

	if (manageIfWeaponDrawn)
	{
		auto actorState(RE::PlayerCharacter::GetSingleton()->AsActorState());
		if (actorState)
		{
			DBG_VMESSAGE("Player WeaponState {}", static_cast<uint32_t>(actorState->GetWeaponState()));
			bool isWeaponDrawn(actorState->IsWeaponDrawn());
			// when state changes between drawn/sheathed, adjust carry weight accordingly
			if (isWeaponDrawn != m_carryAdjustedForDrawnWeapon)
			{
				carryWeightChange += isWeaponDrawn ? InfiniteWeight : -InfiniteWeight;
				m_carryAdjustedForDrawnWeapon = isWeaponDrawn;
				DBG_MESSAGE("Carry weight delta after drawn weapon adjustment {}", carryWeightChange);
			}
		}
	}

	if (carryWeightChange != m_currentCarryWeightChange)
	{
		int requiredWeightDelta(carryWeightChange - m_currentCarryWeightChange);
		m_currentCarryWeightChange = carryWeightChange;
		// upate Player's CarryWeight via Task Interface
		DBG_MESSAGE("Adjust carry weight by delta {}", requiredWeightDelta);
		TaskDispatcher::Instance().EnqueueCarryWeightDelta(requiredWeightDelta);
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

	if (SettingsCache::Instance().DisableDuringCombat() && RE::PlayerCharacter::GetSingleton()->IsInCombat())
	{
		DBG_VMESSAGE("Player in combat, skip");
		return false;
	}

	if (SettingsCache::Instance().DisableWhileWeaponIsDrawn())
	{
		auto actorState(RE::PlayerCharacter::GetSingleton()->As<RE::ActorState>());
		if (actorState && actorState->IsWeaponDrawn())
		{
			DBG_VMESSAGE("Player weapon is drawn, skip");
			return false;
		}
	}

	if (SettingsCache::Instance().DisableWhileConcealed() && IsMagicallyConcealed(player->As<RE::MagicTarget>()))
	{
		DBG_MESSAGE("Player is magically concealed, skip");
		return false;
	}

	if (FortuneHuntOnly())
	{
		DBG_MESSAGE("Player is a pure Fortune Hunter, skip");
		return false;
	}
	return true;
}

bool PlayerState::FortuneHuntOnly() const
{
	return SettingsCache::Instance().FortuneHuntItem() &&
		SettingsCache::Instance().FortuneHuntNPC() &&
		SettingsCache::Instance().FortuneHuntContainer();
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
			DBG_MESSAGE("Leveled items added on death by perks? {}", m_perksAddLeveledItemsOnDeath ? "true" : "false");
		}

		m_harvestedIngredientMultiplier = DataCase::GetInstance()->PerkIngredientMultiplier(player);
		DBG_VMESSAGE("Perk for harvesting -> multiplier {:0.2f}", m_harvestedIngredientMultiplier);

		m_lastPerkCheck = timeNow;
	}
}

// check perks that affect looting
bool PlayerState::PerksAddLeveledItemsOnDeath() const
{
	RecursiveLockGuard guard(m_playerLock);
	return m_perksAddLeveledItemsOnDeath;
}

float PlayerState::PerkIngredientMultiplier() const
{
	RecursiveLockGuard guard(m_playerLock);
	return m_harvestedIngredientMultiplier;
}

// reset carry weight adjustments - script will handle the Player Actor Value, scan will reinstate as needed when we resume
void PlayerState::ResetCarryWeight()
{
	bool managePlayerHome(SettingsCache::Instance().UnencumberedInPlayerHome());
	bool manageIfWeaponDrawn(SettingsCache::Instance().UnencumberedIfWeaponDrawn());
	bool manageDuringCombat(SettingsCache::Instance().UnencumberedInCombat());

	// reset delta to 0 if this is not in use at all
	if (manageDuringCombat || manageIfWeaponDrawn || managePlayerHome)
	{
		RecursiveLockGuard guard(m_playerLock);
		DBG_MESSAGE("Reset carry weight delta {}, in-player-home={}, in-combat={}, weapon-drawn={}", m_currentCarryWeightChange,
			m_carryAdjustedForPlayerHome ? "true" : "false", m_carryAdjustedForCombat ? "true" : "false", m_carryAdjustedForDrawnWeapon ? "true" : "false");
		m_carryAdjustedForCombat = false;
		m_carryAdjustedForPlayerHome = false;
		m_carryAdjustedForDrawnWeapon = false;
		if (m_currentCarryWeightChange != 0)
		{
			m_currentCarryWeightChange = 0;
			TaskDispatcher::Instance().EnqueueResetCarryWeight();
		}
	}
	else
	{
		DBG_VMESSAGE("Reset carry weight skipped, it's not managed");
	}
}

// used for PlayerCharacter
bool PlayerState::IsMagicallyConcealed(RE::MagicTarget* target) const
{
	if (!target)
		return false;
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

void PlayerState::ExcludeMountedIfForbidden(void)
{
	// check for 'Convenient Horses' in Load Order
	constexpr const char* convenientHorsesName("Convenient Horses.esp");
	if (shse::LoadOrder::Instance().ModPrecedesSHSE(convenientHorsesName))
	{
		REL_MESSAGE("Block looting while mounted: Convenient Horses loads before SHSE");
		m_disableWhileMounted = true;
	}
	else if (shse::LoadOrder::Instance().IncludesMod(convenientHorsesName))
	{
		REL_MESSAGE("SHSE handles looting while mounted: Convenient Horses loads after SHSE");
	}
}

Position PlayerState::GetPosition() const
{
	const auto player(RE::PlayerCharacter::GetSingleton());
	return { player->GetPositionX(), player->GetPositionY(), player->GetPositionZ() };
}

const RE::TESRace* PlayerState::GetRace() const
{
	const auto player(RE::PlayerCharacter::GetSingleton());
	return player->GetRace();
}

AlglibPosition PlayerState::GetAlglibPosition() const
{
	const auto player(RE::PlayerCharacter::GetSingleton());
	return { player->GetPositionX(), player->GetPositionY(), player->GetPositionZ() };
}

void PlayerState::UpdateGameTime()
{
	RecursiveLockGuard guard(m_playerLock);
	auto gameTime(RE::Calendar::GetSingleton()->GetCurrentGameTime());
	DBG_MESSAGE("GameTime is now {:0.3f}/{}", gameTime, GameCalendar::Instance().DateTimeString(gameTime));
	m_gameTime = gameTime;
}

double PlayerState::ArrowMovingThreshold() const
{
	// Moving arrows must be skipped if they are in flight. Bobbing on water or rolling around does not count.
	// Assume in-flight movement rate at least N=5 feet per second, scaled to in-game distance units and for loot scan interval.
	// https://github.com/SteveTownsend/SmartHarvestSE/issues/333 reduced this by a factor of 5 when time is slowed
	const double footUnitsPerDelay = SettingsCache::Instance().DelaySeconds() / DistanceUnitInFeet;	// 1 foot per second in units, for slow time
	return IsTimeSlowed() ? footUnitsPerDelay : 5.0 * footUnitsPerDelay;
}

// returns -1 if items are marked to be left behind and inventory is full, or the number allowed before limits are breached
int PlayerState::ItemHeadroom(RE::TESBoundObject* form, const int delta) const
{
	// if we add this, it will be as a new entry with 0 extant instances
	InventoryEntry itemEntry(form, 0);
	if (itemEntry.HandlingType() == ExcessInventoryHandling::NoLimits)
		return InventoryEntry::UnlimitedItems;

	// Inventory limit is configured - check if item is already being tracked
	auto cached(m_currentItems.find(form));
	if (cached == m_currentItems.end())
	{
		itemEntry.Populate();
		// add to cache if limited and not found, to keep cache accurate until next full review
		cached = m_currentItems.insert({ form, itemEntry }).first;
	}

	// Trigger delta reconciliation before next loot scan whenever we loot an item with inventory limits
	m_updates.insert(form);
	return cached->second.Headroom(delta);
}

}