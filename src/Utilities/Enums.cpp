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
#include "Utilities/Enums.h"
#include "Looting/objects.h"
#include "Looting/ManagedLists.h"

namespace shse
{

std::string LootabilityName(const Lootability lootability)
{
	switch (lootability) {
	case Lootability::Lootable: return "Lootable";
	case Lootability::BaseObjectBlocked : return "BaseObjectBlocked";
	case Lootability::CannotRelootFirehoseSource: return "CannotRelootFirehoseSource";
	case Lootability::ContainerPermanentlyOffLimits: return "ContainerPermanentlyOffLimits";
	case Lootability::CorruptArrowPosition: return "CorruptArrowPosition";
	case Lootability::CannotMineTwiceInSameCellVisit: return "CannotMineTwiceInSameCellVisit";
	case Lootability::ReferenceBlacklisted : return "ReferenceBlacklisted";
	case Lootability::UnnamedReference : return "UnnamedReference";
	case Lootability::ReferenceIsPlayer : return "ReferenceIsPlayer";
	case Lootability::ReferenceIsLiveActor : return "ReferenceIsLiveActor";
	case Lootability::FloraHarvested : return "FloraHarvested";
	case Lootability::PendingHarvest : return "PendingHarvest";
	case Lootability::ContainerLootedAlready : return "ContainerLootedAlready";
	case Lootability::DynamicReferenceLootedAlready : return "DynamicReferenceLootedAlready";
	case Lootability::NullReference : return "NullReference";
	case Lootability::InvalidFormID : return "InvalidFormID";
	case Lootability::NoBaseObject : return "NoBaseObject";
	case Lootability::LootDeadBodyDisabled : return "LootDeadBodyDisabled";
	case Lootability::DeadBodyIsPlayerAlly : return "DeadBodyIsPlayerAlly";
	case Lootability::DeadBodyIsSummoned: return "DeadBodyIsSummoned";
	case Lootability::DeadBodyIsEssential: return "DeadBodyIsEssential";
	case Lootability::DeadBodyDelayedLooting : return "DeadBodyDelayedLooting";
	case Lootability::DeadBodyPossibleDuplicate : return "DeadBodyPossibleDuplicate";
	case Lootability::LootContainersDisabled : return "LootContainersDisabled";
	case Lootability::HarvestLooseItemDisabled : return "HarvestLooseItemDisabled";
	case Lootability::PendingProducerIngredient : return "PendingProducerIngredient";
	case Lootability::ObjectTypeUnknown : return "ObjectTypeUnknown";
	case Lootability::ManualLootTarget : return "ManualLootTarget";
	case Lootability::BaseObjectOnBlacklist : return "BaseObjectOnBlacklist";
	case Lootability::CannotLootQuestTarget : return "CannotLootQuestTarget";
	case Lootability::ObjectIsInBlacklistCollection : return "ObjectIsInBlacklistCollection";
	case Lootability::CannotLootValuableObject : return "CannotLootValuableObject";
	case Lootability::CannotLootEnchantedObject: return "CannotLootEnchantedObject";
	case Lootability::CannotLootAmmo : return "CannotLootAmmo";
	case Lootability::PlayerOwned : return "PlayerOwned";
	case Lootability::CrimeToLoot : return "CrimeToLoot";
	case Lootability::CellOrItemOwnerPreventsOwnerlessLooting: return "CellOrItemOwnerPreventsOwnerlessLooting";
	case Lootability::PopulousLocationRestrictsLooting : return "PopulousLocationRestrictsLooting";
	case Lootability::ItemInBlacklistCollection : return "ItemOnBlacklistCollection";
	case Lootability::CollectibleItemSetToGlow : return "CollectibleItemSetToGlow";
	case Lootability::LawAbidingSoNoWhitelistItemLooting : return "CrimeCheckPreventsWhitelistItemLooting";
	case Lootability::ItemIsBlacklisted : return "ItemIsBlacklisted";
	case Lootability::ItemTypeIsSetToPreventLooting : return "ItemTypeIsSetToPreventLooting";
	case Lootability::HarvestDisallowedForBaseObjectType: return "HarvestDisallowedForBaseObjectType";
	case Lootability::ValueWeightPreventsLooting : return "ValueWeightPreventsLooting";
	case Lootability::ItemTheftTriggered : return "ItemTheftTriggered";
	case Lootability::HarvestOperationPending : return "HarvestOperationPending";
	case Lootability::ContainerHasNoLootableItems : return "ContainerHasNoLootableItems";
	case Lootability::ContainerIsLocked : return "ContainerIsLocked";
	case Lootability::ContainerIsBossChest : return "ContainerIsBossChest";
	case Lootability::ContainerHasQuestObject : return "ContainerHasQuestObject";
	case Lootability::ContainerHasValuableObject : return "ContainerHasValuableObject";
	case Lootability::ContainerHasEnchantedObject: return "ContainerHasEnchantedObject";
	case Lootability::ReferencesBlacklistedContainer : return "ReferencesBlacklistedContainer";
	case Lootability::CannotGetAshPile: return "CannotGetAshPile";
	case Lootability::ProducerHasNoLootable: return "ProducerHasNoLootable";
	case Lootability::ContainerBlacklistedByUser: return "ContainerBlacklistedByUser";
	case Lootability::DeadBodyBlacklistedByUser: return "DeadBodyBlacklistedByUser";
	case Lootability::NPCExcludedByDeadBodyFilter: return "NPCExcludedByDeadBodyFilter";
	case Lootability::NPCIsInBlacklistCollection: return "NPCIsInBlacklistCollection";
	case Lootability::ContainerIsLootTransferTarget: return "ContainerIsLootTransferTarget";
	case Lootability::InventoryLimitsEnforced: return "InventoryLimitsEnforced";
	case Lootability::OutOfScope: return "OutOfScope";
	default: return "";
	}
}

bool LootingDependsOnValueWeight(const LootingType lootingType, ObjectType objectType)
{
	if (IsValueWeightExempt(objectType))
	{
		DBG_VMESSAGE("No V/W check for objType {}", GetObjectTypeName(objectType));
		return false;
	}
	if (lootingType != LootingType::LootIfValuableEnoughNotify && lootingType != LootingType::LootIfValuableEnoughSilent)
	{
		DBG_VMESSAGE("No V/W check for LootingType {}", lootingType);
		return false;
	}
	DBG_VMESSAGE("V/W check required for LootingType {}, objType {}", lootingType, GetObjectTypeName(objectType));
	return true;
}

std::string ExcessInventoryExemptionString(const ExcessInventoryExemption excessInventoryExemption)
{
	switch (excessInventoryExemption) {
	case ExcessInventoryExemption::NotExempt:
		return "NotExempt";
	case ExcessInventoryExemption::QuestItem:
		return "QuestItem";
	case ExcessInventoryExemption::ItemInUse:
		return "ItemInUse";
	case ExcessInventoryExemption::IsFavouriteItem:
		return "IsFavouriteItem";
	case ExcessInventoryExemption::CountIsZero:
		return "CountIsZero";
	case ExcessInventoryExemption::Ineligible:
		return "Ineligible";
	case ExcessInventoryExemption::IsLeveledItem:
		return "IsLeveledItem";
	case ExcessInventoryExemption::NotFound:
		return "NotFound";
	default:
		return "OutOfRange";
	}
}

std::string ExcessInventoryHandlingString(const ExcessInventoryHandling excessInventoryHandling)
{
	switch (excessInventoryHandling) {
	case ExcessInventoryHandling::NoLimits:
		return "NoLimits";
	case ExcessInventoryHandling::LeaveBehind:
		return "LeaveBehind";
	case ExcessInventoryHandling::ConvertToSeptims:
		return "ConvertToSeptims";
	default:
		// get container name
		return ManagedList::TransferList().ByIndex(
			static_cast<size_t>(excessInventoryHandling) - static_cast<size_t>(ExcessInventoryHandling::Container1)).second;
	}
}

}