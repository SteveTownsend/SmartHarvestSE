#include "PrecompiledHeaders.h"

#include "objects.h"
#include "TESFormHelper.h"
#include "ExtraDataListHelper.h"
#include "containerLister.h"

bool ContainerLister::GetOrCheckContainerForms(LootableItems& lootableItems, bool &hasQuestObject, bool &hasEnchItem)
{
	if (!m_refr)
		return false;

	bool hasExtraItems(false);
	const RE::TESContainer* container = const_cast<RE::TESObjectREFR*>(m_refr)->GetContainer();
	if (container)
	{
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

		const RE::ExtraContainerChanges* exChanges = m_refr->extraList.GetByType<RE::ExtraContainerChanges>();
		if (exChanges && exChanges->changes && exChanges->changes->entryList)
		{
			for (auto entryData = exChanges->changes->entryList->begin();
				entryData != exChanges->changes->entryList->end() && (!hasQuestObject || !hasEnchItem); ++entryData)
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

				// Check for exchantment or quest target
				hasExtraItems = true;
				for (auto extraList = (*entryData)->extraLists->begin(); extraList != (*entryData)->extraLists->end(); ++extraList)
				{
					if (*extraList)
					{
						ExtraDataListHelper exListHelper(*extraList);
						if (!hasQuestObject)
							hasQuestObject = exListHelper.IsQuestObject(m_requireQuestItemAsTarget);

						if (!hasEnchItem)
							hasEnchItem = exListHelper.GetEnchantment() != nullptr;

						if (!hasEnchItem)
						{
							TESFormHelper itemEx(item);
							hasEnchItem = (itemEx.GetEnchantment()) ? true : false;;
						}
					}
					if (hasEnchItem && hasQuestObject)
						break;
				}
			}
		}
	}
	return !lootableItems.empty() || hasExtraItems;
}
