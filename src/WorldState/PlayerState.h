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

#include "WorldState/LocationTracker.h"

namespace shse
{

class PlayerState
{
public:

	static PlayerState& Instance();
	PlayerState();

	void Refresh();
	void ResetCarryWeight(const bool reloaded);
	void CheckPerks(const bool force);
	bool PerksAddLeveledItemsOnDeath() const;
	float PerkIngredientMultiplier() const;
	bool CanLoot() const;
	bool IsSneaking() const;
	OwnershipRule EffectiveOwnershipRule() const { return m_ownershipRule; }
	SpecialObjectHandling BelongingsCheck() const { return m_belongingsCheck; }
	double SneakDistanceExterior() const;
	double SneakDistanceInterior() const;
	void ExcludeMountedIfForbidden(void);
	Position GetPosition() const;
	AlglibPosition GetAlglibPosition() const;
	bool WithinDetectionRange(const double distance) const;

private:
	void AdjustCarryWeight();
	bool IsMagicallyConcealed(RE::MagicTarget* target) const;

	static std::unique_ptr<PlayerState> m_instance;

	static constexpr int InfiniteWeight = 100000;
	static constexpr int PerkCheckIntervalSeconds = 15;
	std::chrono::time_point<std::chrono::high_resolution_clock> m_lastPerkCheck;
	bool m_perksAddLeveledItemsOnDeath;
	float m_harvestedIngredientMultiplier;

	bool m_carryAdjustedForCombat;
	bool m_carryAdjustedForPlayerHome;
	bool m_carryAdjustedForDrawnWeapon;
	int m_currentCarryWeightChange;

	bool m_sneaking;
	OwnershipRule m_ownershipRule;
	SpecialObjectHandling m_belongingsCheck;
	bool m_disableWhileMounted;

	mutable RecursiveLock m_playerLock;
};

}