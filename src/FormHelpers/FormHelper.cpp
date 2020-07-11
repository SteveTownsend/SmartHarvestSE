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

#include "Data/DataCase.h"
#include "FormHelpers/AlchemyItemHelper.h"
#include "FormHelpers/FormHelper.h"
#include "FormHelpers/ArmorHelper.h"
#include "FormHelpers/WeaponHelper.h"
#include "Collections/Collection.h"
#include "Collections/CollectionManager.h"
#include "Looting/objects.h"

namespace shse
{

TESFormHelper::TESFormHelper(const RE::TESForm* form, const INIFile::SecondaryType scope) : m_form(form), m_matcher(form, scope)
{
	// If this is a leveled item, try to redirect to its contents
	m_form = DataCase::GetInstance()->ConvertIfLeveledItem(m_form);
	m_objectType = GetBaseFormObjectType(form);
	m_typeName = GetObjectTypeName(m_objectType);
}

RE::BGSKeywordForm* TESFormHelper::GetKeywordForm() const
{
	return dynamic_cast<RE::BGSKeywordForm*>(const_cast<RE::TESForm*>(m_form));
}

RE::EnchantmentItem* TESFormHelper::GetEnchantment()
{
	if (!m_form)
		return false;

	if (m_form->formType == RE::FormType::Weapon || m_form->formType == RE::FormType::Armor)
	{
		const RE::TESEnchantableForm* enchanted(m_form->As<RE::TESEnchantableForm>());
		if (enchanted)
   		    return enchanted->formEnchanting;
	}
	return false;
}

SInt32 TESFormHelper::GetGoldValue() const
{
	if (!m_form)
		return 0;

	switch (m_form->formType)
	{
	case RE::FormType::Armor:
	case RE::FormType::Weapon:
	case RE::FormType::Enchantment:
	case RE::FormType::Spell:
	case RE::FormType::Scroll:
	case RE::FormType::Ingredient:
	case RE::FormType::AlchemyItem:
	case RE::FormType::Misc:
	case RE::FormType::Apparatus:
	case RE::FormType::KeyMaster:
	case RE::FormType::SoulGem:
	case RE::FormType::Ammo:
	case RE::FormType::Book:
		break;
	default:
		return 0;
	}

	const RE::TESValueForm* pValue(m_form->As<RE::TESValueForm>());
	if (!pValue)
		return 0;

	return pValue->value;
}

std::pair<bool, SpecialObjectHandling> TESFormHelper::TreatAsCollectible(void) const
{
	// ignore whitelist - we need the underlying object type
	return shse::CollectionManager::Instance().TreatAsCollectible(m_matcher);
}

double TESFormHelper::GetWeight() const
{
	if (!m_form)
		return 0.0;

	const RE::TESWeightForm* pWeight(m_form->As<RE::TESWeightForm>());
	if (!pWeight)
		return 0.0;

	return pWeight->weight;
}

SInt32 TESFormHelper::CalculateWorth() const
{
	if (!m_form)
		return 0;

	if (m_form->formType == RE::FormType::Ammo)
	{
		const RE::TESAmmo* ammo(m_form->As<RE::TESAmmo>());

		DBG_VMESSAGE("Ammo %0.2f", ammo->data.damage);
		return ammo ? static_cast<int>(ammo->data.damage) : 0;
	}
	else if (m_form->formType == RE::FormType::Projectile)
	{
		const RE::BGSProjectile* proj(m_form->As<RE::BGSProjectile>());
		if (proj)
		{
			const RE::TESAmmo* ammo(DataCase::GetInstance()->ProjToAmmo(proj));
			if (ammo)
			{
				DBG_VMESSAGE("Projectile %0.2f", ammo->data.damage);
			}
			return ammo ? static_cast<int>(ammo->data.damage) : 0;
		}
	}
	else
	{
		SInt32 result(0);
		if (m_form->formType == RE::FormType::Weapon)
		{
			result = TESObjectWEAPHelper(m_form->As<RE::TESObjectWEAP>()).GetGoldValue();
		}
		else if (m_form->formType == RE::FormType::Armor)
		{
			result = TESObjectARMOHelper(m_form->As<RE::TESObjectARMO>()).GetGoldValue();
		}
		else if (m_form->formType == RE::FormType::Enchantment ||
			m_form->formType == RE::FormType::Spell ||
			m_form->formType == RE::FormType::Scroll ||
			m_form->formType == RE::FormType::Ingredient ||
			m_form->formType == RE::FormType::AlchemyItem)
		{
			result = AlchemyItemHelper(m_form->As<RE::AlchemyItem>()).GetGoldValue();
		}
		return result == 0 ? GetGoldValue() : result;
	}
	return 0;
}

const char* TESFormHelper::GetName() const
{
	return m_form->GetName();
}

UInt32 TESFormHelper::GetFormID() const
{
	return m_form->formID;
}

bool IsPlayable(const RE::TESForm* pForm)
{
	if (!pForm)
		return false;
	return pForm->GetPlayable();
}

}