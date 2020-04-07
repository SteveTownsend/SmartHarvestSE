#pragma once

#include "CommonLibSSE/include/RE/EnchantmentItem.h"
#include "CommonLibSSE/include/RE/ExtraDataList.h"
#include "CommonLibSSE/include/RE/TESKey.h"

class ExtraDataListHelper
{
public:
	ExtraDataListHelper(RE::ExtraDataList* extraData) : m_extraData(extraData) {}
	RE::EnchantmentItem* GetEnchantment(void);
	bool IsQuestObject(const bool requireFullQuestFlags);

	RE::ExtraDataList* m_extraData;
};
