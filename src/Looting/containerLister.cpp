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

#include "FormHelpers/FormHelper.h"
#include "Data/dataCase.h"
#include "FormHelpers/ExtraDataListHelper.h"
#include "Utilities/utils.h"
#include "Looting/containerLister.h"
#include "Looting/ManagedLists.h"
#include "Looting/objects.h"
#include "WorldState/QuestTargets.h"
#include "Collections/CollectionManager.h"

namespace shse
{

ContainerLister::ContainerLister(const INIFile::SecondaryType targetType, const RE::TESObjectREFR* refr) :
	m_refr(refr), m_targetType(targetType), m_collectibleAction(CollectibleHandling::Leave)
{
	m_enchantedLoot = SettingsCache::Instance().EnchantedObjectHandlingType();
}

void ContainerLister::FilterLootableItems(std::function<bool(RE::TESBoundObject*)> predicate)
{
	// refactored following QuickLookRE
	auto inv = const_cast<RE::TESObjectREFR*>(m_refr)->GetInventory();
	for (auto& item : inv) {
		auto& [count, entry] = item.second;
		if (count <= 0)
			continue;
		RE::TESBoundObject* itemObject = entry->GetObject();
		if (!FormUtils::IsConcrete(itemObject))
			continue;

		if (itemObject->formType == RE::FormType::LeveledItem)
			continue;

		if (predicate(itemObject))
		{
			DBG_DMESSAGE("Matched filter predicate for {}/0x{:08x}, count={}", itemObject->GetName(), itemObject->GetFormID(), count);
			m_lootableItems.emplace_back(std::move(entry), count, m_enchantedLoot);
		}
	}
}

size_t ContainerLister::CountLootableItems(std::function<bool(RE::TESBoundObject*)> predicate)
{
	// refactored following QuickLookRE
	size_t items(0);
	auto inv = const_cast<RE::TESObjectREFR*>(m_refr)->GetInventory();
	for (auto& item : inv) {
		auto& [count, entry] = item.second;
		if (count <= 0)
			continue;
		RE::TESBoundObject* itemObject = entry->GetObject();
		if (!FormUtils::IsConcrete(itemObject))
			continue;

		if (itemObject->formType == RE::FormType::LeveledItem)
			continue;

		if (predicate(itemObject))
		{
			DBG_DMESSAGE("Matched count predicate for {}/0x{:08x}, count={}", itemObject->GetName(), itemObject->GetFormID(), count);
			++items;
		}
	}
	return items;
}

InventoryCache ContainerLister::CacheIfExcessHandlingEnabled(const bool force, const InventoryUpdates& updates) const
{
	// refactored following QuickLookRE
	// return value contains all eligible Inventory items for full scan, or current Inventory item data for deltas
	InventoryCache cache;
	auto inv = const_cast<RE::TESObjectREFR*>(m_refr)->GetInventory();
	for (auto& item : inv) {
		auto& [count, entry] = item.second;
		RE::TESBoundObject* itemObject = entry->GetObject();

		// very quick no-brainer tests first
		if (count <= 0)
			continue;
		if (!FormUtils::IsConcrete(itemObject))
			continue;

		if (itemObject->formType == RE::FormType::LeveledItem)
			continue;

		// register any new user-created Ingestible on full scan as early as possible
		if (itemObject->IsDynamicForm())
		{
			if (itemObject->GetFormType() == RE::FormType::AlchemyItem)
			{
				// User created potion or poison
				DataCase::GetInstance()->RegisterPlayerCreatedALCH(itemObject->As<RE::AlchemyItem>());
			}
		}

		// if this is a delta review, ignore untouched items
		if (!force && !updates.contains(itemObject))
		{
			DBG_DMESSAGE("Item {}/0x{:08x} not touched since last review", itemObject->GetName(), itemObject->GetFormID());
			continue;
		}

		// do not bother with other checks if excess-handling is not in effect for the item
		InventoryEntry itemEntry(itemObject, count);
		if (itemEntry.HandlingType() == ExcessInventoryHandling::NoLimits)
			continue;

		// Do not auto-sell or otherwise futz with this if it even MIGHT be a Quest Target
		if (!QuestTargets::Instance().AllowsExcessHandling(itemObject))
		{
			DBG_DMESSAGE("Quest Item {}/0x{:08x}, exempt from Excess Inventory", itemObject->GetName(), itemObject->GetFormID());
			continue;
		}
		// Check if item is sticky in inventory, requiring deliberate disposal by Player
		// Ex. AddItemMenuSE kit
		const auto isItemSticky(CollectionManager::ExcessInventory().TreatAsCollectible(ConditionMatcher(itemObject)));
		if (isItemSticky.first)
		{
			DBG_DMESSAGE("Sticky Item {}/0x{:08x}, exempt from Excess Inventory", itemObject->GetName(), itemObject->GetFormID());
			continue;
		}

		// exempt Equipped and Worn items
		if (ManagedList::EquippedOrWorn().Contains(itemObject))
		{
			DBG_DMESSAGE("Equipped/Worn Item {}/0x{:08x}, exempt from Excess Inventory", itemObject->GetName(), itemObject->GetFormID());
			continue;
		}

		//  exempt Favorite items
		if (entry->extraLists)
		{
			bool isFavourite(false);
			bool isEnchanted(false);
			bool isTempered(false);
			for (RE::ExtraDataList* extraData : *(entry->extraLists))
			{
				if (!extraData)
					continue;

				if (extraData->HasType(RE::ExtraDataType::kHotkey))
				{
					isFavourite = true;
					break;
				}
				if (ExtraDataList::GetEnchantment(extraData))
				{
					isEnchanted = true;
					break;
				}
				if (extraData->HasType(RE::ExtraDataType::kTextDisplayData))
				{
					isTempered = true;
					break;
				}
			}
			if (isFavourite)
			{
				DBG_DMESSAGE("Favourite Item {}/0x{:08x}, exempt from Excess Inventory", itemObject->GetName(), itemObject->GetFormID());
				continue;
			}
			if (isEnchanted)
			{
				DBG_DMESSAGE("Player-enchanted Item {}/0x{:08x}, exempt from Excess Inventory", itemObject->GetName(), itemObject->GetFormID());
				continue;
			}
			if (isTempered)
			{
				DBG_DMESSAGE("Tempered Item {}/0x{:08x}, exempt from Excess Inventory", itemObject->GetName(), itemObject->GetFormID());
				continue;
			}
		}

		// Transfer or sell items in excess of limits, record as eligible
		itemEntry.Populate();
		itemEntry.HandleExcess();
		cache.insert({ itemObject, itemEntry });
	}
	return cache;
}

std::string ContainerLister::SellItem(RE::TESBoundObject* target, const bool excessOnly)
{
	InventoryEntry entry(GetSingleInventoryEntry(target));
	if (entry.Count() <= 0)
	{
		std::string err(DataCase::GetInstance()->GetTranslation("$SHSE_ITEM_CANNOT_SELL"));
		StringUtils::Replace(err, "{ITEMNAME}", target->GetName());
		return err;
	}
	return entry.Sell(excessOnly);
}
std::string ContainerLister::TransferItem(RE::TESBoundObject* target, const bool excessOnly)
{
	InventoryEntry entry(GetSingleInventoryEntry(target));
	if (entry.Count() <= 0)
	{
		std::string err(DataCase::GetInstance()->GetTranslation("$SHSE_ITEM_CANNOT_TRANSFER"));
		StringUtils::Replace(err, "{ITEMNAME}", target->GetName());
		return err;
	}
	return entry.Transfer(excessOnly);
}
std::string ContainerLister::DeleteItem(RE::TESBoundObject* target, const bool excessOnly)
{
	InventoryEntry entry(GetSingleInventoryEntry(target));
	if (entry.Count() <= 0)
	{
		std::string err(DataCase::GetInstance()->GetTranslation("$SHSE_ITEM_CANNOT_DELETE"));
		StringUtils::Replace(err, "{ITEMNAME}", target->GetName());
		return err;
	}
	return entry.Delete(excessOnly);
}

std::string ContainerLister::CheckItemAsExcess(RE::TESBoundObject* target)
{
	constexpr bool isDestructive(false);
	InventoryEntry entry(GetSingleInventoryEntry(target, isDestructive));
	return entry.Disposition();
}

InventoryEntry ContainerLister::GetSingleInventoryEntry(RE::TESBoundObject* target) const
{
	constexpr bool isDestructive(true);
	return GetSingleInventoryEntry(target, isDestructive);
}

InventoryEntry ContainerLister::GetSingleInventoryEntry(RE::TESBoundObject* target, const bool isDestructive) const
{
	// refactored following QuickLookRE
	auto inv = const_cast<RE::TESObjectREFR*>(m_refr)->GetInventory();
	for (auto& item : inv) {
		auto& [count, entry] = item.second;
		RE::TESBoundObject* itemObject = entry->GetObject();
		if (target != itemObject)
			continue;
		if (count <= 0)
		{
			return InventoryEntry(target, ExcessInventoryExemption::CountIsZero);
		}
		if (!FormUtils::IsConcrete(itemObject))
		{
			return InventoryEntry(target, ExcessInventoryExemption::Ineligible);
		}
		if (itemObject->formType == RE::FormType::LeveledItem)
		{
			return InventoryEntry(target, ExcessInventoryExemption::IsLeveledItem);
		}
		// Mostly skip check for sticky items here: if player wants to get rid of such items like this, that's OK
		// We do make the check if user is introspecting the item (non-destructive)
		if (!isDestructive)
		{
			const auto isItemSticky(CollectionManager::ExcessInventory().TreatAsCollectible(ConditionMatcher(itemObject)));
			if (isItemSticky.first)
			{
				DBG_DMESSAGE("Item {}/0x{:08x} is anchored in inventory", itemObject->GetName(), itemObject->GetFormID());
				return InventoryEntry(target, ExcessInventoryExemption::Anchored);
			}
		}
		// Do not auto-sell or otherwise futz with this if it even MIGHT be a Quest Target
		if (!QuestTargets::Instance().AllowsExcessHandling(itemObject))
		{
			return InventoryEntry(target, ExcessInventoryExemption::QuestItem);
		}

		// exempt Equipped and Worn items
		if (ManagedList::EquippedOrWorn().Contains(itemObject))
		{
			return InventoryEntry(target, ExcessInventoryExemption::ItemInUse);
		}

		//  exempt Favourite, Tempered and Enchanted Items
		bool isFavourite(false);
		bool isEnchanted(false);
		bool isTempered(false);
		if (entry->extraLists)
		{
			for (RE::ExtraDataList* extraData : *(entry->extraLists))
			{
				if (!extraData)
					continue;
				if (extraData->HasType(RE::ExtraDataType::kHotkey))
					isFavourite = true;
				if (ExtraDataList::GetEnchantment(extraData))
					isEnchanted = true;
				if (extraData->HasType(RE::ExtraDataType::kTextDisplayData))
					isTempered = true;
			}
		}
		if (isFavourite)
			return InventoryEntry(target, ExcessInventoryExemption::IsFavourite);
		if (isTempered)
			return InventoryEntry(target, ExcessInventoryExemption::IsTempered);
		if (isEnchanted)
			return InventoryEntry(target, ExcessInventoryExemption::IsPlayerEnchanted);

		return InventoryEntry(itemObject, count);
	}
	return InventoryEntry(target, ExcessInventoryExemption::NotFound);
}

size_t ContainerLister::AnalyzeLootableItems(const EnchantedObjectHandling enchantedObjectHandling)
{
	if (!m_refr)
		return 0;

	const RE::TESContainer* container = const_cast<RE::TESObjectREFR*>(m_refr)->GetContainer();
	if (!container)
		return 0;

	FilterLootableItems([=](RE::TESBoundObject*) -> bool { return true; });
	if (m_lootableItems.empty())
		return m_lootableItems.size();

	const RE::ExtraContainerChanges* exChanges = m_refr->extraList.GetByType<RE::ExtraContainerChanges>();
	if (exChanges && exChanges->changes && exChanges->changes->entryList)
	{
		for (auto entryData = exChanges->changes->entryList->begin();
			entryData != exChanges->changes->entryList->end(); ++entryData)
		{
			RE::TESBoundObject* item = (*entryData)->object;
			if (!FormUtils::IsConcrete(item))
				continue;
			if ((*entryData)->countDelta <= 0)
				continue;
			if (!(*entryData)->extraLists)
				continue;

			// Check for enchantment or quest target
			if (QuestTargets::Instance().QuestTargetLootability(item, m_refr) == Lootability::CannotLootQuestTarget)
			{
				DBG_DMESSAGE("Quest Item={}/0x{:08x}", item->GetName(), item->GetFormID());
				m_questItems.insert(item);
			}

			for (auto extraList = (*entryData)->extraLists->begin(); extraList != (*entryData)->extraLists->end(); ++extraList)
			{
				if (*extraList)
				{
					if (ExtraDataList::IsItemQuestObject(item, *extraList))
					{
						DBG_DMESSAGE("Quest Item {}/0x{:08x}", item->GetName(), item->GetFormID());
						m_questItems.insert(item);
					}
					TESFormHelper itemEx(item, m_targetType);
					RE::EnchantmentItem* enchantItem(nullptr);
					if ((enchantItem = ExtraDataList::GetEnchantment(*extraList)) != nullptr)
					{
						if (TESFormHelper::ConfirmEnchanted(enchantItem, enchantedObjectHandling))
						{
							DBG_DMESSAGE("Enchanted Item {}/0x{:08x}", item->GetName(), item->GetFormID());
							m_enchantedItems.insert(item);
						}
					}
					else if ((enchantItem = itemEx.GetEnchantment()) != nullptr)
					{
						if (TESFormHelper::ConfirmEnchanted(enchantItem, enchantedObjectHandling))
						{
							DBG_DMESSAGE("Enchanted Item={}/0x{:08x}", item->GetName(), item->GetFormID());
							m_enchantedItems.insert(item);
						}
					}
					if (itemEx.IsValuable())
					{
						DBG_DMESSAGE("Valuable Item {}/0x{:08x}", item->GetName(), item->GetFormID());
						m_valuableItems.insert(item);
					}
					// Collectible item precheck - do not record as a possible dup, as we check them all again before looting
					static const bool recordDups(false);
					const auto collectible(itemEx.TreatAsCollectible(recordDups));
					if (collectible.first)
					{
						DBG_DMESSAGE("Collectible Item {}/0x{:08x}", item->GetName(), item->GetFormID());
						// use the most permissive action
						m_collectibleItems.insert(item);
						m_collectibleAction = UpdateCollectibleHandling(m_collectibleAction, collectible.second);
					}
				}
			}
		}
	}
	return m_lootableItems.size();
}

void ContainerLister::RemoveUnlootable(const std::unordered_set<RE::TESBoundObject*>& filter)
{
	decltype(m_lootableItems) filteredList;
	filteredList.reserve(m_lootableItems.size());
	std::copy_if(m_lootableItems.cbegin(), m_lootableItems.cend(), std::back_inserter(filteredList), [&](const InventoryItem& item) -> bool
	{
		return !filter.contains(item.BoundObject());
	});
	DBG_DMESSAGE("Lootable Item count {} filtered from {}", filteredList.size(), m_lootableItems.size());
	m_lootableItems.swap(filteredList);
}

}