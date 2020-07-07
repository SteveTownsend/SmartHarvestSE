#pragma once

#include "Data/iniSettings.h"

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
