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
#include "WorldState/QuestTargets.h"

namespace shse
{

ContainerLister::ContainerLister(const INIFile::SecondaryType targetType, const RE::TESObjectREFR* refr) :
	m_refr(refr),	m_targetType(targetType), m_collectibleAction(CollectibleHandling::Leave)
{
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
			m_lootableItems.emplace_back(std::move(entry), count);
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

size_t ContainerLister::AnalyzeLootableItems()
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
					if (ExtraDataList::GetEnchantment(*extraList) != nullptr)
					{
						DBG_DMESSAGE("Enchanted Item {}/0x{:08x}", item->GetName(), item->GetFormID());
						m_enchantedItems.insert(item);
					}
					else if (itemEx.GetEnchantment() != nullptr)
					{
						DBG_DMESSAGE("Enchanted Item={}/0x{:08x}", item->GetName(), item->GetFormID());
						m_enchantedItems.insert(item);
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