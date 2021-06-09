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
#include "Data/dataCase.h"
#include "Data/SettingsCache.h"
#include "Looting/ManagedLists.h"

namespace shse
{

InventoryEntry::InventoryEntry(const ObjectType excessType, const int count, const uint32_t value, const double weight) :
	m_excessHandling(SettingsCache::Instance().ExcessInventoryHandlingType(excessType)),
	m_count(count),
	m_value(value),
	m_weight(weight)
{
	m_maxCount = SettingsCache::Instance().ExcessInventoryCount(excessType);
	double maxWeight(SettingsCache::Instance().ExcessInventoryWeight(excessType));
	if (weight > 0.0 && maxWeight > 0.0)
	{
		// convert to # of items and round up
		int maxItemsByWeight(static_cast<int>(maxWeight / weight) + 1);
		m_maxCount = std::min(m_maxCount, maxItemsByWeight);
	}
}

int InventoryEntry::Headroom() const
{
	if (m_excessHandling == ExcessInventoryHandling::LeaveBehind)
	{
		return std::max(0, m_maxCount - m_count);
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