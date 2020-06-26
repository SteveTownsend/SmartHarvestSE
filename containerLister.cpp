#include "PrecompiledHeaders.h"

#include "ExtraDataListHelper.h"
#include "containerLister.h"

ContainerLister::ContainerLister(INIFile::SecondaryType targetType, const RE::TESObjectREFR* refr, bool requireQuestItemAsTarget) :
	m_targetType(targetType), m_refr(refr), m_requireQuestItemAsTarget(requireQuestItemAsTarget),
	m_hasQuestItem(false), m_hasEnchantedItem(false), m_hasValuableItem(false), m_hasCollectibleItem(false)
{
}

bool ContainerLister::HasAllTypes() const
{
	return m_hasQuestItem && m_hasEnchantedItem && m_hasValuableItem && m_hasCollectibleItem;
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

		lootableItems.emplace_back(m_targetType, std::move(entry), count);
	}

	if (lootableItems.empty())
		return lootableItems;

	const RE::ExtraContainerChanges* exChanges = m_refr->extraList.GetByType<RE::ExtraContainerChanges>();
	if (exChanges && exChanges->changes && exChanges->changes->entryList)
	{
		for (auto entryData = exChanges->changes->entryList->begin();
			entryData != exChanges->changes->entryList->end() && !HasAllTypes(); ++entryData)
		{
			RE::TESBoundObject* item = (*entryData)->object;
			if (!IsPlayable(item))
				continue;

			RE::TESFullName* fullName = item->As<RE::TESFullName>();
			if (!fullName || fullName->GetFullNameLength() == 0)
				continue;

			if ((*entryData)->countDelta <= 0)
				continue;
			if (!(*entryData)->extraLists)
				continue;

			// Check for enchantment or quest target
			for (auto extraList = (*entryData)->extraLists->begin(); extraList != (*entryData)->extraLists->end() && !HasAllTypes(); ++extraList)
			{
				if (*extraList)
				{
					ExtraDataListHelper exListHelper(*extraList);
					if (!m_hasQuestItem)
						m_hasQuestItem = exListHelper.IsQuestObject(m_requireQuestItemAsTarget);

					if (!m_hasEnchantedItem)
						m_hasEnchantedItem = exListHelper.GetEnchantment() != nullptr;

					TESFormHelper itemEx(item);
					if (!m_hasEnchantedItem)
					{
						m_hasEnchantedItem = itemEx.GetEnchantment() != nullptr;
					}
					if (!m_hasValuableItem)
					{
						m_hasValuableItem = itemEx.IsValuable();
					}
					if (!m_hasCollectibleItem)
					{
						m_hasCollectibleItem = itemEx.IsCollectible().first;
					}
				}
			}
		}
	}
	return lootableItems;
}
