#include "PrecompiledHeaders.h"
#include "ArmorHelper.h"

UInt32 TESObjectARMOHelper::GetGoldValue() const
{
	if (!m_armor)
		return 0;

	const RE::TESValueForm* pValue = m_armor->As<RE::TESValueForm>();
	if (!pValue)
		return 0;

	RE::EnchantmentItem* ench = TESFormHelper(m_armor).GetEnchantment();
	if (!ench)
	{
		return pValue->value;
	}

	double costPP = 0.0;
	for (RE::Effect* effect : ench->effects)
	{
		if (!effect)
			continue;

		costPP += effect->cost;
	}

	UInt32 result = (costPP > 0) ? static_cast<UInt32>(costPP) : 0;
	DBG_VMESSAGE("TESObjectARMOHelper::GetGoldValue()  %d  %d", pValue->value, result);

	return pValue->value + result;
}
