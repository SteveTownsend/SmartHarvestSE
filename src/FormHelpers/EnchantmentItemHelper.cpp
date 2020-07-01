#include "PrecompiledHeaders.h"

#include "EnchantmentItemHelper.h"

UInt32 EnchantmentItemHelper::GetGoldValue()
{
	if (!this)
		return 0;

	RE::TESValueForm* pValue = m_item->As<RE::TESValueForm>();
	if (!pValue)
		return 0;

	DBG_VMESSAGE("EnchantmentItemHelper::GetGoldValue()  %d", pValue->value);
	return pValue->value;
}
