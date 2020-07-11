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
#include "FormHelpers/ArmorHelper.h"

namespace shse
{

UInt32 TESObjectARMOHelper::GetGoldValue() const
{
	if (!m_armor)
		return 0;

	RE::EnchantmentItem* ench = TESFormHelper(m_armor, INIFile::SecondaryType::itemObjects).GetEnchantment();
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

}