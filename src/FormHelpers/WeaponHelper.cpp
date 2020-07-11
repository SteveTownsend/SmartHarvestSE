/*************************************************************************
SmartHarvest SE
Copyright (c) Steve Townsend 2020

>>> SOURCE LICENSE >>>
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation (www.fsf.org); either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License is available at
http://www.fsf.org/licensing/licenses
>>> END OF LICENSE >>>
*************************************************************************/
#include "PrecompiledHeaders.h"

#include "FormHelpers/FormHelper.h"
#include "FormHelpers/WeaponHelper.h"

namespace shse
{

UInt32 TESObjectWEAPHelper::GetGoldValue(void) const
{
	if (!m_weapon)
		return 0;

	static RE::BSFixedString fEnchantmentPointsMult = "fEnchantmentPointsMult";
	double fEPM = GetGameSettingFloat(fEnchantmentPointsMult);
	static RE::BSFixedString fEnchantmentEffectPointsMult = "fEnchantmentEffectPointsMult";
	double fEEPM = GetGameSettingFloat(fEnchantmentEffectPointsMult);

	RE::EnchantmentItem* ench = TESFormHelper(m_weapon, INIFile::SecondaryType::itemObjects).GetEnchantment();
	if (!ench)
		return m_weapon->value;

	SInt16 charge = this->GetMaxCharge();
	UInt32 cost = static_cast<UInt32>(ench->data.costOverride);
	return static_cast<UInt32>((m_weapon->value * 2) + (fEPM * charge) + (fEEPM * cost));
}

SInt16 TESObjectWEAPHelper::GetMaxCharge() const
{
	if (!m_weapon)
		return 0;
	RE::EnchantmentItem* ench = TESFormHelper(m_weapon, INIFile::SecondaryType::itemObjects).GetEnchantment();
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

}