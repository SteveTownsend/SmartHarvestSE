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

TESFormHelper::TESFormHelper(const RE::TESForm* form, const INIFile::SecondaryType scope) : m_form(form), m_matcher(form, scope, GetBaseFormObjectType(form))
{
	init();
}

TESFormHelper::TESFormHelper(const RE::TESForm* form, ObjectType effectiveType, const INIFile::SecondaryType scope) : m_form(form), m_matcher(form, scope, effectiveType)
{
	init();
}

void TESFormHelper::init()
{
	// If this is a leveled item, try to redirect to its contents
	m_form = DataCase::GetInstance()->ConvertIfLeveledItem(m_form);
	m_objectType = m_matcher.GetObjectType();
	m_typeName = GetObjectTypeName(m_objectType);
}

RE::BGSKeywordForm* TESFormHelper::GetKeywordForm() const
{
	return dynamic_cast<RE::BGSKeywordForm*>(const_cast<RE::TESForm*>(m_form));
}

RE::EnchantmentItem* TESFormHelper::GetEnchantment()
{
	if (!m_form)
		return nullptr;

	if (m_form->formType == RE::FormType::Weapon || m_form->formType == RE::FormType::Armor)
	{
		const RE::TESEnchantableForm* enchanted(m_form->As<RE::TESEnchantableForm>());
		if (enchanted)
   		    return enchanted->formEnchanting;
	}
	return nullptr;
}

bool TESFormHelper::ConfirmEnchanted(const RE::EnchantmentItem* item, const EnchantedObjectHandling handling)
{
	if (!item)
		return false;
	// Player may not be interested in known enchantments - treat item with known enchantment as unenchanted
	if (IncludeEnchantedObjectIfKnown(handling))
	{
		DBG_DMESSAGE("Skip Enchantment check for {}/0x{:08x} due to loot handling {}", item->GetName(), item->GetFormID(), handling);
		return true;
	}
	if (item->data.baseEnchantment)
	{
		bool known(item->data.baseEnchantment->GetKnown());
		DBG_DMESSAGE("Base Enchantment {}/0x{:08x} known={} for {}/0x{:08x}", item->data.baseEnchantment->GetName(),
			item->data.baseEnchantment->GetFormID(), known, item->GetName(), item->GetFormID());
		return !known;
	}
	else
	{
		bool known(item->GetKnown());
		DBG_DMESSAGE("Enchantment {}/0x{:08x} known={}", item->GetName(), item->GetFormID(), known);
		return !known;
	}
}

// modified from RE::TESObjectREFR::IsEnchanted
ObjectType TESFormHelper::EnchantedREFREffectiveType(const RE::TESObjectREFR* refr, const ObjectType objectType, const EnchantedObjectHandling handling)
{
	auto xEnch = refr->extraList.GetByType<RE::ExtraEnchantment>();
	if (xEnch && xEnch->enchantment && ConfirmEnchanted(xEnch->enchantment, handling)) {
		DBG_DMESSAGE("ExtraList Enchantment 0x{:08x} for {}/0x{:08x}",
			xEnch->enchantment->GetFormID(), refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID());
		return objectType;
	}

	auto obj = refr->GetObjectReference();
	if (obj) {
		auto ench = obj->As<RE::TESEnchantableForm>();
		if (ench && ench->formEnchanting && ConfirmEnchanted(ench->formEnchanting, handling)) {
			DBG_DMESSAGE("Enchantment Form 0x{:08x} for {}/0x{:08x}",
				ench->formEnchanting->GetFormID(), refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID());
			return objectType;
		}
	}

	return ConvertToUnenchanted(objectType);
}

ObjectType TESFormHelper::EnchantedItemEffectiveType(const RE::TESBoundObject* obj, const ObjectType objectType, const EnchantedObjectHandling handling)
{
	if (obj) {
		auto ench = obj->As<RE::TESEnchantableForm>();
		if (ench && ench->formEnchanting && ConfirmEnchanted(ench->formEnchanting, handling)) {
			DBG_DMESSAGE("Enchantment Form 0x{:08x} for {}/0x{:08x}", ench->formEnchanting->GetFormID(), obj->GetName(), obj->GetFormID());
			return objectType;
		}
	}

	return ConvertToUnenchanted(objectType);
}

uint32_t TESFormHelper::GetGoldValue() const
{
	if (!m_form)
		return 0;

	switch (m_form->GetFormType())
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

	return static_cast<uint32_t>(pValue->value);
}

std::pair<bool, CollectibleHandling> TESFormHelper::TreatAsCollectible(const bool recordDups) const
{
	// ignore whitelist - we need the underlying object type
	return shse::CollectionManager::Instance().TreatAsCollectible(m_matcher, recordDups);
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

uint32_t TESFormHelper::CalculateWorth(void) const 
{
	if (!m_form)
		return 0;

	if (m_form->formType == RE::FormType::Ammo)
	{
		const RE::TESAmmo* ammo(m_form->As<RE::TESAmmo>());
		if (ammo)
		{
			DBG_VMESSAGE("Ammo {}({:08x}) damage = {:0.2f}", GetName(), GetFormID(), ammo->data.damage);
			return static_cast<uint32_t>(ammo->data.damage);
		}
		return 0;
	}
	else
	{
		uint32_t result(0);
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
}

const char* TESFormHelper::GetName() const
{
	return m_form->GetName();
}

uint32_t TESFormHelper::GetFormID() const
{
	return m_form->formID;
}

}