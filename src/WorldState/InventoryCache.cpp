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
#include "Utilities/utils.h"

namespace shse
{

InventoryEntry::InventoryEntry(RE::TESBoundObject* item, const int count) :
	m_item(item),
	m_exemption(ExcessInventoryExemption::NotExempt),
	m_count(count),
	m_totalDelta(0),
	m_maxItems(0),
	m_maxItemsByWeight(0),
	m_maxCount(0),
	m_value(0),
	m_saleProceeds(0),
	m_weight(0.0),
	m_salePercent(0.0),
	m_handled(0)
{
	init();
}

InventoryEntry::InventoryEntry(RE::TESBoundObject* item, const ExcessInventoryExemption exemption) :
	m_item(item),
	m_exemption(exemption),
	m_count(0),
	m_totalDelta(0),
	m_maxItems(0),
	m_maxItemsByWeight(0),
	m_maxCount(0),
	m_value(0),
	m_saleProceeds(0),
	m_weight(0.0),
	m_salePercent(0.0),
	m_handled(0)
{
	init();
}

void InventoryEntry::init()
{
	m_crafting = SettingsCache::Instance().HandleExcessCraftingItems() && CraftingItems::Instance().IsCraftingItem(m_item);
	m_excessType = GetExcessObjectType(m_item);
	if (m_crafting)
	{
		m_excessHandling = SettingsCache::Instance().CraftingItemsExcessHandling();
	}
	else
	{
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
	// for crafting items, we use a more permissive limit per item type, if set
	m_maxItems = SettingsCache::Instance().ExcessInventoryCount(m_excessType);
	maxWeight = SettingsCache::Instance().ExcessInventoryWeight(m_excessType);
	if (m_crafting)
	{
		m_maxItems = std::max(m_maxItems, SettingsCache::Instance().CraftingItemsExcessCount());
		maxWeight = std::max(maxWeight, SettingsCache::Instance().CraftingItemsExcessWeight());
	}
	if (weight > 0.0 && maxWeight > 0.0)
	{
		// if weight configured, convert to # of items and round down by casting, after incrementing with weight epsilon
		m_maxItemsByWeight = static_cast<int>((maxWeight / weight) + 0.001);
		m_maxCount = std::min(m_maxItems, m_maxItemsByWeight);
	}
	else
	{
		m_maxCount = m_maxItems;
	}
	DBG_DMESSAGE("Excess handling for {} item {}/0x{:08x} type={}, item count={} vs max {}, per-item weight={:0.2f} vs per-item total {} -> {} items",
		(m_crafting ? "crafting" : "non-crafting"), m_item->GetName(), m_item->GetFormID(), GetObjectTypeName(m_excessType),
		m_count, m_maxItems, weight, maxWeight, m_maxItemsByWeight);
	m_salePercent = SettingsCache::Instance().SaleValuePercentMultiplier();
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

void InventoryEntry::HandleExcess()
{
	static RE::TESBoundObject* goldItem(RE::TESForm::LookupByID<RE::TESBoundObject>(DataCase::Gold));
	if (m_excessHandling == ExcessInventoryHandling::NoLimits || m_excessHandling == ExcessInventoryHandling::LeaveBehind)
	{
		return;
	}
	m_handled = std::max(m_count - m_maxCount, 0);
	if (m_handled > 0)
	{
		if (m_excessHandling == ExcessInventoryHandling::ConvertToSeptims)
		{
			RE::PlayerCharacter::GetSingleton()->RemoveItem(
				const_cast<RE::TESBoundObject*>(m_item), m_handled, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
			m_saleProceeds = static_cast<uint32_t>(static_cast<double>(m_handled * m_value) * m_salePercent);
			if (m_saleProceeds > 0)
			{
				RE::PlayerCharacter::GetSingleton()->AddObjectToContainer(goldItem, nullptr, m_saleProceeds, nullptr);
				DBG_VMESSAGE("Sold excess {} of {}/0x{:08x} for {} gold", m_handled, m_item->GetName(), m_item->GetFormID(), m_saleProceeds);
			}
			else
			{
				DBG_VMESSAGE("Excess {} of {}/0x{:08x} discarded as worthless", m_handled, m_item->GetName(), m_item->GetFormID());
			}
		}
		else
		{
			size_t index(static_cast<size_t>(m_excessHandling) - static_cast<size_t>(ExcessInventoryHandling::Container1));
			RE::TESForm* form(ManagedList::TransferList().ByIndex(index).first);
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
				m_transferTarget = refr->GetBaseObject()->GetName();
				RE::PlayerCharacter::GetSingleton()->RemoveItem(
					m_item, m_handled, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
				refr->AddObjectToContainer(const_cast<RE::TESBoundObject*>(m_item), nullptr, m_handled, nullptr);
				DBG_VMESSAGE("Moved excess {} of {}/0x{:08x} to Container {} for REFR 0x{:08x}",
					m_handled, m_item->GetName(), m_item->GetFormID(), m_transferTarget, refr->GetFormID());
			}
		}
	}
}

std::string InventoryEntry::Sell(const bool excessOnly)
{
	Populate();
	m_excessHandling = ExcessInventoryHandling::ConvertToSeptims;
	if (!excessOnly)
		m_maxCount = 0;
	HandleExcess();
	std::string sold(DataCase::GetInstance()->GetTranslation("$SHSE_ITEM_SOLD"));
	StringUtils::Replace(sold, "{ITEMNAME}", m_item->GetName());
	StringUtils::Replace(sold, "{ITEMCOUNT}", std::to_string(m_handled));
	StringUtils::Replace(sold, "{ITEMVALUE}", std::to_string(m_saleProceeds));
	return sold;
}
std::string InventoryEntry::Transfer(const bool excessOnly)
{
	if (!UseTransferForExcess(m_excessHandling))
		return "Transfer not set up for type " + (m_crafting ? "crafting" : GetObjectTypeName(m_excessType));
	Populate();
	if (!excessOnly)
		m_maxCount = 0;
	HandleExcess();
	std::string transferred(DataCase::GetInstance()->GetTranslation("$SHSE_ITEM_TRANSFERRED"));
	StringUtils::Replace(transferred, "{ITEMNAME}", m_item->GetName());
	StringUtils::Replace(transferred, "{ITEMCOUNT}", std::to_string(m_handled));
	StringUtils::Replace(transferred, "{ITEMTARGET}", m_transferTarget);
	return transferred;
}
std::string InventoryEntry::Delete(const bool excessOnly)
{
	Populate();
	m_excessHandling = ExcessInventoryHandling::ConvertToSeptims;
	m_salePercent = 0.0;		// sell for 0.0 is effectively a deletion
	if (!excessOnly)
		m_maxCount = 0;
	HandleExcess();
	std::string deleted(DataCase::GetInstance()->GetTranslation("$SHSE_ITEM_DELETED"));
	StringUtils::Replace(deleted, "{ITEMNAME}", m_item->GetName());
	StringUtils::Replace(deleted, "{ITEMCOUNT}", std::to_string(m_handled));
	return deleted;
}
std::string InventoryEntry::Disposition()
{
	Populate();
	std::ostringstream oss;
	oss << m_item->GetName() << " (" << (m_crafting ? "crafting" : "non-crafting") << '/' << GetObjectTypeName(m_excessType);
	oss << ") " << ExcessInventoryHandlingString(m_excessHandling) << '\n';
	if (m_exemption != ExcessInventoryExemption::NotExempt)
	{
		oss << " Exempt: " << ExcessInventoryExemptionString(m_exemption) << '\n';
	}
	if (m_excessHandling != ExcessInventoryHandling::NoLimits)
	{
		oss << "Max: " << m_maxItems << ',';
		if (m_maxItemsByWeight > 0)
		{
			oss << " by weight: " << m_maxItemsByWeight;
		}
		else
		{
			oss << " ignore weight";
		}
		if (m_excessHandling == ExcessInventoryHandling::ConvertToSeptims)
			oss << ", sell as " << static_cast<size_t>(m_salePercent * 100.0) << "% of value";
		else if (m_excessHandling == ExcessInventoryHandling::LeaveBehind)
			oss << ", leave behind";
	}
	std::string result(oss.str());
	REL_MESSAGE("Excess Inventory Handling for {}/0x{:08x}:\n{}", m_item->GetName(), m_item->GetFormID(), result);
	return result;
}

}