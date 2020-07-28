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
#include "FormHelpers/ExtraDataListHelper.h"
#include "Looting/containerLister.h"

namespace shse
{

ContainerLister::ContainerLister(const INIFile::SecondaryType targetType, const RE::TESObjectREFR* refr,
	const bool requireQuestItemAsTarget, const bool checkSpecials) :
	m_targetType(targetType), m_refr(refr), m_requireQuestItemAsTarget(requireQuestItemAsTarget),
	m_hasQuestItem(false), m_hasEnchantedItem(false), m_hasValuableItem(false),
	m_hasCollectibleItem(false), m_collectibleAction(CollectibleHandling::Leave), m_checkSpecials(checkSpecials)
{
}

LootableItems ContainerLister::GetOrCheckContainerForms()
{
	LootableItems lootableItems;
	if (!m_refr)
		return lootableItems;

	const RE::TESContainer* container = const_cast<RE::TESObjectREFR*>(m_refr)->GetContainer();
	if (!container)
		return lootableItems;

	// refactored following QuickLookRE
	auto inv = const_cast<RE::TESObjectREFR*>(m_refr)->GetInventory();
	for (auto& item : inv) {
		auto& [count, entry] = item.second;
		if (count <= 0)
			continue;
		RE::TESBoundObject* item = entry->GetObject();
		if (item->formType == RE::FormType::LeveledItem)
			continue;

		if (!item->GetPlayable())
			continue;

		RE::TESFullName* fullName = item->As<RE::TESFullName>();
		if (!fullName || fullName->GetFullNameLength() == 0)
			continue;

		lootableItems.emplace_back(std::move(entry), count);
	}

	if (lootableItems.empty() || !m_checkSpecials)
		return lootableItems;

	const RE::ExtraContainerChanges* exChanges = m_refr->extraList.GetByType<RE::ExtraContainerChanges>();
	if (exChanges && exChanges->changes && exChanges->changes->entryList)
	{
		for (auto entryData = exChanges->changes->entryList->begin();
			entryData != exChanges->changes->entryList->end(); ++entryData)
		{
			RE::TESBoundObject* item = (*entryData)->object;
			if (!item || !item->GetPlayable())
				continue;

			RE::TESFullName* fullName = item->As<RE::TESFullName>();
			if (!fullName || fullName->GetFullNameLength() == 0)
				continue;

			if ((*entryData)->countDelta <= 0)
				continue;
			if (!(*entryData)->extraLists)
				continue;

			// Check for enchantment or quest target
			for (auto extraList = (*entryData)->extraLists->begin(); extraList != (*entryData)->extraLists->end(); ++extraList)
			{
				if (*extraList)
				{
					ExtraDataListHelper exListHelper(*extraList);
					if (!m_hasQuestItem)
						m_hasQuestItem = exListHelper.IsQuestObject(m_requireQuestItemAsTarget);

					if (!m_hasEnchantedItem)
						m_hasEnchantedItem = exListHelper.GetEnchantment() != nullptr;

					TESFormHelper itemEx(item, m_targetType);
					if (!m_hasEnchantedItem)
					{
						m_hasEnchantedItem = itemEx.GetEnchantment() != nullptr;
					}
					if (!m_hasValuableItem)
					{
						m_hasValuableItem = itemEx.IsValuable();
					}
					const auto collectible(itemEx.TreatAsCollectible());
					if (collectible.first)
					{
						// use the most permissive action
						m_hasCollectibleItem = true;
						m_collectibleAction = UpdateCollectibleHandling(m_collectibleAction, collectible.second);
					}
				}
			}
		}
	}
	return lootableItems;
}

}