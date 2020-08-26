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
#include "Data/iniSettings.h"
#include "Looting/InventoryItem.h"

namespace shse
{

class TryLootREFR
{
public:
	TryLootREFR(RE::TESObjectREFR* target, INIFile::SecondaryType targetType, const bool stolen, const bool glowOnly);
	Lootability Process(const bool dryRun);
	inline std::string ObjectTypeName() const { return m_typeName; }

private:
	bool m_stolen;
	bool m_glowOnly;
	RE::TESObjectREFR* m_candidate;
	INIFile::SecondaryType m_targetType;
	std::string m_typeName;

	Lootability ItemLootingLegality(const bool isCollectible, INIFile::SecondaryType targetType);
	Lootability LootingLegality(const INIFile::SecondaryType targetType);
	void GetLootFromContainer(std::vector<std::tuple<InventoryItem, bool, bool, size_t>>& targets,
		const int animationType, const bool directTransfer);
	void CopyLootFromContainer(std::vector<std::tuple<InventoryItem, bool, bool, size_t>>& targets);

	// special object glow - not too long, in case we loot or move away. Used for Loot Sense too.
	static constexpr int ObjectGlowDurationSpecialSeconds = 10;
	// brief glow for looted objects and other purposes
	static constexpr int ObjectGlowDurationLootedSeconds = 2;

	GlowReason m_glowReason;
	inline void UpdateGlowReason(const GlowReason glowReason)
	{
		if (glowReason < m_glowReason)
			m_glowReason = glowReason;
	}
	bool IsBookGlowable() const;
};

}
