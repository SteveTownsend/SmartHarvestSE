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

namespace shse
{

typedef std::vector<InventoryItem> LootableItems;

struct ContainerLister
{
public:
	ContainerLister(const INIFile::SecondaryType targetType, const RE::TESObjectREFR* refr, const bool requireQuestItemAsTarget, const bool checkSpecials);
	LootableItems GetOrCheckContainerForms();
	inline bool HasQuestItem() const { return m_hasQuestItem; }
	inline bool HasEnchantedItem() const { return m_hasEnchantedItem; }
	inline bool HasValuableItem() const { return m_hasValuableItem; }
	inline bool HasCollectibleItem() const { return m_hasCollectibleItem; }
	inline CollectibleHandling CollectibleAction() const { return m_collectibleAction; }

private:
	const RE::TESObjectREFR* m_refr;
	INIFile::SecondaryType m_targetType;
	bool m_requireQuestItemAsTarget;
	bool m_checkSpecials;
	bool m_hasQuestItem;
	bool m_hasEnchantedItem;
	bool m_hasValuableItem;
	bool m_hasCollectibleItem;
	CollectibleHandling m_collectibleAction;
};

}