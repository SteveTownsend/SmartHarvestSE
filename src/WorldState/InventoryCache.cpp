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
#include "PrecompiledHeaders.h"
#include "WorldState/InventoryCache.h"
#include "WorldState/CraftingItems.h"
#include "FormHelpers/FormHelper.h"
#include "Data/dataCase.h"
#include "Data/SettingsCache.h"
#include "Looting/ManagedLists.h"

namespace shse
{

InventoryEntry::InventoryEntry(RE::TESBoundObject* item, const int count) :
	m_item(item),
	m_count(count),
	m_totalDelta(0)
{
	m_crafting = SettingsCache::Instance().HandleExcessCraftingItems() && CraftingItems::Instance().IsCraftingItem(m_item);
	if (m_crafting)
	{
		m_excessType = ObjectType::unknown;
		m_excessHandling = SettingsCache::Instance().CraftingItemsExcessHandling();
	}
	else
	{
		m_excessType = GetExcessObjectType(m_item);
		m_excessHandling = SettingsCache::Instance().ExcessInventoryHandlingType(m_excessType);
	}
}

ExcessInventoryHandling InventoryEntry::HandlingType() const
{
	return m_excessHandling;
}

void InventoryEntry::Populate()
{
	TESFormHelper helper(m_item, m_excessType, INIFile::SecondaryType::itemObjects);
	m_value = helper.GetWorth();
	double weight(helper.GetWeight());
	double maxWeight(0.0);
	if (m_crafting)
	{
		m_maxCount = SettingsCache::Instance().CraftingItemsExcessCount();
		maxWeight = SettingsCache::Instance().CraftingItemsExcessWeight();
	}
	else
	{
		m_maxCount = SettingsCache::Instance().ExcessInventoryCount(m_excessType);
		maxWeight = SettingsCache::Instance().ExcessInventoryWeight(m_excessType);
	}
	DBG_DMESSAGE("Excess handling for {} item {}/0x{:08x}, count={} vs {}, per-item weight={:0.2f} vs per-item total {}",
		(m_crafting ? "crafting" : "non-crafting"), m_item->GetName(), m_item->GetFormID(), m_count, m_maxCount, weight, maxWeight);
	if (weight > 0.0 && maxWeight > 0.0)
	{
		// convert to # of items and round down (by casting)
		int maxItemsByWeight(static_cast<int>(maxWeight / weight));
		m_maxCount = std::min(m_maxCount, maxItemsByWeight);
	}
}

int InventoryEntry::Headroom(const int delta) const
{
	// record item count in aggregate delta to avoid over-looting of locally-abundant items in the 'Leave Behind' category
	if (m_excessHandling == ExcessInventoryHandling::LeaveBehind)
	{
		DBG_VMESSAGE("Check LeaveBehind for {}/0x{:08x}: delta {} vs max {}, cached {}, total-delta {}",
			m_item->GetName(), m_item->GetFormID(), delta, m_maxCount, m_count, m_totalDelta);
		int headroom = std::max(0, m_maxCount - (m_count + m_totalDelta));
		m_totalDelta += headroom;
		return headroom;
	}
	else
	{
		// will always loot this many
		m_totalDelta += delta;
	}
	// all other cases are reconciled periodically, so take them and sort it out then
	return UnlimitedItems;
}

void InventoryEntry::HandleExcess(const RE::TESBoundObject* item)
{
	static RE::TESBoundObject* goldItem(RE::TESForm::LookupByID<RE::TESBoundObject>(DataCase::Gold));
	if (m_excessHandling == ExcessInventoryHandling::NoLimits || m_excessHandling == ExcessInventoryHandling::LeaveBehind)
	{
		return;
	}
	int excess(m_count - m_maxCount);
	if (excess > 0)
	{
		if (m_excessHandling == ExcessInventoryHandling::ConvertToSeptims)
		{
			RE::PlayerCharacter::GetSingleton()->RemoveItem(
				const_cast<RE::TESBoundObject*>(item), excess, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
			uint32_t payment(static_cast<uint32_t>(static_cast<double>(excess * m_value) * SettingsCache::Instance().SaleValuePercentMultiplier()));
			if (payment > 0)
			{
				RE::PlayerCharacter::GetSingleton()->AddObjectToContainer(goldItem, nullptr, payment, nullptr);
				DBG_VMESSAGE("Sold excess {} of {}/0x{:08x} for {} gold", excess, item->GetName(), item->GetFormID(), payment);
			}
			else
			{
				DBG_VMESSAGE("Excess {} of {}/0x{:08x} discarded as worthless", excess, item->GetName(), item->GetFormID());
			}
		}
		else
		{
			size_t index(static_cast<size_t>(m_excessHandling) - static_cast<size_t>(ExcessInventoryHandling::Container1));
			RE::TESForm* form(ManagedList::TransferList().ByIndex(index));
			RE::TESObjectREFR* refr(form->As<RE::TESObjectREFR>());
			if (refr)
			{
				// script on REFR indicates we should check for ACTI with link to a Container
				const RE::TESObjectACTI* activator(refr->GetBaseObject()->As<RE::TESObjectACTI>());
				if (activator)
				{
					DBG_VMESSAGE("Check ACTI {}/0x{:08x} for linked container", activator->GetFullName(), activator->GetFormID());
					refr = refr->GetLinkedRef(nullptr);
				}
			}
			if (refr && refr->GetBaseObject()->As<RE::TESObjectCONT>())
			{
				RE::PlayerCharacter::GetSingleton()->RemoveItem(
					const_cast<RE::TESBoundObject*>(item), excess, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				refr->AddObjectToContainer(const_cast<RE::TESBoundObject*>(item), nullptr, excess, nullptr);
				DBG_VMESSAGE("Moved excess {} of {}/0x{:08x} to Container for REFR 0x{:08x}",
					excess, item->GetName(), item->GetFormID(), refr->GetFormID());
			}
		}
	}
}

}