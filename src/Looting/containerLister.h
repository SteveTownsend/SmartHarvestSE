#pragma once

#include "InventoryItem.h"

typedef std::vector<InventoryItem> LootableItems;

struct ContainerLister
{
public:
	ContainerLister(INIFile::SecondaryType targetType, const RE::TESObjectREFR* refr, bool requireQuestItemAsTarget);
	LootableItems GetOrCheckContainerForms();
	inline bool HasQuestItem() const { return m_hasQuestItem; }
	inline bool HasEnchantedItem() const { return m_hasEnchantedItem; }
	inline bool HasValuableItem() const { return m_hasValuableItem; }
	inline bool HasCollectibleItem() const { return m_hasCollectibleItem; }
	inline SpecialObjectHandling CollectibleAction() const { return m_collectibleAction; }

private:
	const RE::TESObjectREFR* m_refr;
	INIFile::SecondaryType m_targetType;
	bool m_requireQuestItemAsTarget;
	bool m_hasQuestItem;
	bool m_hasEnchantedItem;
	bool m_hasValuableItem;
	bool m_hasCollectibleItem;
	SpecialObjectHandling m_collectibleAction;
};

