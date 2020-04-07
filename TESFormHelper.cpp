#include "PrecompiledHeaders.h"

#include "dataCase.h"
#include "iniSettings.h"
#include "objects.h"

#include "AlchemyItemHelper.h"
#include "TESFormHelper.h"
#include "TESObjectARMOHelper.h"
#include "TESObjectWEAPHelper.h"
#include "TESQuestHelper.h"

#include "CommonLibSSE/include/RE/BGSProjectile.h"

TESFormHelper::TESFormHelper(const RE::TESForm* form) : m_form(form)
{
	// If this is a leveled item, try to redirect to its contents
	m_form = DataCase::GetInstance()->ConvertIfLeveledItem(m_form);
	m_objectType = ClassifyType(m_form);
	m_typeName = GetObjectTypeName(m_objectType);
}

RE::BGSKeywordForm* TESFormHelper::GetKeywordForm() const
{
	RE::BGSKeywordForm* result = nullptr;

	switch (m_form->formType)
	{
	case RE::FormType::MagicEffect:
		result = KeywordFormCast<RE::EffectSetting>(m_form);
		break;
	case RE::FormType::NPC:
		result = KeywordFormCast<RE::TESActorBase>(m_form);
		break;
	case RE::FormType::Race:
		result = KeywordFormCast<RE::TESRace>(m_form);
		break;
	case RE::FormType::Armor:
		result = KeywordFormCast<RE::TESObjectARMO>(m_form);
		break;
	case RE::FormType::Weapon:
		result = KeywordFormCast<RE::TESObjectWEAP>(m_form);
		break;
	case RE::FormType::Location:
		result = KeywordFormCast<RE::BGSLocation>(m_form);
		break;
	case RE::FormType::Activator:
	case RE::FormType::TalkingActivator:
	case RE::FormType::Flora:
	case RE::FormType::Furniture:
		result = KeywordFormCast<RE::TESObjectACTI>(m_form);
		break;
	case RE::FormType::Enchantment:
	case RE::FormType::Spell:
	case RE::FormType::Scroll:
	case RE::FormType::Ingredient:
	case RE::FormType::AlchemyItem:
		result = KeywordFormCast<RE::AlchemyItem>(m_form);
		break;
	case RE::FormType::Misc:
	case RE::FormType::Apparatus:
	case RE::FormType::KeyMaster:
	case RE::FormType::SoulGem:
		result = KeywordFormCast<RE::TESObjectMISC>(m_form);
		break;
	case RE::FormType::Ammo:
		result = KeywordFormCast<RE::TESAmmo>(m_form);
		break;
	case RE::FormType::Book:
		result = KeywordFormCast<RE::TESObjectBOOK>(m_form);
		break;
	default:
		result = skyrim_cast<RE::BGSKeywordForm*, RE::TESForm>(m_form);
		break;
	}
	return result;
}

RE::EnchantmentItem* TESFormHelper::GetEnchantment()
{
	if (!m_form)
		return false;

	if (m_form->formType == RE::FormType::Weapon || m_form->formType == RE::FormType::Armor)
	{
		RE::TESEnchantableForm* enchanted(skyrim_cast<RE::TESEnchantableForm*, RE::TESForm>(m_form));
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

double TESFormHelper::GetWeight() const
{
	if (!m_form)
		return 0.0;

	const RE::TESWeightForm* pWeight(m_form->As<RE::TESWeightForm>());
	if (!pWeight)
		return 0.0;

	return pWeight->weight;
}

double TESFormHelper::GetWorth() const
{
	if (!m_form)
		return 0.;

	if (m_form->formType == RE::FormType::Ammo)
	{
		const RE::TESAmmo* ammo(m_form->As<RE::TESAmmo>());

#if _DEBUG
		_MESSAGE("Ammo %0.2f", ammo->data.damage);
#endif
		return ammo ? static_cast<int>(ammo->data.damage) : 0;
	}
	else if (m_form->formType == RE::FormType::Projectile)
	{
		const RE::BGSProjectile* proj(m_form->As<RE::BGSProjectile>());
		if (proj)
		{
			const RE::TESAmmo* ammo(DataCase::GetInstance()->ProjToAmmo(proj));
#if _DEBUG
			_MESSAGE("Projectile %0.2f", ammo->data.damage);
#endif
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
