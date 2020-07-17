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
#include "FormHelpers/FormHelper.h"
#include "VM/EventPublisher.h"
#include "Utilities/debugs.h"
#include "Utilities/utils.h"
#include "WorldState/PlayerState.h"

namespace shse
{

TryLootREFR::TryLootREFR(RE::TESObjectREFR* target, INIFile::SecondaryType targetType, const bool stolen)
	: m_candidate(target), m_targetType(targetType), m_glowReason(GlowReason::None), m_stolen(stolen)
{
}

Lootability TryLootREFR::Process(const bool dryRun)
{
	DataCase* data = DataCase::GetInstance();
	Lootability result(Lootability::Lootable);

	if (m_targetType == INIFile::SecondaryType::itemObjects)
	{
		LootableREFR refrEx(m_candidate, m_targetType);
		ObjectType objType = refrEx.GetObjectType();
		std::string typeName = refrEx.GetTypeName();
		// Various form types contain an ingredient or FormList that is the final lootable item - resolve here
		if (!dryRun && objType == ObjectType::critter)
		{
			RE::TESForm* lootable(ProducerLootables::Instance().GetLootableForProducer(m_candidate->GetBaseObject()));
			if (lootable)
			{
				DBG_VMESSAGE("producer %s/0x%08x has lootable %s/0x%08x", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID,
					lootable->GetName(), lootable->formID);
				refrEx.SetLootable(lootable);
			}
			else
			{
				// trigger critter -> ingredient resolution and skip until it's resolved - pending resolve recorded using nullptr,
				// only trigger if not already pending
				DBG_VMESSAGE("resolve critter %s/0x%08x to ingredient", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
				if (ProducerLootables::Instance().SetLootableForProducer(m_candidate->GetBaseObject(), nullptr))
				{
					EventPublisher::Instance().TriggerGetProducerLootable(m_candidate);
				}
				return Lootability::PendingProducerIngredient;
			}
		}

		if (objType == ObjectType::unknown)
		{
			if (!dryRun)
			{
				DBG_VMESSAGE("blacklist objType == ObjectType::unknown for 0x%08x", m_candidate->GetFormID());
				data->BlacklistReference(m_candidate);
			}
			return Lootability::ObjectTypeUnknown;
		}

		bool manualLootNotify(INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "ManualLootTargetNotify") != 0);
		if (objType == ObjectType::manualLoot && manualLootNotify)
		{
			if (!dryRun)
			{
				// notify about these, just once
				std::string notificationText;
				static RE::BSFixedString manualLootText(DataCase::GetInstance()->GetTranslation("$SHSE_MANUAL_LOOT_MSG"));
				if (!manualLootText.empty())
				{
					notificationText = manualLootText;
					StringUtils::Replace(notificationText, "{ITEMNAME}", m_candidate->GetName());
					if (!notificationText.empty())
					{
						RE::DebugNotification(notificationText.c_str());
					}
				}
				DBG_VMESSAGE("notify, then block objType == ObjectType::manualLoot for 0x%08x", m_candidate->GetFormID());
				data->BlockReference(m_candidate);
			}
			return Lootability::ManualLootTarget;
		}

		if (ManagedList::BlackList().Contains(m_candidate->GetBaseObject()))
		{
			if (!dryRun)
			{
				DBG_VMESSAGE("Block BlackListed REFR base form 0x%08x", m_candidate->GetBaseObject()->GetFormID());
				data->BlockForm(m_candidate->GetBaseObject());
			}
			return Lootability::BaseObjectOnBlacklist;
		}
#if _DEBUG
		DumpReference(refrEx, typeName.c_str(), m_targetType);
#endif
		// initially no glow - use synthetic value with highest precedence
		m_glowReason = GlowReason::None;
		bool skipLooting(false);

		bool needsFullQuestFlags(INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "questObjectScope") != 0);
		SpecialObjectHandling questObjectLoot =
			SpecialObjectHandlingFromIniSetting(INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "questObjectLoot"));
		if (refrEx.IsQuestItem(needsFullQuestFlags))
		{
			DBG_VMESSAGE("Quest Item 0x%08x", m_candidate->GetBaseObject()->formID);
			if (questObjectLoot == SpecialObjectHandling::GlowTarget)
			{
				DBG_VMESSAGE("glow quest object %s/0x%08x", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
				UpdateGlowReason(GlowReason::QuestObject);
			}

			if (!IsSpecialObjectLootable(questObjectLoot))
			{
				skipLooting = true;
				result = Lootability::CannotLootQuestObject;
			}
		}
		// glow unread notes as they are often quest-related
		else if (questObjectLoot == SpecialObjectHandling::GlowTarget && objType == ObjectType::book && IsBookGlowable())
		{
			DBG_VMESSAGE("Glowable book 0x%08x", m_candidate->GetBaseObject()->formID);
			UpdateGlowReason(GlowReason::SimpleTarget);
		}

		// TODO this may update state on a dry run but we should already have processed the item on >= 1 pass, so no harm?
		const auto collectible(refrEx.TreatAsCollectible());
		SpecialObjectHandling collectibleAction(collectible.second);
		if (collectible.first)
		{
			DBG_VMESSAGE("Collectible Item 0x%08x", m_candidate->GetBaseObject()->formID);
			if (collectibleAction == SpecialObjectHandling::GlowTarget)
			{
				DBG_VMESSAGE("glow collectible object %s/0x%08x", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
				UpdateGlowReason(GlowReason::Collectible);
			}

			if (!IsSpecialObjectLootable(collectibleAction))
			{
				skipLooting = true;
				result = Lootability::CannotLootCollectibleObject;
			}
		}

		SpecialObjectHandling valuableLoot =
			SpecialObjectHandlingFromIniSetting(INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "ValuableItemLoot"));
		if (refrEx.IsValuable())
		{
			DBG_VMESSAGE("Valuable Item 0x%08x", m_candidate->GetBaseObject()->formID);
			if (valuableLoot == SpecialObjectHandling::GlowTarget)
			{
				DBG_VMESSAGE("glow valuable object %s/0x%08x", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
				UpdateGlowReason(GlowReason::Valuable);
			}

			if (!IsSpecialObjectLootable(valuableLoot))
			{
				skipLooting = true;
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

		// order is important to ensure we glow correctly even if blocked
		Lootability forbidden(LootingLegality(m_targetType));
		if (forbidden != Lootability::Lootable)
		{
			skipLooting = true;
			result = forbidden;
		}

		if (!dryRun && m_glowReason != GlowReason::None)
		{
			ScanGovernor::Instance().GlowObject(m_candidate, ObjectGlowDurationSpecialSeconds, m_glowReason);
		}

		// Harvesting and mining is allowed in settlements. We really just want to not auto-loot entire
		// buildings of friendly factions, and the like. Mines and farms mostly self-identify as Settlements.
		if (!LocationTracker::Instance().IsPlayerInWhitelistedPlace(LocationTracker::Instance().PlayerCell()) &&
			LocationTracker::Instance().IsPlayerInRestrictedLootSettlement(LocationTracker::Instance().PlayerCell()) &&
			!IsItemLootableInPopulationCenter(m_candidate->GetBaseObject(), objType))
		{
			DBG_VMESSAGE("Player location is excluded as restricted population center for this item");
			result = Lootability::PopulousLocationRestrictsLooting;
			skipLooting = true;
		}

		LootingType lootingType(LootingType::LeaveBehind);
		if (collectible.first)
		{
			// ** if configured as permitted ** collectible objects are always looted silently
			DBG_VMESSAGE("check REFR to collectible 0x%08x", m_candidate->GetBaseObject()->formID);
			skipLooting = forbidden != Lootability::Lootable || collectibleAction != SpecialObjectHandling::DoLoot;
			lootingType = collectibleAction == SpecialObjectHandling::DoLoot ? LootingType::LootAlwaysSilent : LootingType::LeaveBehind;
			if (lootingType == LootingType::LeaveBehind)
			{
				if (!dryRun)
				{
					// this is a blacklist collection
					DBG_VMESSAGE("block blacklist collection member 0x%08x", m_candidate->GetBaseObject()->formID);
					data->BlockForm(m_candidate->GetBaseObject());
				}
				result = Lootability::ItemInBlacklistCollection;
			}
			else if (skipLooting)
			{
				result = Lootability::CollectibleItemSetToGlow;
			}
		}
		else if (ManagedList::WhiteList().Contains(m_candidate->GetBaseObject()))
		{
			// ** if configured as permitted ** whitelisted objects are always looted silently
			DBG_VMESSAGE("check REFR to whitelisted 0x%08x", m_candidate->GetBaseObject()->formID);
			skipLooting = forbidden != Lootability::Lootable;
			if (skipLooting)
			{
				result = Lootability::LawAbidingSoNoWhitelistItemLooting;
			}
			lootingType = LootingType::LootAlwaysSilent;
		}
		else if (ManagedList::BlackList().Contains(m_candidate->GetBaseObject()))
		{
			if (!dryRun)
			{
				// blacklisted objects are never looted
				DBG_VMESSAGE("block blacklisted base %s/0x%08x for REFR 0x%08x",
					m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->GetFormID(), m_candidate->GetFormID());
				data->BlockForm(m_candidate->GetBaseObject());
			}
			skipLooting = true;
			result = Lootability::ItemIsBlacklisted;
			lootingType = LootingType::LeaveBehind;
		}
		else if (!skipLooting)
		{
			lootingType = LootingTypeFromIniSetting(
				INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::itemObjects, typeName.c_str()));
			if (lootingType == LootingType::LeaveBehind)
			{
				if (!dryRun)
				{
					DBG_VMESSAGE("Block REFR : LeaveBehind for 0x%08x", m_candidate->GetBaseObject()->formID);
					data->BlockReference(m_candidate);
				}
				skipLooting = true;
				result = Lootability::ItemSettingsPreventLooting;
			}
			else if (LootingDependsOnValueWeight(lootingType, objType))
			{
				TESFormHelper helper(m_candidate->GetBaseObject(), m_targetType);
				if (helper.ValueWeightTooLowToLoot())
				{
					if (!dryRun)
					{
						DBG_VMESSAGE("block - v/w excludes harvest for 0x%08x", m_candidate->GetBaseObject()->formID);
						data->BlockForm(m_candidate->GetBaseObject());
					}
					skipLooting = true;
					result = Lootability::ValueWeightPreventsLooting;
				}
				DBG_VMESSAGE("%s/0x%08x value:%d", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID, int(helper.GetWorth()));
			}
		}

		if (skipLooting || dryRun)
			return result;

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

		// don't try to re-harvest excluded, depleted or malformed ore vein again until we revisit the cell
		if (objType == ObjectType::oreVein)
		{
			DBG_VMESSAGE("do not process oreVein more than once per cell visit: 0x%08x", m_candidate->formID);
			data->BlockReference(m_candidate);
			EventPublisher::Instance().TriggerMining(m_candidate, data->OreVeinResourceType(m_candidate->GetBaseObject()->As<RE::TESObjectACTI>()), manualLootNotify);
		}
		else
		{
			bool isSilent = !LootingRequiresNotification(lootingType);
			DBG_VMESSAGE("SmartHarvest %s/0x%08x for REFR 0x%08x", m_candidate->GetBaseObject()->GetName(),
				m_candidate->GetBaseObject()->GetFormID(), m_candidate->GetFormID());
			// don't let the backlog of messages get too large, it's about 1 per second
			// Event handler in Papyrus script unlocks the task - do not issue multiple concurrent events on the same REFR
			if (!ScanGovernor::Instance().LockHarvest(m_candidate, isSilent))
				return Lootability::HarvestOperationPending;
			EventPublisher::Instance().TriggerHarvest(m_candidate, objType, refrEx.GetItemCount(),
				isSilent || ScanGovernor::Instance().PendingHarvestNotifications() > ScanGovernor::HarvestSpamLimit, manualLootNotify,
				collectible.first, PlayerState::Instance().PerkIngredientMultiplier());
		}
	}
	else if (m_targetType == INIFile::SecondaryType::containers || m_targetType == INIFile::SecondaryType::deadbodies)
	{
		DBG_MESSAGE("scanning container/body %s/0x%08x", m_candidate->GetName(), m_candidate->formID);
#if _DEBUG
		DumpContainer(LootableREFR(m_candidate, m_targetType));
#endif
		bool requireQuestItemAsTarget = INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "questObjectScope") != 0;
		bool skipLooting(false);
		// INI defaults exclude nudity by not looting armor from dead bodies
		bool excludeArmor(m_targetType == INIFile::SecondaryType::deadbodies &&
			DeadBodyLootingFromIniSetting(INIFile::GetInstance()->GetSetting(
				INIFile::PrimaryType::common, INIFile::SecondaryType::config, "EnableLootDeadbody")) == DeadBodyLooting::LootExcludingArmor);
		ContainerLister lister(m_targetType, m_candidate, requireQuestItemAsTarget);
		LootableItems lootableItems(lister.GetOrCheckContainerForms());
		if (lootableItems.empty())
		{
			if (!dryRun)
			{
				// Nothing lootable here
				DBG_MESSAGE("container %s/0x%08x is empty", m_candidate->GetName(), m_candidate->formID);
				// record looting so we don't rescan
				ScanGovernor::Instance().MarkContainerLooted(m_candidate);
			}
			return Lootability::ContainerHasNoLootableItems;
		}

		// initially no glow - flag using synthetic value with highest precedence
		m_glowReason = GlowReason::None;
		if (m_targetType == INIFile::SecondaryType::containers)
		{
			// If a container is once found locked, it remains treated the same way according to the looting rules. This means a chest that player unlocked
			// will continue to glow if not auto-looted.
			if (ScanGovernor::Instance().IsReferenceLockedContainer(m_candidate))
			{
				SpecialObjectHandling lockedChestLoot =
					SpecialObjectHandlingFromIniSetting(INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "lockedChestLoot"));
				if (lockedChestLoot == SpecialObjectHandling::GlowTarget)
				{
					DBG_VMESSAGE("glow locked container %s/0x%08x", m_candidate->GetName(), m_candidate->formID);
					UpdateGlowReason(GlowReason::LockedContainer);
				}

				if (!IsSpecialObjectLootable(lockedChestLoot))
				{
					skipLooting = true;
					result = Lootability::ContainerIsLocked;
				}
			}

			if (IsBossContainer(m_candidate))
			{
				SpecialObjectHandling bossChestLoot =
					SpecialObjectHandlingFromIniSetting(INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "bossChestLoot"));
				if (bossChestLoot == SpecialObjectHandling::GlowTarget)
				{
					DBG_VMESSAGE("glow boss container %s/0x%08x", m_candidate->GetName(), m_candidate->formID);
					UpdateGlowReason(GlowReason::BossContainer);
				}

				if (!IsSpecialObjectLootable(bossChestLoot))
				{
					skipLooting = true;
					result = Lootability::ContainerIsBossChest;
				}
			}
		}

