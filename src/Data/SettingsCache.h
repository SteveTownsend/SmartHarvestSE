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
	bool FortuneHuntItem() const;
	bool FortuneHuntNPC() const;
	bool FortuneHuntContainer() const;
	bool CollectionsEnabled() const;
	bool NotifyLocationChange() const;

	double ValuableItemThreshold() const;
	bool CheckWeightlessValue() const;
	int WeightlessMinimumValue() const;
	double ValueWeightDefault() const;
	double ValueWeight(ObjectType objectType) const;
	LootingType ObjectLootingType(ObjectType objectType) const;
	ExcessInventoryHandling ExcessInventoryHandlingType(ObjectType objectType) const;
	int ExcessInventoryCount(ObjectType objectType) const;
	double ExcessInventoryWeight(ObjectType objectType) const;
	double SaleValuePercentMultiplier() const;
	bool HandleExcessCraftingItems() const;
	ExcessInventoryHandling CraftingItemsExcessHandling() const;
	int CraftingItemsExcessCount() const;
	double CraftingItemsExcessWeight() const;

	DeadBodyLooting DeadBodyLootingType() const;
	EnchantedObjectHandling EnchantedObjectHandlingType() const;

	double DelaySeconds() const;

	OwnershipRule CrimeCheckSneaking() const;
	OwnershipRule CrimeCheckNotSneaking() const;
	SpecialObjectHandling PlayerBelongingsLoot() const;
	LockedContainerHandling LockedChestLoot() const;
	SpecialObjectHandling BossChestLoot() const;
	SpecialObjectHandling ValuableItemLoot() const;
	ContainerAnimationHandling PlayContainerAnimation() const;
	QuestObjectHandling QuestObjectLoot() const;

	bool EnableLootContainer() const;
	bool EnableHarvest() const;
	bool LootAllowedItemsInSettlement() const;
	bool LootAllowedItemsInPlayerHouse() const;
	bool UnknownIngredientLoot() const;
	bool WhiteListTargetNotify() const;
	bool ManualLootTargetNotify() const;
	PopulationCenterSize PreventPopulationCenterLooting() const;
	int16_t MaxMiningItems() const;
	bool MiningToolsRequired() const;
	bool DisallowMiningIfSneaking() const;

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
	bool m_fortuneHuntItem;
	bool m_fortuneHuntNPC;
	bool m_fortuneHuntContainer;
	bool m_collectionsEnabled;
	bool m_notifyLocationChange;
	double m_valuableItemThreshold;
	bool m_checkWeightlessValue;
	int m_weightlessMinimumValue;
	double m_valueWeightDefault;
	DeadBodyLooting m_deadBodyLooting;
	EnchantedObjectHandling m_enchantedObjectHandling;

	// Worker thread loop smallest possible delay
	static constexpr double MinThreadDelaySeconds = 0.1;

	double m_delaySeconds;
	double m_delaySecondsIndoors;
	OwnershipRule m_crimeCheckSneaking;
	OwnershipRule m_crimeCheckNotSneaking;
	SpecialObjectHandling m_playerBelongingsLoot;
	LockedContainerHandling m_lockedChestLoot;
	SpecialObjectHandling m_bossChestLoot;
	SpecialObjectHandling m_valuableItemLoot;
	ContainerAnimationHandling m_playContainerAnimation;
	QuestObjectHandling m_questObjectLoot;
	bool m_enableLootContainer;
	bool m_enableHarvest;
	bool m_lootAllowedItemsInSettlement;
	bool m_lootAllowedItemsInPlayerHouse;
	bool m_unknownIngredientLoot;
	bool m_whiteListTargetNotify;
	bool m_manualLootTargetNotify;
	PopulationCenterSize m_preventPopulationCenterLooting;
	int16_t m_maxMiningItems;
	bool m_miningToolsRequired;
	bool m_disallowMiningIfSneaking;

	static constexpr size_t TypeCount = size_t(ObjectType::oreVein);

	std::array<LootingType, TypeCount> m_lootingType;
	std::array<ExcessInventoryHandling, TypeCount> m_excessHandling;
	std::array<int, TypeCount> m_excessCount;
	std::array<double, TypeCount> m_excessWeight;
	std::array<double, TypeCount> m_valueWeight;
	double m_saleValuePercentMultiplier;
	bool m_handleExcessCraftingItems;
	ExcessInventoryHandling m_craftingItemsExcessHandling;
	int m_craftingItemsExcessCount;
	double m_craftingItemsExcessWeight;
};

}
