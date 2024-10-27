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

namespace shse
{
#ifdef GetObject
#undef GetObject
#endif

class InventoryItem
{
public:
	InventoryItem(std::unique_ptr<RE::InventoryEntryData> a_entry, std::ptrdiff_t a_count, const EnchantedObjectHandling enchantedObjectHandling);
	InventoryItem(const InventoryItem& rhs);

	// returns number of objects added
	size_t TakeAll(RE::TESObjectREFR* container, RE::TESObjectREFR* target, const bool inlineTransfer);

	inline RE::TESBoundObject* BoundObject() const { return m_entry->GetObject(); }
	inline ObjectType LootObjectType() const { return m_objectType; }
	inline std::ptrdiff_t Count() const { return m_count; }
	void MakeCopies(RE::TESObjectREFR* target, size_t count);

private:
	void Remove(
		RE::TESObjectREFR* container, RE::TESObjectREFR* target, RE::ExtraDataList* extraDataList, ptrdiff_t count);

	bool m_inlineTransfer;
	mutable std::unique_ptr<RE::InventoryEntryData> m_entry;
	const std::ptrdiff_t m_count;
	ObjectType m_objectType;
};

}