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
#include "Looting/TryLootREFR.h"
#include "Looting/ContainerLister.h"
#include "Looting/LootableREFR.h"
#include "Looting/ManagedLists.h"
#include "Looting/objects.h"
#include "Looting/ScanGovernor.h"
#include "Looting/TheftCoordinator.h"
#include "Collections/CollectionManager.h"
#include "Data/dataCase.h"
#include "Data/SettingsCache.h"
#include "FormHelpers/FormHelper.h"
#include "VM/EventPublisher.h"
#include "Utilities/utils.h"
#include "WorldState/PlayerState.h"

namespace shse
{

TryLootREFR::TryLootREFR(RE::TESObjectREFR* target, INIFile::SecondaryType targetType,
	const bool stolen, const bool glowOnly, const bool forceHarvest)
	: m_stolen(stolen), m_glowOnly(glowOnly), m_forceHarvest(forceHarvest),
	m_candidate(target), m_targetType(targetType), m_glowReason(GlowReason::None)
{
}

void TryLootREFR::TryForceHarvest(RE::TESObjectREFR* refr)
{
	static const bool stolen(false);
	static const bool glowOnly(false);
	// Bypass Quest Target checking at user's option, for loose items e.g Books so they are not opened when taken
	static const bool forceHarvest(true);
	static const bool dryRun(false);
	TryLootREFR(refr, INIFile::SecondaryType::itemObjects, stolen, glowOnly, forceHarvest).Process(dryRun);
}

Lootability TryLootREFR::Process(const bool dryRun)
{
	if (!m_candidate)
		return Lootability::NullReference;

	DataCase* data = DataCase::GetInstance();
	Lootability result(Lootability::Lootable);
	if (m_targetType == INIFile::SecondaryType::itemObjects)
	{
		LootableREFR refrEx(m_candidate, m_targetType);
		ObjectType objType = refrEx.GetObjectType();
		m_typeName = refrEx.GetTypeName();
		// Various form types contain an ingredient or FormList that is the final lootable item
		if (!dryRun && refrEx.IsHarvestable())
		{
			RE::TESBoundObject* lootable(ProducerLootables::Instance().GetLootableForProducer(m_candidate->GetBaseObject()));
			if (lootable)
			{
				DBG_VMESSAGE("producer {}/0x{:08x} has lootable {}/0x{:08x} of type {}", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID,
					lootable->GetName(), lootable->formID, GetObjectTypeName(objType));
			}
			// resolve here if critter and not yet known
			else if (refrEx.IsCritter())
			{
				// trigger critter -> ingredient resolution and skip until it's resolved - pending resolve recorded using nullptr,
				// only trigger if not already pending
				DBG_VMESSAGE("resolve critter {}/0x{:08x} to ingredient", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
				if (ProducerLootables::Instance().SetLootableForProducer(m_candidate->GetBaseObject(), nullptr))
				{
					EventPublisher::Instance().TriggerGetProducerLootable(m_candidate);
				}
				return Lootability::PendingProducerIngredient;
			}
			else if (refrEx.IsFlora())
			{
				DBG_VMESSAGE("flora {}/0x{:08x} has lootable {}/0x{:08x} of type {}", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID,
					refrEx.GetTarget()->GetName(), refrEx.GetTarget()->formID, GetObjectTypeName(refrEx.GetObjectType()));
			}
			else
			{
				REL_WARNING("Non-scripted Producer {}/0x{:08x} has no lootable", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
				DataCase::GetInstance()->BlockForm(m_candidate->GetBaseObject(), Lootability::ProducerHasNoLootable);
				return Lootability::ProducerHasNoLootable;
			}
		}

		// initially no glow - use synthetic value with highest precedence
		m_glowReason = GlowReason::None;
		bool skipLooting(false);

		// TODO this may update state on a dry run but we should already have processed the item on >= 1 pass, so no harm?
		// Check Collections first in case there are Manual Loot items that do not have an objectType, esp. scripted ACTI
		auto collectible(refrEx.TreatAsCollectible());
		if (collectible.first)
		{
			CollectibleHandling collectibleAction(collectible.second);
			if (m_glowOnly && CanLootCollectible(collectibleAction))
			{
				collectibleAction = CollectibleHandling::Glow;
			}
			DBG_VMESSAGE("Collectible Item 0x{:08x}", m_candidate->GetBaseObject()->formID);
			if (!CanLootCollectible(collectibleAction))
			{
				// ignore collectibility from here on, since we've determined it is unlootable
				collectible.first = false;
				skipLooting = true;
				if (collectibleAction == CollectibleHandling::Print)
				{
					if (!dryRun)
					{
						// print message about loose REFR
						ProcessManualLootREFR(m_candidate);
					}
					//we do not want to blacklist the base object even if it's not a proper objectType
					return Lootability::ManualLootTarget;
				}
				else if (collectibleAction == CollectibleHandling::Glow)
				{
					DBG_VMESSAGE("glow collectible object {}/0x{:08x}", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
					UpdateGlowReason(GlowReason::Collectible);
					result = Lootability::CollectibleItemSetToGlow;
				}
				else
				{
					if (!dryRun)
					{
						// this is a blacklist collection, blacklist the item forever
						DBG_VMESSAGE("block blacklist collection member 0x{:08x}", m_candidate->GetBaseObject()->formID);
						data->BlockFormPermanently(m_candidate->GetBaseObject(), Lootability::ObjectIsInBlacklistCollection);
					}
					return Lootability::ObjectIsInBlacklistCollection;
				}
			}
		}

		if (objType == ObjectType::unknown)
		{
			if (!dryRun)
			{
				DBG_VMESSAGE("blacklist objType == ObjectType::unknown for 0x{:08x}", m_candidate->GetFormID());
				data->BlacklistReference(m_candidate);
			}
			return Lootability::ObjectTypeUnknown;
		}

		if (ManagedList::BlackList().Contains(m_candidate->GetBaseObject()))
		{
			DBG_VMESSAGE("Skip BlackListed REFR base form 0x{:08x}", m_candidate->GetBaseObject()->GetFormID());
			return Lootability::BaseObjectOnBlacklist;
		}

		if (IsQuestItem(m_candidate))
		{
			QuestObjectHandling questObjectLoot = SettingsCache::Instance().QuestObjectLoot();
			DBG_VMESSAGE("Quest Item 0x{:08x}", m_candidate->GetBaseObject()->formID);
			if (m_glowOnly)
			{
				questObjectLoot = QuestObjectHandling::GlowTarget;
			}
			if (questObjectLoot == QuestObjectHandling::GlowTarget)
			{
				DBG_VMESSAGE("glow quest object {}/0x{:08x}", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
				UpdateGlowReason(GlowReason::QuestObject);
			}

			skipLooting = true;

			// ignore collectibility from here on, since we've determined it is unlootable as a Quest Target
			collectible.first = false;
			result = Lootability::CannotLootQuestTarget;
		}
		// glow unread notes as they are often quest-related
		else if (objType == ObjectType::book)
		{
			QuestObjectHandling questObjectLoot = SettingsCache::Instance().QuestObjectLoot();
			if (questObjectLoot == QuestObjectHandling::GlowTarget && IsBookGlowable())
			{
				DBG_VMESSAGE("Glowable book 0x{:08x}", m_candidate->GetBaseObject()->formID);
				UpdateGlowReason(GlowReason::SimpleTarget);
			}
		}

		EnchantedObjectHandling enchantedLoot = SettingsCache::Instance().EnchantedObjectHandlingType();
		ObjectType newType = TESFormHelper::EnchantedREFREffectiveType(m_candidate, objType, enchantedLoot);
		if (TypeIsEnchanted(newType))
		{
			DBG_VMESSAGE("Loose Enchanted Item {}/0x{:08x}", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
			if (m_glowOnly)
			{
				enchantedLoot = EnchantedObjectHandling::GlowTarget;
			}
			if (enchantedLoot == EnchantedObjectHandling::GlowTarget || enchantedLoot == EnchantedObjectHandling::GlowTargetUnknown)
			{
				DBG_VMESSAGE("glow enchanted object {}/0x{:08x}", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
				UpdateGlowReason(GlowReason::EnchantedItem);
			}

			if (!IsEnchantedObjectLootable(enchantedLoot))
			{
				skipLooting = true;
				// in this case, Collectibility can override the decision
				result = Lootability::CannotLootEnchantedObject;
			}
		}

		// Enchanted items may be treated as unenchanted if the enchantment is known
		if (newType != objType)
		{
			objType = newType;
			m_typeName = GetObjectTypeName(newType);
			refrEx.SetEffectiveObjectType(newType);
		}

		if (refrEx.IsValuable())
		{
			SpecialObjectHandling valuableLoot = SettingsCache::Instance().ValuableItemLoot();
			DBG_VMESSAGE("Valuable Item 0x{:08x}", m_candidate->GetBaseObject()->formID);
			if (m_glowOnly)
			{
				valuableLoot = SpecialObjectHandling::GlowTarget;
			}
			if (valuableLoot == SpecialObjectHandling::GlowTarget)
			{
				DBG_VMESSAGE("glow valuable object {}/0x{:08x}", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
				UpdateGlowReason(GlowReason::Valuable);
			}
			if (!IsSpecialObjectLootable(valuableLoot))
			{
				skipLooting = true;
				// in this case, Collectibility can override the decision
				result = Lootability::CannotLootValuableObject;
			}
		}

		if (objType == ObjectType::ammo)
		{
			if (data->SkipAmmoLooting(m_candidate))
			{
				skipLooting = true;
				result = Lootability::CannotLootAmmo;
			}
		}

		// Special case ingredient handling - we may be asked to always pick up ingredients with Unknown effects, overriding 'ingredient' setting
		bool forceIngredientLoot = false;
		if (objType == ObjectType::ingredient)
		{
			forceIngredientLoot = SettingsCache::Instance().UnknownIngredientLoot() &&
				!DataCase::GetInstance()->IsIngredientKnown(refrEx.GetLootable());
			if (forceIngredientLoot)
			{
				DBG_VMESSAGE("force pickup of unknown effect ingredient {}/0x{:08x}", refrEx.GetLootable()->GetName(), refrEx.GetLootable()->GetFormID());
			}
		}


		// Check if this is an item harvestable in player home - this setting ignores player-ownership for flora, critters etc
		bool atHome(LocationTracker::Instance().IsPlayerAtHome());
		bool allowHarvestAtHome(SettingsCache::Instance().LootAllowedItemsInPlayerHouse() &&
			refrEx.IsItemLootableInPlayerHouse(objType));

		// Order is important to ensure we glow correctly even if blocked. Collectibility may override the initial result.
		Lootability forbidden(atHome && allowHarvestAtHome ?
			Lootability::Lootable : ItemLootingLegality(collectible.first, m_targetType));
		if (forbidden != Lootability::Lootable)
		{
			skipLooting = true;
			result = forbidden;
		}

		// Force-Harvest hotkey is used to pick up loose Quest Targets, at user's discretion and own risk
		if (!dryRun && !m_forceHarvest && m_glowReason != GlowReason::None)
		{
			ScanGovernor::Instance().GlowObject(m_candidate, ObjectGlowDurationSpecialSeconds, m_glowReason);
			// further checks redundant if we have a glowable target
			if (m_glowOnly)
			{
				return result;
			}
		}

		// Harvesting and mining is allowed in settlements. We really just want to not auto-loot entire
		// buildings of friendly factions, and the like. Mines and farms mostly self-identify as Settlements.
		// Harvesting in player house is a special-case.
		bool whitelistedPlace(LocationTracker::Instance().IsPlayerInWhitelistedPlace());
		if ((!atHome || !allowHarvestAtHome) && !whitelistedPlace &&
			LocationTracker::Instance().IsPlayerInRestrictedLootSettlement() &&
			!refrEx.IsItemLootableInPopulationCenter(objType))
		{
			DBG_VMESSAGE("Player location is excluded as restricted population center for this item");
			result = Lootability::PopulousLocationRestrictsLooting;
			skipLooting = true;
		}
		if (atHome && !allowHarvestAtHome && !whitelistedPlace)
		{
			DBG_VMESSAGE("Player House is excluded for this item");
			result = Lootability::PlayerHouseRestrictsLooting;
			skipLooting = true;
		}

		LootingType lootingType(LootingType::LeaveBehind);
		bool whitelisted(false);
		if (collectible.first)
		{
			CollectibleHandling collectibleAction(collectible.second);
			// ** if configured as permitted ** collectible objects are always looted silently
			if (CanLootCollectible(collectibleAction))
			{
				skipLooting = forbidden != Lootability::Lootable;
				DBG_VMESSAGE("Lootable REFR to collectible 0x{:08x}, skip = {}", m_candidate->GetBaseObject()->formID,
				    skipLooting ? "true" : "false");
				lootingType = LootingType::LootAlwaysSilent;
			}
			else
			{
				DBG_VMESSAGE("Unlootable REFR to collectible 0x{:08x}", m_candidate->GetBaseObject()->formID);
				skipLooting = true;
			}
		}
		else if (ManagedList::WhiteList().Contains(m_candidate->GetBaseObject()))
		{
			// ** if configured as permitted ** whitelisted objects are always looted silently
			DBG_VMESSAGE("check REFR 0x{:08x} to whitelisted {}/0x{:08x}",
				m_candidate->GetFormID(), m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
			skipLooting = forbidden != Lootability::Lootable;
			if (skipLooting)
			{
				result = Lootability::LawAbidingSoNoWhitelistItemLooting;
			}
			whitelisted = true;
			lootingType = LootingType::LootAlwaysSilent;
		}
		else if (ManagedList::BlackList().Contains(m_candidate->GetBaseObject()))
		{
			// blacklisted objects are never looted
			DBG_VMESSAGE("disallow blacklisted Base {}/0x{:08x} for REFR 0x{:08x}",
				m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->GetFormID(), m_candidate->GetFormID());
			skipLooting = true;
			result = Lootability::ItemIsBlacklisted;
			lootingType = LootingType::LeaveBehind;
		}
		else if (!skipLooting)
		{
			// check if final output of harvest is lootable
			lootingType = forceIngredientLoot ? LootingType::LootAlwaysNotify : SettingsCache::Instance().ObjectLootingType(objType);
			if (lootingType == LootingType::LeaveBehind)
			{
				if (!dryRun)
				{
					DBG_VMESSAGE("Block REFR : LeaveBehind for 0x{:08x}", m_candidate->GetBaseObject()->formID);
					data->BlockReference(m_candidate, Lootability::ItemTypeIsSetToPreventLooting);
				}
				skipLooting = true;
				result = Lootability::ItemTypeIsSetToPreventLooting;
			}
			else if (refrEx.HarvestForbiddenForForm())
			{
				// check if harvest is forbidden for Base Object, irrespective of final harvested item
				if (!dryRun)
				{
					DBG_VMESSAGE("REFR 0x{:08x} with type {} has unharvestable Base Object {}/0x{:08x}", m_candidate->GetFormID(), m_typeName,
						m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->GetFormID());
					data->BlockReference(m_candidate, Lootability::HarvestDisallowedForBaseObjectType);
				}
				skipLooting = true;
				result = Lootability::HarvestDisallowedForBaseObjectType;
			}
			else if (!forceIngredientLoot && LootingDependsOnValueWeight(lootingType, objType))
			{
				if (refrEx.ValueWeightTooLowToLoot())
				{
					if (!dryRun)
					{
						DBG_VMESSAGE("block - v/w excludes harvest for 0x{:08x}", m_candidate->GetBaseObject()->formID);
						data->BlockForm(m_candidate->GetBaseObject(), Lootability::ValueWeightPreventsLooting);
					}
					skipLooting = true;
					result = Lootability::ValueWeightPreventsLooting;
				}
				DBG_VMESSAGE("{}/0x{:08x} value:{}", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID, int(refrEx.GetWorth()));
			}
		}

		// Force-Harvest hotkey is used to pick up loose Quest Targets, at user's discretion and own risk
		if (m_forceHarvest)
		{
			DBG_VMESSAGE("Item force-harvest by user {}/0x{:08x}", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
		}
		else
		{
			if (skipLooting || dryRun)
				return result;

			// we would loot this - glow and exit if we are using Loot Sense
			if (m_glowOnly)
			{
				ScanGovernor::Instance().GlowObject(m_candidate, ObjectGlowDurationSpecialSeconds, GlowReason::SimpleTarget);
				return result;
			}

			// Check if we should attempt to steal the item. If we skip it due to looting rules, it's immune from stealing.
			// If we wish to auto-steal an item we must check we are not detected, which requires a scripted check. If this
			// is the delayed autoloot operation after we find we are undetected, don't trigger that check again here.
			if (!m_stolen && m_candidate->IsOffLimits() &&
				PlayerState::Instance().EffectiveOwnershipRule() == OwnershipRule::AllowCrimeIfUndetected)
			{
				DBG_VMESSAGE("REFR to be stolen if undetected");
				TheftCoordinator::Instance().DelayStealableItem(m_candidate, m_targetType);
				return Lootability::ItemTheftTriggered;
			}
		}

		const bool isFirehose(DataCase::GetInstance()->IsFirehose(m_candidate->GetBaseObject()));
		// don't try to re-harvest excluded, depleted or malformed ore vein again until we revisit the cell
		if (objType == ObjectType::oreVein)
		{
			if (DataCase::GetInstance()->AutoMiningDisabled())
			{
				DBG_VMESSAGE("loot oreVein - mining disabled due to conflict with another mod: 0x{:08x}", m_candidate->formID);
				data->BlockReference(m_candidate, Lootability::AutoMiningDisabledByIncompatibleMod);
				return Lootability::AutoMiningDisabledByIncompatibleMod;
			}
			else 
			{
				DBG_VMESSAGE("loot oreVein - do not process again during this cell visit: 0x{:08x}", m_candidate->formID);
				data->BlockReference(m_candidate, Lootability::CannotMineTwiceInSameCellVisit);
				const bool manualLootNotify(SettingsCache::Instance().ManualLootTargetNotify());
				const bool mineAll(SettingsCache::Instance().ObjectLootingType(ObjectType::oreVein) == LootingType::LootOreVeinAlways);
				if (!isFirehose || mineAll)
				{
					if (isFirehose)
					{
						// do not revisit over-generous sources any time soon - this is stronger than the oreVein temp block
						DataCase::GetInstance()->BlockFirehoseSource(m_candidate);
					}
					EventPublisher::Instance().TriggerMining(
						m_candidate, data->OreVeinResourceType(m_candidate->GetBaseObject()->As<RE::TESObjectACTI>()), manualLootNotify, isFirehose);
				}
			}
		}
		else
		{
			bool isSilent = !LootingRequiresNotification(lootingType) ||
				ScanGovernor::Instance().PendingHarvestNotifications() > ScanGovernor::HarvestSpamLimit;
			// don't let the backlog of messages get too large, it's about 1 per second
			// Event handler in Papyrus script unlocks the task - do not issue multiple concurrent events on the same REFR
			if (!ScanGovernor::Instance().LockHarvest(m_candidate, isSilent))
				return Lootability::HarvestOperationPending;
			// Check inventory limits. We don't try to fine-tune transfer-count here since the exact amount to be retrieved is not known.
			int itemCount(refrEx.GetItemCount());
			if (PlayerState::Instance().ItemHeadroom(const_cast<RE::TESBoundObject*>(refrEx.GetLootable()), itemCount) <= 0)
			{
				DBG_VMESSAGE("Inventory Limits preclude harvest for {}/0x{:08x}", refrEx.GetLootable()->GetName(), refrEx.GetLootable()->GetFormID());
				data->BlockReference(m_candidate, Lootability::InventoryLimitsEnforced);
				return Lootability::InventoryLimitsEnforced;
			}
			DBG_VMESSAGE("SmartHarvest {}/0x{:08x} for REFR 0x{:08x}, collectible={}, type {}, notify {}, pending {}", m_candidate->GetBaseObject()->GetName(),
				m_candidate->GetBaseObject()->GetFormID(), m_candidate->GetFormID(), collectible.first ? "true" : "false",
				GetObjectTypeName(objType), LootingRequiresNotification(lootingType), ScanGovernor::Instance().PendingHarvestNotifications());
			const bool whiteListNotify(SettingsCache::Instance().WhiteListTargetNotify());
			EventPublisher::Instance().TriggerHarvest(m_candidate, refrEx.GetTarget(), objType, itemCount,
				isSilent,
				collectible.first, PlayerState::Instance().PerkIngredientMultiplier(), whiteListNotify && whitelisted);
			if (isFirehose)
			{
				// do not revisit over-generous sources any time soon
				DataCase::GetInstance()->BlockFirehoseSource(m_candidate);
			}
		}
	}
	else if (m_targetType == INIFile::SecondaryType::containers || m_targetType == INIFile::SecondaryType::deadbodies)
	{
		if (m_candidate->IsActivationBlocked())
		{
			DBG_MESSAGE("skip activation-blocked container {}/0x{:08x}", m_candidate->GetName(), m_candidate->formID);
			return Lootability::ReferenceActivationBlocked;
		}
		if (DataCase::GetInstance()->ReferencesBlacklistedContainer(m_candidate))
		{
			DBG_MESSAGE("skip blacklisted container {}/0x{:08x}", m_candidate->GetName(), m_candidate->formID);
			return Lootability::ReferencesBlacklistedContainer;
		}
		DBG_MESSAGE("scanning container/body {}/0x{:08x}", m_candidate->GetName(), m_candidate->formID);
		bool skipLooting(false);
		// INI defaults exclude nudity by not looting armor from dead bodies
		bool excludeArmor(m_targetType == INIFile::SecondaryType::deadbodies &&
			SettingsCache::Instance().DeadBodyLootingType() == DeadBodyLooting::LootExcludingArmor);
		EnchantedObjectHandling enchantedLoot = SettingsCache::Instance().EnchantedObjectHandlingType();
		ContainerLister lister(m_targetType, m_candidate);
		size_t lootableItems(lister.AnalyzeLootableItems(enchantedLoot));
		if (lootableItems == 0)
		{
			if (!dryRun)
			{
				// Nothing lootable here
				DBG_MESSAGE("container {}/0x{:08x} is empty", m_candidate->GetName(), m_candidate->formID);
				// record looting so we don't rescan
				ScanGovernor::Instance().MarkContainerLooted(m_candidate);
			}
			return Lootability::ContainerHasNoLootableItems;
		}

		// initially no glow - flag using synthetic value with highest precedence
		m_glowReason = GlowReason::None;
		// if we glow for special objects, we do not want to overwrite that with 'items looted' glow lower down, and we want to handle repeat glow
		int glowDuration(0);
		if (m_targetType == INIFile::SecondaryType::containers)
		{
			// If a container is once found locked, it remains treated the same way according to the looting rules. This means a chest that player unlocked
			// will continue to glow if not auto-looted.
			LockedContainerHandling lockedChestLoot = SettingsCache::Instance().LockedChestLoot();
			if (ScanGovernor::Instance().IsReferenceLockedContainer(m_candidate, lockedChestLoot))
			{
				if (m_glowOnly)
				{
					lockedChestLoot = LockedContainerHandling::GlowTarget;
				}
				if (lockedChestLoot == LockedContainerHandling::GlowTarget || lockedChestLoot == LockedContainerHandling::LootOnceUnlocked)
				{
					DBG_VMESSAGE("glow locked container {}/0x{:08x}", m_candidate->GetName(), m_candidate->formID);
					UpdateGlowReason(GlowReason::LockedContainer);
				}

				if (!IsLockedContainerLootable(lockedChestLoot))
				{
					skipLooting = true;
					result = Lootability::ContainerIsLocked;
				}
			}

			if (IsBossContainer(m_candidate))
			{
				SpecialObjectHandling bossChestLoot = SettingsCache::Instance().BossChestLoot();
				if (m_glowOnly)
				{
					bossChestLoot = SpecialObjectHandling::GlowTarget;
				}
				if (bossChestLoot == SpecialObjectHandling::GlowTarget)
				{
					DBG_VMESSAGE("glow boss container {}/0x{:08x}", m_candidate->GetName(), m_candidate->formID);
					UpdateGlowReason(GlowReason::BossContainer);
				}

				if (!IsSpecialObjectLootable(bossChestLoot))
				{
					skipLooting = true;
					result = Lootability::ContainerIsBossChest;
				}
			}
		}

		// Container or NPC may itself be a Quest target - if so the entire thing is blocked from autoloot
		if (IsQuestItem(m_candidate))
		{
			QuestObjectHandling questObjectLoot = SettingsCache::Instance().QuestObjectLoot();
			DBG_VMESSAGE("Quest Container/NPC REFR 0x{:08x} to {}/0x{:08x}, glow={}", m_candidate->GetFormID(), m_candidate->GetBaseObject()->GetName(),
				m_candidate->GetBaseObject()->formID, questObjectLoot == QuestObjectHandling::GlowTarget ? "true" : "false");
			if (questObjectLoot == QuestObjectHandling::GlowTarget)
			{
				UpdateGlowReason(GlowReason::QuestObject);
			}

			skipLooting = true;
			result = Lootability::CannotLootQuestTarget;
		}
		else
		{
			if (lister.HasQuestItem())
			{
				QuestObjectHandling questObjectLoot = SettingsCache::Instance().QuestObjectLoot();
				if (questObjectLoot == QuestObjectHandling::GlowTarget)
				{
					DBG_VMESSAGE("glow container with quest object {}/0x{:08x}", m_candidate->GetName(), m_candidate->formID);
					UpdateGlowReason(GlowReason::QuestObject);
				}

				// this is not a blocker for looting of non-special items
				lister.ExcludeQuestItems();
				result = Lootability::ContainerHasQuestObject;
			}

			if (lister.HasEnchantedItem())
			{
				if (m_glowOnly)
				{
					enchantedLoot = EnchantedObjectHandling::GlowTarget;
				}
				if (enchantedLoot == EnchantedObjectHandling::GlowTarget || enchantedLoot == EnchantedObjectHandling::GlowTargetUnknown)
				{
					DBG_VMESSAGE("glow container with enchanted object {}/0x{:08x}", m_candidate->GetName(), m_candidate->formID);
					UpdateGlowReason(GlowReason::EnchantedItem);
				}
				if (!IsEnchantedObjectLootable(enchantedLoot))
				{
					// this is not a blocker for looting of non-special items
					lister.ExcludeEnchantedItems();
					result = Lootability::ContainerHasEnchantedObject;
				}
			}

			if (lister.HasValuableItem())
			{
				SpecialObjectHandling valuableLoot = SettingsCache::Instance().ValuableItemLoot();
				if (m_glowOnly)
				{
					valuableLoot = SpecialObjectHandling::GlowTarget;
				}
				if (valuableLoot == SpecialObjectHandling::GlowTarget)
				{
					DBG_VMESSAGE("glow container with valuable object {}/0x{:08x}", m_candidate->GetName(), m_candidate->formID);
					UpdateGlowReason(GlowReason::Valuable);
				}

				if (!IsSpecialObjectLootable(valuableLoot))
				{
					// this is not a blocker for looting of non-special items
					lister.ExcludeValuableItems();
					result = Lootability::ContainerHasValuableObject;
				}
			}

			if (lister.HasCollectibleItem())
			{
				CollectibleHandling collectibleAction(lister.CollectibleAction());
				if (m_glowOnly)
				{
					collectibleAction = CollectibleHandling::Glow;
				}
				if (!CanLootCollectible(collectibleAction))
				{
					// this is not a blocker for looting of non-special items
					lister.ExcludeCollectibleItems();

					if (collectibleAction == CollectibleHandling::Glow)
					{
						DBG_VMESSAGE("glow container with collectible object {}/0x{:08x}", m_candidate->GetName(), m_candidate->formID);
						UpdateGlowReason(GlowReason::Collectible);
						result = Lootability::CollectibleItemSetToGlow;
					}
					else if (collectibleAction == CollectibleHandling::Print)
					{
						result = Lootability::ManualLootTarget;
					}
					else
					{
						result = Lootability::ItemInBlacklistCollection;
					}
				}
			}

			// Order is important to ensure we glow correctly even if blocked.
			// Check here is on the container, skip all contents if looting not permitted
			Lootability forbidden(LootingLegality(m_targetType));
			if (forbidden != Lootability::Lootable)
			{
				skipLooting = true;
				result = forbidden;
			}
		}

		// Always allow auto-looting of dead bodies, e.g. Solitude Hall of the Dead in LCTN Solitude has skeletons that we
		// should be able to murder/plunder. And don't forget Margret in Markarth.
		if (!skipLooting && m_targetType != INIFile::SecondaryType::deadbodies &&
			!LocationTracker::Instance().IsPlayerInWhitelistedPlace() &&
			LocationTracker::Instance().IsPlayerInRestrictedLootSettlement())
		{
			DBG_VMESSAGE("Player location is excluded as restricted population center for this target type");
			skipLooting = true;
			result = Lootability::PopulousLocationRestrictsLooting;
		}

		if (!dryRun && m_glowReason != GlowReason::None)
		{
			ScanGovernor::Instance().GlowObject(m_candidate, ObjectGlowDurationSpecialSeconds, m_glowReason);
			if (m_glowOnly)
			{
				return result;
			}
			glowDuration = ObjectGlowDurationSpecialSeconds;
		}

		// If it contains white-listed items we must nonetheless skip, due to legality checks at the container level
		if (dryRun || skipLooting)
			return result;

		// Check if we should attempt to loot the target's contents the item. If we skip it due to looting rules, it's
		// immune from stealing.
		// If we wish to auto-steal an item we must check we are not detected, which requires a scripted check. If this
		// is the delayed autoloot operation after we find we are undetected, don't trigger that check again here.
		// This check is skipped if we are in glow-only mode.
		if (!m_glowOnly && !m_stolen && m_candidate->IsOffLimits() &&
			PlayerState::Instance().EffectiveOwnershipRule() == OwnershipRule::AllowCrimeIfUndetected)
		{
			DBG_VMESSAGE("Container/deadbody contents {}/0x{:08x} to be stolen if undetected", m_candidate->GetName(), m_candidate->formID);
			TheftCoordinator::Instance().DelayStealableItem(m_candidate, m_targetType);
			return Lootability::ItemTheftTriggered;
		}

		// Build list of lootable targets with notification, collectibility flag, whitelist notification & count for each
		const bool whiteListNotify(SettingsCache::Instance().WhiteListTargetNotify());
		std::vector<std::tuple<InventoryItem, bool, bool, bool, size_t>> targets;
		targets.reserve(lootableItems);
		for (auto& targetItemInfo : lister.GetLootableItems())
		{
			RE::TESBoundObject* target(targetItemInfo.BoundObject());
			if (!target)
				continue;

			if (ManagedList::BlackList().Contains(target))
			{
				DBG_VMESSAGE("skip 0x{:08x} due to BlackList", target->formID);
				continue;
			}

			ObjectType objType = targetItemInfo.LootObjectType();
			if (excludeArmor && (objType == ObjectType::armor || objType == ObjectType::enchantedArmor))
			{
				// obey SFW setting, for this REFR on this pass - state resets on game reload/cell re-entry/MCM update
				DBG_VMESSAGE("block looting of armor from dead body {}/0x{:08x}", target->GetName(), target->GetFormID());
				continue;
			}
			// Special case ingredient handling - we may be asked to always pick up ingredients with Unknown effects, overriding 'ingredient' setting
			bool forceIngredientLoot = false;
			if (objType == ObjectType::ingredient)
			{
				forceIngredientLoot = SettingsCache::Instance().UnknownIngredientLoot() &&
					!DataCase::GetInstance()->IsIngredientKnown(target);
				if (forceIngredientLoot)
				{
					DBG_VMESSAGE("force transfer of unknown effect ingredient {}/0x{:08x}", target->GetName(), target->GetFormID());
				}
			}

			LootingType lootingType(LootingType::LeaveBehind);
			bool sendWhiteListNotify(false);
			static const bool recordDups(true);		// final decision to loot the item happens here
			const auto collectible(CollectionManager::Instance().TreatAsCollectible(
				ConditionMatcher(target, m_targetType, objType), recordDups));
			if (collectible.first)
			{
				CollectibleHandling collectibleAction(collectible.second);
				if (CanLootCollectible(collectibleAction))
				{
					DBG_VMESSAGE("Collectible Item 0x{:08x}", target->formID);
					lootingType = LootingType::LootAlwaysSilent;
				}
				else if (collectibleAction == CollectibleHandling::Print)
				{
					if (!dryRun)
					{
						// print message about container item
						ProcessManualLootItem(target);
					}
					continue;
				}
				else
				{
					// blacklisted or 'glow'
					DBG_VMESSAGE("Collectible Item 0x{:08x} skipped", target->formID);
					continue;
				}
			}
			else if (ManagedList::WhiteList().Contains(target))
			{
				// whitelisted objects are always looted silently
				DBG_VMESSAGE("transfer whitelisted 0x{:08x}", target->formID);
				lootingType = LootingType::LootAlwaysSilent;
				sendWhiteListNotify = whiteListNotify;
			}
			else if (ManagedList::BlackList().Contains(target))
			{
				// blacklisted objects are never looted
				DBG_VMESSAGE("skip blacklisted target {}/0x{:08x}", target->GetName(), target->GetFormID());
				continue;
			}
			else
			{
				std::string typeName = GetObjectTypeName(objType);
				lootingType = forceIngredientLoot ? LootingType::LootAlwaysNotify : SettingsCache::Instance().ObjectLootingType(objType);

				if (lootingType == LootingType::LeaveBehind)
				{
					DBG_VMESSAGE("block - typename {} excluded for 0x{:08x}", typeName.c_str(), target->formID);
					data->BlockForm(target, Lootability::ItemTypeIsSetToPreventLooting);
					continue;
				}
				else if (!forceIngredientLoot && LootingDependsOnValueWeight(lootingType, objType) &&
					TESFormHelper(target, objType, m_targetType).ValueWeightTooLowToLoot())
				{
					DBG_VMESSAGE("block - v/w excludes for 0x{:08x}", target->formID);
					data->BlockForm(target, Lootability::ValueWeightPreventsLooting);
					continue;
				}
			}

			// crime-check this REFR from the container as individual object, respecting collectibility if not a crime
			if (ItemLootingLegality(collectible.first, m_targetType) != Lootability::Lootable)
			{
				continue;
			}

			// item count unknown at this point
			targets.push_back({ 
				targetItemInfo,
				LootingRequiresNotification(lootingType),
				collectible.first,
				sendWhiteListNotify, 0 });
			DBG_MESSAGE("get {} ({}) from container {}/0x{:08x}", target->GetName(), targetItemInfo.Count(),
				m_candidate->GetName(), m_candidate->formID);
		}

		if (m_glowOnly)
		{
			if (!targets.empty())
			{
				ScanGovernor::Instance().GlowObject(m_candidate, ObjectGlowDurationSpecialSeconds, GlowReason::SimpleTarget);
			}
			return result;
		}

		if (!targets.empty())
		{
			// check highlighting for dead NPC or container
			ContainerAnimationHandling playContainerAnimation(SettingsCache::Instance().PlayContainerAnimation());
			if (playContainerAnimation != ContainerAnimationHandling::DoNotPlay)
			{
				if (m_targetType == INIFile::SecondaryType::containers)
				{
					if (!GetTimeController(m_candidate))
					{
						// no container animation feasible, highlight it instead
						playContainerAnimation = ContainerAnimationHandling::Glow;
					}
				}
				else
				{
					// Dead NPCs cannot be animated, but highlighting requested
					playContainerAnimation = ContainerAnimationHandling::Glow;
				}
			}
			// Persistent glow for special items overrides "items looted" glow
			if (playContainerAnimation == ContainerAnimationHandling::Glow && glowDuration > 0)
			{
				playContainerAnimation = ContainerAnimationHandling::DoNotPlay;
			}
			// use inline transfer for containers on first attempt - fills in item counts
			GetLootFromContainer(targets, playContainerAnimation, m_targetType == INIFile::SecondaryType::containers);
		}

		// Avoid re-looting without a player cell or config change. Sometimes auto-looting here may fail, so we just copy the
		// items and blacklist the REFR to avoid revisiting. Confirm looting by checking lootable target count now vs start
		// value. This logic only applies to containers: NPC auto-looting is scripted and not known to fail.
		if (m_targetType == INIFile::SecondaryType::containers && !targets.empty() &&
			lister.CountLootableItems([=](RE::TESBoundObject*) -> bool { return true; }) >= lootableItems)
		{
			// nothing looted - make copies of targets and blacklist the reference (e.g. MrB's Lootable Things)
			REL_WARNING("looting {} items from container {}/0x{:08x} resulted in no-op, make copies", targets.size(),
				m_candidate->GetName(), m_candidate->formID);
			CopyLootFromContainer(targets);
			// Main Blacklist does not work for dynamic forms - block those separately. e.g. Hawk shot down outside Solitude
			if (!ScanGovernor::Instance().HandleAsDynamicData(m_candidate))
			{
				ScanGovernor::Instance().MarkContainerLootedRepeatGlow(m_candidate, glowDuration);
			}
		}
		else
		{
			DBG_MESSAGE("block looted container/NPC {}/0x{:08x}", m_candidate->GetName(), m_candidate->formID);
			ScanGovernor::Instance().MarkContainerLootedRepeatGlow(m_candidate, glowDuration);
		}
	}
	return result;
}

void TryLootREFR::GetLootFromContainer(std::vector<std::tuple<InventoryItem, bool, bool, bool, size_t>>& targets,
	const ContainerAnimationHandling animationType, const bool inlineTransfer)
{
	if (!m_candidate)
		return;

	REL_MESSAGE("Loot {} items from {}/0x{:08x}", targets.size(), m_candidate->GetName(), m_candidate->formID);

	// visual notification, if requested
	if (animationType == ContainerAnimationHandling::Play)
	{
		m_candidate->PlayAnimation("Close", "Open");
	}
	else if (animationType == ContainerAnimationHandling::Glow)
	{
		// glow looted object briefly after looting
		ScanGovernor::Instance().GlowObject(m_candidate, ObjectGlowDurationLootedSeconds, GlowReason::SimpleTarget);
	}

	// avoid sound spam
	bool madeSound(false);
	for (auto& target : targets)
	{
		// Play sound first as this uses InventoryItemData on the source container
		InventoryItem& itemInfo(std::get<0>(target));
		bool notify(std::get<1>(target));
		bool collectible(std::get<2>(target));
		bool whiteListNotify(std::get<3>(target));
		if (!madeSound)
		{
			RE::PlayerCharacter::GetSingleton()->PlayPickUpSound(itemInfo.BoundObject(), true, false);
			madeSound = true;
		}
		std::string name(itemInfo.BoundObject()->GetName());
		size_t count(itemInfo.TakeAll(m_candidate, RE::PlayerCharacter::GetSingleton(), collectible, inlineTransfer));
		// save count in case we have to copy these after failure to transfer (e.g. MrB's Lootable Things)
		std::get<4>(target) = count;
		std::string notificationText;
		if (notify)
		{
			if (count > 1)
			{
				static RE::BSFixedString multiActivate(DataCase::GetInstance()->GetTranslation("$SHSE_ACTIVATE(COUNT)_MSG"));
				if (!multiActivate.empty())
				{
					notificationText = multiActivate;
					StringUtils::Replace(notificationText, "{ITEMNAME}", name.c_str());
					std::ostringstream intStr;
					intStr << count;
					StringUtils::Replace(notificationText, "{COUNT}", intStr.str());
				}
			}
			else
			{
				static RE::BSFixedString singleActivate(DataCase::GetInstance()->GetTranslation("$SHSE_ACTIVATE_MSG"));
				if (!singleActivate.empty())
				{
					notificationText = singleActivate;
					StringUtils::Replace(notificationText, "{ITEMNAME}", name.c_str());
				}
			}
			if (!notificationText.empty())
			{
				RE::DebugNotification(notificationText.c_str());
			}
		}
		if (whiteListNotify)
		{
			static RE::BSFixedString whiteListMsg(DataCase::GetInstance()->GetTranslation("$SHSE_WHITELIST_ITEM_LOOTED"));
			if (!whiteListMsg.empty())
			{
				notificationText = whiteListMsg;
				StringUtils::Replace(notificationText, "{ITEMNAME}", name.c_str());
				RE::DebugNotification(notificationText.c_str());
			}
		}
	}
}

void TryLootREFR::CopyLootFromContainer(std::vector<std::tuple<InventoryItem, bool, bool, bool, size_t>>& targets)
{
	if (!m_candidate)
		return;

	for (auto& target : targets)
	{
		InventoryItem& itemInfo(std::get<0>(target));
		itemInfo.MakeCopies(RE::PlayerCharacter::GetSingleton(), std::get<4>(target));
	}
}

Lootability TryLootREFR::ItemLootingLegality(const bool isCollectible, INIFile::SecondaryType targetType)
{
	Lootability result(LootingLegality(targetType));
	if (isCollectible && LootOwnedItemIfCollectible(result))
	{
		DBG_VMESSAGE("Collectible REFR 0x{:08x} overrides Legality {} for {}/0x{:08x}", m_candidate->GetFormID(), LootabilityName(result).c_str(),
			m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->GetFormID());
		result = Lootability::Lootable;
	}
	return result;
}

Lootability TryLootREFR::LootingLegality(const INIFile::SecondaryType targetType)
{
	// Already trying to steal this - bypass repeat check, known to be OK modulo actor or player state change
	// in the world
	if (m_stolen)
	{
		// Avoid theft due to timing window by removing ownership for autoloot of items for which we were undetected
		RE::TESForm* owner(m_candidate->GetOwner());
		if (owner)
		{
			DBG_VMESSAGE("Remove owner {}/0x{:08x} from theft target {}/0x{:08x}", owner->GetName(), owner->GetFormID(),
				m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
			m_candidate->extraList.SetOwner(nullptr);
		}
		return Lootability::Lootable;
	}

	Lootability legality(Lootability::Lootable);
	// Perform crime checks - this is done after checks for quest object glowing, as many quest-related objects are owned.
	// Ownership expires with the target, e.g. Francis the Horse from Interesting NPCs was killed by a wolf in Solitude
	// and becomes lootable thereafter.
	// For non-dead targets, check law-abiding settings vs criminality of target and player-ownership settings vs ownership
	if (targetType != INIFile::SecondaryType::deadbodies)
	{
		// check up to three ownership conditions depending on config
		bool playerOwned(IsPlayerOwned(m_candidate));
		// Fired arrows are marked as player owned but we don't want to prevent pickup, ever
		bool firedArrow(m_candidate->formType == RE::FormType::ProjectileArrow);
		bool lootingIsCrime(m_candidate->IsOffLimits());
		if (!lootingIsCrime && playerOwned && !firedArrow)
		{
			// can configure to not loot my own belongings even though it's always legal
			if (!IsSpecialObjectLootable(PlayerState::Instance().BelongingsCheck()))
			{
				DBG_VMESSAGE("Player-owned {}, looting belongings disallowed: {}/0x{:08x}",
					playerOwned ? "true" : "false", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
				legality = Lootability::PlayerOwned;
				// Glow if configured
				if (PlayerState::Instance().BelongingsCheck() == SpecialObjectHandling::GlowTarget)
				{
					UpdateGlowReason(GlowReason::PlayerProperty);
				}
			}
		}
		// if restricted to law-abiding citizenship, check if OK to loot
		else if (PlayerState::Instance().EffectiveOwnershipRule() != OwnershipRule::AllowCrimeIfUndetected)
		{
			if (lootingIsCrime)
			{
				// never commit a crime unless crimeCheck is 0
				DBG_VMESSAGE("Crime to loot REFR, cannot loot");
				legality = Lootability::CrimeToLoot;
			}
			else if (PlayerState::Instance().EffectiveOwnershipRule() == OwnershipRule::Ownerless)
			{
				if (!playerOwned && !firedArrow &&
					(m_candidate->GetOwner() != nullptr || !LocationTracker::Instance().IsPlayerInFriendlyCell()))
				{
					// owner of item or cell is not player/player-friendly - disallow owned item
					DBG_VMESSAGE("REFR or Cell is not player-owned, cannot loot");
					legality = Lootability::CellOrItemOwnerPreventsOwnerlessLooting;
				}
			}
		}
	}
	return legality;
}

bool TryLootREFR::IsBookGlowable() const
{
	RE::BGSKeywordForm* keywordForm(m_candidate->GetBaseObject()->As<RE::BGSKeywordForm>());
	if (!keywordForm)
		return false;
	for (uint32_t index = 0; index < keywordForm->GetNumKeywords(); ++index)
	{
		std::optional<RE::BGSKeyword*> keyword(keywordForm->GetKeywordAt(index));
		if (!keyword || !keyword.has_value())
			continue;
		if (DataCase::GetInstance()->IsBookGlowableKeyword(keyword.value()))
			return true;
	}
	return false;
}

}
