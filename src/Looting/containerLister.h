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

#include "InventoryItem.h"
#include "Data/SettingsCache.h"
#include "WorldState/InventoryCache.h"

namespace shse
{

typedef std::vector<InventoryItem> LootableItems;

struct ContainerLister
{
public:
	ContainerLister(const INIFile::SecondaryType targetType, const RE::TESObjectREFR* refr);
	size_t AnalyzeLootableItems(const EnchantedObjectHandling enchantedObjectHandling);
	void FilterLootableItems(std::function<bool(RE::TESBoundObject*)> predicate);
	size_t CountLootableItems(std::function<bool(RE::TESBoundObject*)> predicate);
	inline bool HasQuestItem() const { return !m_questItems.empty(); }
	inline bool HasEnchantedItem() const { return !m_enchantedItems.empty(); }
	inline bool HasValuableItem() const { return !m_valuableItems.empty(); }
	inline bool HasCollectibleItem() const { return !m_collectibleItems.empty(); }
	inline CollectibleHandling CollectibleAction() const { return m_collectibleAction; }
	inline const LootableItems& GetLootableItems() const { return m_lootableItems; }
	InventoryCache CacheIfExcessHandlingEnabled(const bool force, const InventoryUpdates& updates) const;
	std::string SellItem(RE::TESBoundObject* target, const bool excessOnly);
	std::string TransferItem(RE::TESBoundObject* target, const bool excessOnly);
	std::string DeleteItem(RE::TESBoundObject* target, const bool excessOnly);
	std::string CheckItemAsExcess(RE::TESBoundObject* target);
	void ExcludeQuestItems() { RemoveUnlootable(m_questItems); }
	void ExcludeEnchantedItems() { RemoveUnlootable(m_enchantedItems); }
	void ExcludeValuableItems() { RemoveUnlootable(m_valuableItems); }
	void ExcludeCollectibleItems() { RemoveUnlootable(m_collectibleItems); }

private:
	void RemoveUnlootable(const std::unordered_set<RE::TESBoundObject*>& filter);
	InventoryEntry GetSingleInventoryEntry(RE::TESBoundObject* target) const;

	const RE::TESObjectREFR* m_refr;
	INIFile::SecondaryType m_targetType;
	EnchantedObjectHandling m_enchantedLoot;
	std::unordered_set<RE::TESBoundObject*> m_questItems;
	std::unordered_set<RE::TESBoundObject*> m_enchantedItems;
	std::unordered_set<RE::TESBoundObject*> m_valuableItems;
	std::unordered_set<RE::TESBoundObject*> m_collectibleItems;
	LootableItems m_lootableItems;
	CollectibleHandling m_collectibleAction;
};

}