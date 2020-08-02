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
#include "Utilities/utils.h"

namespace shse
{

uint32_t TESObjectWEAPHelper::GetGoldValue(void) const
{
	if (!m_weapon)
		return 0;

	static RE::BSFixedString fEnchantmentPointsMult = "fEnchantmentPointsMult";
	static const double fEPM = utils::GetGameSettingFloat(fEnchantmentPointsMult);
	static RE::BSFixedString fEnchantmentEffectPointsMult = "fEnchantmentEffectPointsMult";
	static const double fEEPM = utils::GetGameSettingFloat(fEnchantmentEffectPointsMult);

	RE::EnchantmentItem* ench = TESFormHelper(m_weapon, INIFile::SecondaryType::itemObjects).GetEnchantment();
	if (!ench)
		return m_weapon->value;

	int16_t charge = this->GetMaxCharge();
	uint32_t cost = static_cast<uint32_t>(ench->data.costOverride);
	return static_cast<uint32_t>((m_weapon->value * 2) + (fEPM * charge) + (fEEPM * cost));
}

int16_t TESObjectWEAPHelper::GetMaxCharge() const
{
	if (!m_weapon)
		return 0;
	RE::EnchantmentItem* ench = TESFormHelper(m_weapon, INIFile::SecondaryType::itemObjects).GetEnchantment();
	if (ench)
        return static_cast<int16_t>(m_weapon->amountofEnchantment);
	return 0;
}

}