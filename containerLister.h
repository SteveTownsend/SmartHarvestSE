#pragma once

#include <unordered_map>
#include "InventoryItem.h"

typedef std::vector<InventoryItem> LootableItems;

struct ContainerLister
{
public:
	ContainerLister(const RE::TESObjectREFR* refr, bool requireQuestItemAsTarget) : m_refr(refr), m_requireQuestItemAsTarget(requireQuestItemAsTarget) {};
	bool GetOrCheckContainerForms(LootableItems& lootableItems, bool& hasQuestObject, bool& hasEnchItem);
private:
	const RE::TESObjectREFR* m_refr;
	bool m_requireQuestItemAsTarget;
};

