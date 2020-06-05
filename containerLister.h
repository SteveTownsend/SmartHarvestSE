#pragma once

#include "InventoryItem.h"

typedef std::vector<InventoryItem> LootableItems;

struct ContainerLister
{
public:
	ContainerLister(INIFile::SecondaryType targetType, const RE::TESObjectREFR* refr, bool requireQuestItemAsTarget) :
		m_targetType(targetType), m_refr(refr), m_requireQuestItemAsTarget(requireQuestItemAsTarget) {};
	LootableItems GetOrCheckContainerForms(bool& hasQuestObject, bool& hasEnchItem);
private:
	const RE::TESObjectREFR* m_refr;
	INIFile::SecondaryType m_targetType;
	bool m_requireQuestItemAsTarget;
};

