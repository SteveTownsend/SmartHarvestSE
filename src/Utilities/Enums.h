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
#pragma once

#include "Looting/ObjectType.h"

namespace shse
{

// object glow reasons, in descending order of precedence
enum class GlowReason {
	LockedContainer = 1,
	BossContainer,
	QuestObject,
	Collectible,
	Valuable,
	EnchantedItem,
	PlayerProperty,
	SimpleTarget,
	None
};

inline GlowReason CycleGlow(const GlowReason current)
{
	int next(int(current) + 1);
	if (next > int(GlowReason::SimpleTarget))
		return GlowReason::LockedContainer;
	return GlowReason(next);
}

inline std::string GlowName(const GlowReason glow)
{
	switch (glow) {
	case GlowReason::LockedContainer:
		return "Locked";
	case GlowReason::BossContainer:
		return "Boss";
	case GlowReason::QuestObject:
		return "Quest";
	case GlowReason::Collectible:
		return "Collectible";
	case GlowReason::EnchantedItem:
		return "Enchanted";
	case GlowReason::Valuable:
		return "Valuable";
	case GlowReason::PlayerProperty:
		return "PlayerOwned";
	case GlowReason::SimpleTarget:
		return "Looted";
	default:
		return "Unknown";
	}
}

enum class LootingType {
	LeaveBehind = 0,
	LootAlwaysSilent,
	LootAlwaysNotify,
	LootIfValuableEnoughSilent,
	LootIfValuableEnoughNotify,
	MAX
};

inline bool LootingRequiresNotification(const LootingType lootingType)
{
	return lootingType == LootingType::LootIfValuableEnoughNotify || lootingType == LootingType::LootAlwaysNotify;
}

inline LootingType LootingTypeFromIniSetting(const double iniSetting)
{
	UInt32 intSetting(static_cast<UInt32>(iniSetting));
	if (intSetting >= static_cast<SInt32>(LootingType::MAX))
	{
		return LootingType::LeaveBehind;
	}
	return static_cast<LootingType>(intSetting);
}

enum class SpecialObjectHandling {
	DoNotLoot = 0,
	DoLoot,
	GlowTarget,
	MAX
};

constexpr std::pair<bool, SpecialObjectHandling> NotCollectible = { false, SpecialObjectHandling::DoNotLoot };

inline SpecialObjectHandling UpdateSpecialObjectHandling(const SpecialObjectHandling initial, const SpecialObjectHandling next)
{
	// update if new is more permissive
	if (next == SpecialObjectHandling::DoLoot)
	{
		return next;
	}
	else if (next == SpecialObjectHandling::GlowTarget)
	{
		return initial == SpecialObjectHandling::DoLoot ? initial : next;
	}
	else
	{
		// this is the least permissive - initial cannot be any less so
		return initial;
	}
}

inline bool IsSpecialObjectLootable(const SpecialObjectHandling specialObjectHandling)
{
	return specialObjectHandling == SpecialObjectHandling::DoLoot;
}

inline std::string SpecialObjectHandlingJSON(const SpecialObjectHandling specialObjectHandling)
{
	switch (specialObjectHandling) {
	case SpecialObjectHandling::DoLoot:
		return "take";
	case SpecialObjectHandling::GlowTarget:
		return "glow";
	case SpecialObjectHandling::DoNotLoot:
	default:
		return "leave";
	}
}

inline SpecialObjectHandling ParseSpecialObjectHandling(const std::string& action)
{
	if (action == "take")
		return SpecialObjectHandling::DoLoot;
	if (action == "glow")
		return SpecialObjectHandling::GlowTarget;
	return SpecialObjectHandling::DoNotLoot;
}

inline SpecialObjectHandling SpecialObjectHandlingFromIniSetting(const double iniSetting)
{
	UInt32 intSetting(static_cast<UInt32>(iniSetting));
	if (intSetting >= static_cast<SInt32>(SpecialObjectHandling::MAX))
	{
		return SpecialObjectHandling::DoNotLoot;
	}
	return static_cast<SpecialObjectHandling>(intSetting);
}

inline bool LootingDependsOnValueWeight(const LootingType lootingType, ObjectType objectType)
{
	if (objectType == ObjectType::septims ||
		objectType == ObjectType::key ||
		objectType == ObjectType::oreVein ||
		objectType == ObjectType::lockpick)
		return false;
	if (lootingType != LootingType::LootIfValuableEnoughNotify && lootingType != LootingType::LootIfValuableEnoughSilent)
		return false;
	return true;
}

enum class DeadBodyLooting {
	DoNotLoot = 0,
	LootExcludingArmor,
	LootAll,
	MAX
};

inline DeadBodyLooting DeadBodyLootingFromIniSetting(const double iniSetting)
{
	UInt32 intSetting(static_cast<UInt32>(iniSetting));
	if (intSetting >= static_cast<SInt32>(DeadBodyLooting::MAX))
	{
		return DeadBodyLooting::DoNotLoot;
	}
	return static_cast<DeadBodyLooting>(intSetting);
}

// the approximate compass direction to take to reach a target
enum class CompassDirection {
	North = 0,
	NorthEast,
	East,
	SouthEast,
	South,
	SouthWest,
	West,
	NorthWest,
	AlreadyThere,	// no movement needed to get to target
	MAX
};

inline std::string CompassDirectionName(const CompassDirection direction)
{
	switch (direction) {
	case CompassDirection::North:
		return "north";
	case CompassDirection::NorthEast:
		return "northeast";
	case CompassDirection::East:
		return "east";
	case CompassDirection::SouthEast:
		return "southeast";
	case CompassDirection::South:
		return "south";
	case CompassDirection::SouthWest:
		return "southwest";
	case CompassDirection::West:
		return "west";
	case CompassDirection::NorthWest:
		return "northwest";
	default:
		return "";
	}
}

enum class OwnershipRule {
	AllowCrimeIfUndetected = 0,
	DisallowCrime,
	Ownerless,
	MAX
};

inline OwnershipRule OwnershipRuleFromIniSetting(const double iniSetting)
{
	UInt32 intSetting(static_cast<UInt32>(iniSetting));
	if (intSetting >= static_cast<SInt32>(OwnershipRule::MAX))
	{
		return OwnershipRule::Ownerless;
	}
	return static_cast<OwnershipRule>(intSetting);
}

// Cell Ownership - lower values are less restrictive
enum class CellOwnership {
	NoOwner = 0,
	Player,
	PlayerFaction,
	OtherFaction,
	NPC,
	MAX
};

inline bool IsPlayerFriendly(const CellOwnership ownership)
{
	return ownership != CellOwnership::OtherFaction && ownership != CellOwnership::NPC;
}

inline std::string CellOwnershipName(const CellOwnership ownership)
{
	switch (ownership) {
	case CellOwnership::NoOwner:
		return "NoOwner";
	case CellOwnership::Player:
		return "Player";
	case CellOwnership::PlayerFaction:
		return "PlayerFaction";
	case CellOwnership::OtherFaction:
		return "OtherFaction";
	case CellOwnership::NPC:
		return "NPC";
	default:
		return "";
	}
}

enum class Lootability {
	Lootable = 0,
	BaseObjectBlocked,
	CannotRelootFirehoseSource,
	ContainerPermanentlyOffLimits,
	CorruptArrowPosition,
	CannotMineTwiceInSameCellVisit,
	ReferenceBlacklisted,
	UnnamedReference,
	ReferenceIsPlayer,
	ReferenceIsLiveActor,
	FloraHarvested,
	PendingHarvest,
	ContainerLootedAlready,
	DynamicContainerLootedAlready,
	NullReference,
	InvalidFormID,
	NoBaseObject,
	LootDeadBodyDisabled,
	DeadBodyIsPlayerAlly,
	DeadBodyIsSummoned,
	DeadBodyIsEssential,
	DeadBodyDelayedLooting,
	DeadBodyPossibleDuplicate,
	LootContainersDisabled,
	HarvestLooseItemDisabled,
	PendingProducerIngredient,
	ObjectTypeUnknown,
	ManualLootTarget,
	BaseObjectOnBlacklist,
	CannotLootQuestObject,
	CannotLootCollectibleObject,
	CannotLootValuableObject,
	CannotLootAmmo,
	PlayerOwned,
	CrimeToLoot,
	CellOrItemOwnerPreventsOwnerlessLooting,
	PopulousLocationRestrictsLooting,
	ItemInBlacklistCollection,
	CollectibleItemSetToGlow,
	LawAbidingSoNoWhitelistItemLooting,
	ItemIsBlacklisted,
	ItemTypeIsSetToPreventLooting,
	ValueWeightPreventsLooting,
	ItemTheftTriggered,
	HarvestOperationPending,
	ContainerHasNoLootableItems,
	ContainerIsLocked,
	ContainerIsBossChest,
	ContainerHasQuestObject,
	ContainerHasValuableObject,
	ContainerHasCollectibleObject,
	ContainerIsBlacklisted,
	MAX
};

inline bool LootIfCollectible(const Lootability lootability)
{
	return lootability == Lootability::PlayerOwned ||
		lootability == Lootability::CellOrItemOwnerPreventsOwnerlessLooting;
}

std::string LootabilityName(const Lootability lootability);

}