		if (lister.HasQuestItem())
		{
			SpecialObjectHandling questObjectLoot =
				SpecialObjectHandlingFromIniSetting(INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "questObjectLoot"));
			if (questObjectLoot == SpecialObjectHandling::GlowTarget)
			{
				DBG_VMESSAGE("glow container with quest object %s/0x%08x", m_candidate->GetName(), m_candidate->formID);
				UpdateGlowReason(GlowReason::QuestObject);
			}

			if (!IsSpecialObjectLootable(questObjectLoot))
			{
				skipLooting = true;
				result = Lootability::ContainerHasQuestObject;
			}
		}

		if (lister.HasEnchantedItem())
		{
			SInt32 enchantItemGlow = static_cast<int>(INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "enchantItemGlow"));
			if (enchantItemGlow == 1)
			{
				DBG_VMESSAGE("glow container with enchanted object %s/0x%08x", m_candidate->GetName(), m_candidate->formID);
				UpdateGlowReason(GlowReason::EnchantedItem);
			}
		}

		if (lister.HasValuableItem())
		{
			SpecialObjectHandling valuableLoot =
				SpecialObjectHandlingFromIniSetting(INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "ValuableItemLoot"));
			if (valuableLoot == SpecialObjectHandling::GlowTarget)
			{
				DBG_VMESSAGE("glow container with valuable object %s/0x%08x", m_candidate->GetName(), m_candidate->formID);
				UpdateGlowReason(GlowReason::Valuable);
			}

			if (!IsSpecialObjectLootable(valuableLoot))
			{
				skipLooting = true;
				result = Lootability::ContainerHasValuableObject;
			}
		}

		if (lister.HasCollectibleItem())
		{
			if (lister.CollectibleAction() == SpecialObjectHandling::GlowTarget)
			{
				DBG_VMESSAGE("glow container with collectible object %s/0x%08x", m_candidate->GetName(), m_candidate->formID);
				UpdateGlowReason(GlowReason::Collectible);
			}

			if (!IsSpecialObjectLootable(lister.CollectibleAction()))
			{
				skipLooting = true;
				result = Lootability::ContainerHasCollectibleObject;
			}
		}
		// Order is important to ensure we glow correctly even if blocked - IsLootingForbidden must come first.
		// Check here is on the container, skip all contents if looting not permitted
		Lootability forbidden(LootingLegality(m_targetType));
		if (forbidden != Lootability::Lootable)
		{
			skipLooting = true;
			result = forbidden;
		}
		else if (DataCase::GetInstance()->ReferencesBlacklistedContainer(m_candidate))
		{
			skipLooting = true;
			result = Lootability::ContainerIsBlacklisted;
		}

		// Always allow auto-looting of dead bodies, e.g. Solitude Hall of the Dead in LCTN Solitude has skeletons that we
		// should be able to murder/plunder. And don't forget Margret in Markarth.
		if (!skipLooting && m_targetType != INIFile::SecondaryType::deadbodies &&
			!LocationTracker::Instance().IsPlayerInWhitelistedPlace(LocationTracker::Instance().PlayerCell()) &&
			LocationTracker::Instance().IsPlayerInRestrictedLootSettlement(LocationTracker::Instance().PlayerCell()))
		{
			DBG_VMESSAGE("Player location is excluded as restricted population center for this target type");
			skipLooting = true;
			result = Lootability::PopulousLocationRestrictsLooting;
		}

		if (!dryRun && m_glowReason != GlowReason::None)
		{
			ScanGovernor::Instance().GlowObject(m_candidate, ObjectGlowDurationSpecialSeconds, m_glowReason);
		}

		// TODO if it contains whitelisted items we will nonetheless skip, due to checks at the container level
		if (dryRun || skipLooting)
			return result;

		// Check if we should attempt to loot the target's contents the item. If we skip it due to looting rules, it's
		// immune from stealing.
		// If we wish to auto-steal an item we must check we are not detected, which requires a scripted check. If this
		// is the delayed autoloot operation after we find we are undetected, don't trigger that check again here.
		if (!m_stolen && m_candidate->IsOffLimits() &&
			PlayerState::Instance().EffectiveOwnershipRule() == OwnershipRule::AllowCrimeIfUndetected)
		{
			DBG_VMESSAGE("Container/deadbody contents %s/0x%08x to be stolen if undetected", m_candidate->GetName(), m_candidate->formID);
			TheftCoordinator::Instance().DelayStealableItem(m_candidate, m_targetType);
			return Lootability::ItemTheftTriggered;
		}

		// when we get to the point where looting is confirmed, block the reference to
		// avoid re-looting without a player cell or config change
		DBG_MESSAGE("block looted container %s/0x%08x", m_candidate->GetName(), m_candidate->formID);
		data->BlockReference(m_candidate);
		// Build list of lootable targets with notification and collectibility flag for each
		std::vector<std::tuple<InventoryItem, bool, bool>> targets;
		targets.reserve(lootableItems.size());
		for (auto& targetItemInfo : lootableItems)
		{
			RE::TESBoundObject* target(targetItemInfo.BoundObject());
			if (!target)
				continue;

			// crime-check this REFR from the container as individual object
			if (LootingLegality(INIFile::SecondaryType::itemObjects) != Lootability::Lootable)
			{
				continue;
			}

			if (ManagedList::BlackList().Contains(target))
			{
				DBG_VMESSAGE("block 0x%08x due to BlackList", target->formID);
				data->BlockForm(target);
				continue;
			}

			ObjectType objType = targetItemInfo.LootObjectType();
			if (excludeArmor && (objType == ObjectType::armor || objType == ObjectType::enchantedArmor))
			{
				// obey SFW setting, for this REFR on this pass - state resets on game reload/cell re-entry/MCM update
				DBG_VMESSAGE("block looting of armor from dead body %s/0x%08x", target->GetName(), target->GetFormID());
				continue;
			}

			if (objType == ObjectType::weapon || objType == ObjectType::armor || objType == ObjectType::jewelry)
			{
				bool hasEnchantment = GetEnchantmentFromExtraLists(targetItemInfo.GetExtraDataLists()) != nullptr;
				if (hasEnchantment) {
					DBG_VMESSAGE("%s/0x%08x has player-created enchantment",
						targetItemInfo.BoundObject()->GetName(), targetItemInfo.BoundObject()->GetFormID());
					switch (objType)
					{
					case ObjectType::weapon:
						objType = ObjectType::enchantedWeapon;
						break;
					case ObjectType::armor:
						objType = ObjectType::enchantedArmor;
						break;
					case ObjectType::jewelry:
						objType = ObjectType::enchantedJewelry;
						break;
					default:
						break;
					}
				}
			}

			LootingType lootingType(LootingType::LeaveBehind);
			const auto collectible(CollectionManager::Instance().TreatAsCollectible(
				ConditionMatcher(targetItemInfo.BoundObject(), m_targetType)));
			if (collectible.first)
			{
				if (IsSpecialObjectLootable(collectible.second))
				{
					DBG_VMESSAGE("Collectible Item 0x%08x", targetItemInfo.BoundObject()->formID);
					lootingType = LootingType::LootAlwaysSilent;
				}
				else
				{
					// blacklisted or 'glow'
					DBG_VMESSAGE("Collectible Item 0x%08x skipped", targetItemInfo.BoundObject()->formID);
					continue;
				}
			}
			else if (ManagedList::WhiteList().Contains(target))
			{
				// whitelisted objects are always looted silently
				DBG_VMESSAGE("transfer whitelisted 0x%08x", target->formID);
				lootingType = LootingType::LootAlwaysSilent;
			}
			else if (ManagedList::BlackList().Contains(target))
			{
				// blacklisted objects are never looted
				DBG_VMESSAGE("block blacklisted target %s/0x%08x", target->GetName(), target->GetFormID());
				data->BlockForm(target);
				continue;
			}
			else
			{
				std::string typeName = GetObjectTypeName(objType);
				lootingType = LootingTypeFromIniSetting(
					INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::itemObjects, typeName.c_str()));

				if (lootingType == LootingType::LeaveBehind)
				{
					DBG_VMESSAGE("block - typename %s excluded for 0x%08x", typeName.c_str(), target->formID);
					data->BlockForm(target);
					continue;
				}
				else if (LootingDependsOnValueWeight(lootingType, objType) &&
					TESFormHelper(target, m_targetType).ValueWeightTooLowToLoot())
				{
					DBG_VMESSAGE("block - v/w excludes for 0x%08x", target->formID);
					data->BlockForm(target);
					continue;
				}
			}

			targets.push_back({ targetItemInfo, LootingRequiresNotification(lootingType), collectible.first });
			DBG_MESSAGE("get %s (%d) from container %s/0x%08x", target->GetName(), targetItemInfo.Count(),
				m_candidate->GetName(), m_candidate->formID);
		}

		if (!targets.empty())
		{
			// check highlighting for dead NPC or container
			int playContainerAnimation(static_cast<int>(INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "PlayContainerAnimation")));
			if (playContainerAnimation > 0)
			{
				if (m_targetType == INIFile::SecondaryType::containers)
				{
					if (!GetTimeController(m_candidate))
					{
						// no container animation feasible, highlight it instead
						playContainerAnimation = 2;
					}
				}
				else
				{
					// Dead NPCs cannot be animated, but highlighting requested
					playContainerAnimation = 2;
				}
			}

			GetLootFromContainer(targets, playContainerAnimation);
		}
	}
	return result;
}

