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
				return true;

			if (entry->count <= 0)
				return true;

			if (!item->GetPlayable())
				return true;

			RE::TESFullName* fullName = skyrim_cast<RE::TESFullName*, RE::TESForm>(item);
			if (!fullName || fullName->GetFullNameLength() == 0)
				return true;

			lootableItems[item] = entry->count;
			return true;
		});
	}

	const RE::ExtraContainerChanges* exChanges = m_refr->extraList.GetByType<RE::ExtraContainerChanges>();
	if (exChanges && exChanges->changes && exChanges->changes->entryList)
	{
		for (const RE::InventoryEntryData* entryData : *exChanges->changes->entryList)
		{
			RE::TESBoundObject* item = entryData->object;
			if (!IsPlayable(item))
				continue;

			RE::TESFullName *fullName = skyrim_cast<RE::TESFullName*, RE::TESForm>(item);
			if (!fullName || fullName->GetFullNameLength() == 0)
				continue;

			int total(entryData->countDelta);
			if (total > 0)
			{
				const auto matched(lootableItems.find(item));
				if (matched != lootableItems.cend())
				{
					total += matched->second;
				}
				lootableItems[item] = total;
			}

			if (!entryData->extraLists)
				continue;
			if (hasEnchItem && hasQuestObject)
				continue;

			// Check for exchantment or quest target
			for (RE::ExtraDataList* extraList : *entryData->extraLists)
			{
				if (extraList)
				{
					ExtraDataListHelper exListHelper(extraList);
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
