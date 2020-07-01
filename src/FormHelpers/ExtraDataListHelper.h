#pragma once

class ExtraDataListHelper
{
public:
	ExtraDataListHelper(const RE::ExtraDataList* extraData) : m_extraData(extraData) {}
	RE::EnchantmentItem* GetEnchantment(void);
	bool IsQuestObject(const bool requireFullQuestFlags);

	const RE::ExtraDataList* m_extraData;
};
