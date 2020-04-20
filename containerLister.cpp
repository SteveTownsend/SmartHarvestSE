#include "PrecompiledHeaders.h"

#include "objects.h"
#include "TESFormHelper.h"
#include "ExtraDataListHelper.h"
#include "containerLister.h"

bool ContainerLister::GetOrCheckContainerForms(std::unordered_map<RE::TESForm*, int>& lootableItems, bool &hasQuestObject, bool &hasEnchItem)
{
	if (!m_refr)
		return false;

	const RE::TESContainer *container = const_cast<RE::TESObjectREFR*>(m_refr)->GetContainer();
	if (container)
	{
		container->ForEachContainerObject([&](RE::ContainerObject* entry) -> bool {
			RE::TESBoundObject* item = entry->obj;
			if (item->formType == RE::FormType::LeveledItem)
				return false;

			if (entry->count <= 0)
				return false;

			if (!item->GetPlayable())
				return false;

			RE::TESFullName* fullName = item->As<RE::TESFullName>();
			if (!fullName || fullName->GetFullNameLength() == 0)
				return false;

			lootableItems[item] = entry->count;
			return true;
		});
	}

	const RE::ExtraContainerChanges* exChanges = m_refr->extraList.GetByType<RE::ExtraContainerChanges>();
	if (exChanges && exChanges->changes && exChanges->changes->entryList)
	{
		//for (const RE::InventoryEntryData* entryData : *exChanges->changes->entryList)
		for (auto entryData = exChanges->changes->entryList->begin(); entryData != exChanges->changes->entryList->end(); ++entryData)
			{
			RE::TESBoundObject* item = (*entryData)->object;
			if (!IsPlayable(item))
				continue;

			RE::TESFullName *fullName = item->As<RE::TESFullName>();
			if (!fullName || fullName->GetFullNameLength() == 0)
				continue;

			int total((*entryData)->countDelta);
			if (total > 0)
			{
				const auto matched(lootableItems.find(item));
				if (matched != lootableItems.cend())
				{
					total += matched->second;
				}
				lootableItems[item] = total;
			}

			if (!(*entryData)->extraLists)
				continue;
			if (hasEnchItem && hasQuestObject)
				continue;

			// Check for exchantment or quest target
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
	return !lootableItems.empty();
}
