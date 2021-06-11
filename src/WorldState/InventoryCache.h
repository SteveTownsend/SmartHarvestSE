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
		InventoryEntry(const ObjectType excessType, const int count, const uint32_t value, const double weight);
		static constexpr int UnlimitedItems = 1000000;
		int Headroom(const RE::TESBoundObject* item, const int delta) const;
		void HandleExcess(const RE::TESBoundObject* item);

	private:
		ExcessInventoryHandling m_excessHandling;
		int m_count;
		mutable int m_totalDelta;		// number of items assumed added by loot requests since last reconciliation
		int m_maxCount;
		uint32_t m_value;
		double m_weight;
	};

	typedef std::unordered_map<const RE::TESBoundObject*, InventoryEntry> InventoryCache;
}
