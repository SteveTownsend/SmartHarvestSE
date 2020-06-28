#include "PrecompiledHeaders.h"

#include "FormHelpers/FormHelper.h"
#include "FormHelpers/ArmorHelper.h"

UInt32 TESObjectARMOHelper::GetGoldValue() const
{
	if (!m_armor)
		return 0;

	RE::EnchantmentItem* ench = TESFormHelper(m_armor).GetEnchantment();
	if (!ench)
	{
		return m_armor->value;
	}

	double costPP = 0.0;
	for (RE::Effect* effect : ench->effects)
	{
		if (!effect)
			continue;

		costPP += effect->cost;
	}

	UInt32 result = (costPP > 0) ? static_cast<UInt32>(costPP) : 0;
	DBG_VMESSAGE("TESObjectARMOHelper::GetGoldValue()  %d  %d", m_armor->value, result);

	return m_armor->value + result;
}