void TryLootREFR::GetLootFromContainer(std::vector<std::tuple<InventoryItem, bool, bool>>& targets, const int animationType)
{
	if (!m_candidate)
		return;

	// visual notification, if requested
	if (animationType == 1)
	{
		m_candidate->PlayAnimation("Close", "Open");
	}
	else if (animationType == 2)
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
		if (!madeSound)
		{
			RE::PlayerCharacter::GetSingleton()->PlayPickUpSound(itemInfo.BoundObject(), true, false);
			madeSound = true;
		}
		std::string name(itemInfo.BoundObject()->GetName());
		int count(itemInfo.TakeAll(m_candidate, RE::PlayerCharacter::GetSingleton(), collectible));
		if (notify)
		{
			std::string notificationText;
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
	}
}

Lootability TryLootREFR::LootingLegality(const INIFile::SecondaryType targetType)
{
	// Already trying to steal this - bypass repeat check
	if (m_stolen)
		return Lootability::PendingItemSteal;

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
				DBG_VMESSAGE("Player-owned %s, looting belongings disallowed: %s/0x%08x",
					playerOwned ? "true" : "false", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
				legality = Lootability::PlayerOwned;
				// Glow if configured
				if (PlayerState::Instance().BelongingsCheck() == SpecialObjectHandling::GlowTarget)
					UpdateGlowReason(GlowReason::PlayerProperty);
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
	for (UInt32 index = 0; index < keywordForm->GetNumKeywords(); ++index)
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
