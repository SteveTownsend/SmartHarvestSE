#pragma once

class InventoryItem
{
public:
	InventoryItem(std::unique_ptr<RE::InventoryEntryData> a_entry, std::ptrdiff_t a_count);
	InventoryItem(const InventoryItem& rhs);

	// returns number of objects added
	int TakeAll(RE::TESObjectREFR* container, RE::TESObjectREFR* target);

	inline RE::TESBoundObject* BoundObject() const { return m_entry->GetObject(); }
	inline std::ptrdiff_t Count() const { return m_count; }

private:
	mutable std::unique_ptr<RE::InventoryEntryData> m_entry;
	std::ptrdiff_t m_count;
};
