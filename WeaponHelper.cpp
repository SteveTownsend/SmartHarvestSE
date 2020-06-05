#include "PrecompiledHeaders.h"

#include "WeaponHelper.h"

UInt32 TESObjectWEAPHelper::GetGoldValue(void) const
{
	if (!m_weapon)
		return 0;

	static RE::BSFixedString fEnchantmentPointsMult = "fEnchantmentPointsMult";
	double fEPM = GetGameSettingFloat(fEnchantmentPointsMult);
	static RE::BSFixedString fEnchantmentEffectPointsMult = "fEnchantmentEffectPointsMult";
	double fEEPM = GetGameSettingFloat(fEnchantmentEffectPointsMult);

	const RE::TESValueForm* pValue = m_weapon->As<RE::TESValueForm>();
	if (!pValue)
		return 0;

	RE::EnchantmentItem* ench = TESFormHelper(m_weapon).GetEnchantment();
	if (!ench)
		return pValue->value;

	SInt16 charge = this->GetMaxCharge();
	UInt32 cost = static_cast<UInt32>(ench->data.costOverride);
	return static_cast<UInt32>((pValue->value * 2) + (fEPM * charge) + (fEEPM * cost));
}

SInt16 TESObjectWEAPHelper::GetMaxCharge() const
{
	if (!m_weapon)
		return 0;
	RE::EnchantmentItem* ench = TESFormHelper(m_weapon).GetEnchantment();
	if (ench)
        return static_cast<SInt16>(m_weapon->amountofEnchantment);
	return 0;
}

double GetGameSettingFloat(const RE::BSFixedString& name)
{
	RE::Setting* setting(nullptr);
	RE::GameSettingCollection* settings(RE::GameSettingCollection::GetSingleton());
	if (settings)
	{
		setting = settings->GetSetting(name.c_str());
	}

	if (!setting || setting->GetType() != RE::Setting::Type::kFloat)
		return 0.0;

	return setting->GetFloat();
}
