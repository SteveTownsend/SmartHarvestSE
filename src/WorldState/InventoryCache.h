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

#include <unordered_map>

namespace shse
{

	class InventoryEntry
	{
	public:
		InventoryEntry(RE::TESBoundObject* item, const int count);
		InventoryEntry(RE::TESBoundObject* item, const ExcessInventoryExemption exemption);

		static constexpr int UnlimitedItems = 1000000;

		ExcessInventoryHandling HandlingType() const;
		void Populate();
		int Headroom(const int delta) const;
		void HandleExcess();
		int Count() const { return m_count; }
		std::string Sell(const bool excessOnly);
		std::string Transfer(const bool excessOnly);
		std::string Delete(const bool excessOnly);
		std::string Disposition();

	private:
		void init();

		RE::TESBoundObject* m_item;
		ExcessInventoryExemption m_exemption;
		ExcessInventoryHandling m_excessHandling;
		ObjectType m_excessType;
		bool m_crafting;
		int m_count;
		mutable int m_totalDelta;		// number of items assumed added by loot requests since last reconciliation
		int m_maxItems;
		int m_maxItemsByWeight;
		int m_maxCount;
		uint32_t m_value;
		uint32_t m_saleProceeds;
		double m_weight;
		double m_salePercent;
		int m_handled;
		std::string m_transferTarget;
	};

	typedef std::unordered_map<const RE::TESBoundObject*, InventoryEntry> InventoryCache;
	typedef std::unordered_set<const RE::TESBoundObject*> InventoryUpdates;
}
