#pragma once

class ExtraDataListHelper
{
public:
	ExtraDataListHelper(RE::ExtraDataList* extraData) : m_extraData(extraData) {}
	RE::EnchantmentItem* GetEnchantment(void);
	bool IsQuestObject(const bool requireFullQuestFlags);

	RE::ExtraDataList* m_extraData;
};
