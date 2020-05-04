#include "PrecompiledHeaders.h"
#include "InventoryItem.h"

InventoryItem::InventoryItem(std::unique_ptr<RE::InventoryEntryData> a_entry, std::ptrdiff_t a_count) : m_entry(std::move(a_entry)), m_count(a_count) {}
InventoryItem::InventoryItem(const InventoryItem& rhs) : m_entry(std::move(rhs.m_entry)), m_count(rhs.m_count) {}

// returns number of objects added
int InventoryItem::TakeAll(RE::TESObjectREFR* container, RE::TESObjectREFR* target)
{
	auto toRemove = m_count;
	if (toRemove <= 0) {
		return 0;
	}
#if _DEBUG
	_DMESSAGE("get %s/0x%08x (%d)", m_entry->GetObject()->GetName(), m_entry->GetObject()->GetFormID(), toRemove);
#endif

	std::vector<std::pair<RE::ExtraDataList*, std::ptrdiff_t>> queued;
	auto object = m_entry->GetObject();
	if (m_entry->extraLists) {
		for (auto& xList : *m_entry->extraLists) {
			if (xList) {
				auto xCount = std::min<std::ptrdiff_t>(xList->GetCount(), toRemove);
#if _DEBUG
				_DMESSAGE("Handle extra list %s (%d)", xList->GetDisplayName(object), xCount);
#endif
				toRemove -= xCount;
				queued.push_back(std::make_pair(xList, xCount));

				if (toRemove <= 0) {
					break;
				}
			}
		}
	}

	for (auto& elem : queued) {
#if _DEBUG
		_DMESSAGE("Move extra list %s (%d)", elem.first->GetDisplayName(object), elem.second);
#endif
		//target->AddObjectToContainer(object, elem.first, static_cast<SInt32>(elem.second), container);
		container->RemoveItem(object, static_cast<SInt32>(elem.second), RE::ITEM_REMOVE_REASON::kRemove, elem.first, target);
	}
	if (toRemove > 0) {
#if _DEBUG
		_DMESSAGE("Move item %s (%d)", object->GetName(), toRemove);
#endif
		//target->AddObjectToContainer(object, nullptr, static_cast<SInt32>(toRemove), container);
		container->RemoveItem(object, static_cast<SInt32>(toRemove), RE::ITEM_REMOVE_REASON::kRemove, nullptr, target);
	}
	return static_cast<int>(toRemove + queued.size());
}
