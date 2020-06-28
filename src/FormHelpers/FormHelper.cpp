#include "PrecompiledHeaders.h"

#include "AlchemyItemHelper.h"
#include "FormHelper.h"
#include "ArmorHelper.h"
#include "WeaponHelper.h"
#include "Collections/CollectionManager.h"

TESFormHelper::TESFormHelper(const RE::TESForm* form) : m_form(form)
{
	// If this is a leveled item, try to redirect to its contents
	m_form = DataCase::GetInstance()->ConvertIfLeveledItem(m_form);
	m_objectType = GetBaseFormObjectType(m_form);
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

UInt32 TESFormHelper::GetGoldValue() const
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

std::pair<bool, SpecialObjectHandling> TESFormHelper::IsCollectible(void) const
{
	// ignore whitelist - we need the underlying object type
	return shse::CollectionManager::Instance().IsCollectible(m_form);
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

double TESFormHelper::CalculateWorth() const
{
	if (!m_form)
		return 0.;

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
		double result(0.);
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
		return result == 0. ? GetGoldValue() : result;
	}
	return 0.;
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
