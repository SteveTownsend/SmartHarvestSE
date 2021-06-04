/*************************************************************************
SmartHarvest SE
Copyright (c) Steve Townsend 2021

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

#include "WorldState/PopulationCenters.h"

namespace shse
{

class SettingsCache {
public:
	static SettingsCache& Instance();
	SettingsCache();
	void Refresh(void);

	double OutdoorsRadius() const;
	double IndoorsRadius() const;
	double VerticalFactor() const;

	bool RespectDoors() const;
	bool UnencumberedInPlayerHome() const;
	bool UnencumberedIfWeaponDrawn() const;
	bool UnencumberedInCombat() const;
	bool DisableDuringCombat() const;
	bool DisableWhileWeaponIsDrawn() const;
	bool DisableWhileConcealed() const;
	bool FortuneHuntingEnabled() const;
	bool CollectionsEnabled() const;
	bool NotifyLocationChange() const;

	double ValuableItemThreshold() const;
	double ValueWeightDefault() const;
	double ValueWeight(ObjectType objectType) const;
	LootingType ObjectLootingType(ObjectType objectType) const;
	ExcessInventoryHandling ExcessInventoryHandlingType(ObjectType objectType) const;

	DeadBodyLooting DeadBodyLootingType() const;
	EnchantedObjectHandling EnchantedObjectHandlingType() const;

	double DelaySeconds() const;
	OwnershipRule CrimeCheckSneaking() const;
	OwnershipRule CrimeCheckNotSneaking() const;
	SpecialObjectHandling PlayerBelongingsLoot() const;
	SpecialObjectHandling LockedChestLoot() const;
	SpecialObjectHandling BossChestLoot() const;
	SpecialObjectHandling ValuableItemLoot() const;
	ContainerAnimationHandling PlayContainerAnimation() const;
	QuestObjectHandling QuestObjectLoot() const;

	bool EnableLootContainer() const;
	bool EnableHarvest() const;
	bool LootAllowedItemsInSettlement() const;
	bool UnknownIngredientLoot() const;
	bool WhiteListTargetNotify() const;
	bool ManualLootTargetNotify() const;
	PopulationCenterSize PreventPopulationCenterLooting() const;
	int16_t MaxMiningItems() const;

private:
	static std::unique_ptr<SettingsCache> m_instance;
	double m_outdoorsRadius;
	double m_indoorsRadius;
	double m_verticalFactor;
	bool m_respectDoors;
	bool m_unencumberedInPlayerHome;
	bool m_unencumberedIfWeaponDrawn;
	bool m_unencumberedInCombat;
	bool m_disableDuringCombat;
	bool m_disableWhileWeaponIsDrawn;
	bool m_disableWhileConcealed;
	bool m_fortuneHuntingEnabled;
	bool m_collectionsEnabled;
	bool m_notifyLocationChange;
	double m_valuableItemThreshold;
	double m_valueWeightDefault;
	DeadBodyLooting m_deadBodyLooting;
	EnchantedObjectHandling m_enchantedObjectHandling;
	double m_delaySeconds;
	OwnershipRule m_crimeCheckSneaking;
	OwnershipRule m_crimeCheckNotSneaking;
	SpecialObjectHandling m_playerBelongingsLoot;
	SpecialObjectHandling m_lockedChestLoot;
	SpecialObjectHandling m_bossChestLoot;
	SpecialObjectHandling m_valuableItemLoot;
	ContainerAnimationHandling m_playContainerAnimation;
	QuestObjectHandling m_questObjectLoot;
	bool m_enableLootContainer;
	bool m_enableHarvest;
	bool m_lootAllowedItemsInSettlement;
	bool m_unknownIngredientLoot;
	bool m_whiteListTargetNotify;
	bool m_manualLootTargetNotify;
	PopulationCenterSize m_preventPopulationCenterLooting;
	int16_t m_maxMiningItems;

	static constexpr size_t TypeCount = size_t(ObjectType::oreVein);

	std::array<LootingType, TypeCount> m_lootingType;
	std::array<ExcessInventoryHandling, TypeCount> m_excessHandling;
	std::array<double, TypeCount> m_valueWeight;
};

}
