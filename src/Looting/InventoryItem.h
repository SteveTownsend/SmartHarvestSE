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

class InventoryItem
{
public:
	InventoryItem(const INIFile::SecondaryType targetType, std::unique_ptr<RE::InventoryEntryData> a_entry, std::ptrdiff_t a_count);
	InventoryItem(const InventoryItem& rhs);

	// returns number of objects added
	int TakeAll(RE::TESObjectREFR* container, RE::TESObjectREFR* target, const bool collectible);

	inline RE::BSSimpleList<RE::ExtraDataList*>* GetExtraDataLists() const { return m_entry->extraLists; }
	inline RE::TESBoundObject* BoundObject() const { return m_entry->GetObject(); }
	inline ObjectType LootObjectType() const { return m_objectType; }
	inline std::ptrdiff_t Count() const { return m_count; }

private:
	void Remove(RE::TESObjectREFR* container, RE::TESObjectREFR* target, RE::ExtraDataList* extraDataList,
		ptrdiff_t count, const bool collectible);

	const INIFile::SecondaryType m_targetType;
	mutable std::unique_ptr<RE::InventoryEntryData> m_entry;
	const std::ptrdiff_t m_count;
	const ObjectType m_objectType;
};

}