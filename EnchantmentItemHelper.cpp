#include "PrecompiledHeaders.h"

#include "EnchantmentItemHelper.h"

UInt32 EnchantmentItemHelper::GetGoldValue()
{
	if (!this)
		return 0;

	RE::TESValueForm* pValue = m_item->As<RE::TESValueForm>();
	if (!pValue)
		return 0;

#if _DEBUG
	_MESSAGE("EnchantmentItemHelper::GetGoldValue()  %d", pValue->value);
#endif

	return pValue->value;
}
