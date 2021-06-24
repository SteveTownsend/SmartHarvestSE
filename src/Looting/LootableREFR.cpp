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

#include "Looting/LootableREFR.h"
#include "Data/dataCase.h"
#include "FormHelpers/ExtraDataListHelper.h"
#include "FormHelpers/FormHelper.h"
#include "Looting/objects.h"
#include "WorldState/QuestTargets.h"

namespace shse
{
       
LootableREFR::LootableREFR(const RE::TESObjectREFR* ref, const INIFile::SecondaryType scope) : m_ref(ref), m_scope(scope), m_lootable(nullptr)
{
	// Projectile REFRs need to be mapped to lootable Ammo
	const RE::Projectile* projectile(ref->As<RE::Projectile>());
	if (projectile && projectile->ammoSource)
	{
		m_lootable = projectile->ammoSource;
		m_objectType = ObjectType::ammo;
		DBG_MESSAGE("Projectile REFR 0x{:08x} with Base {}/0x{:08x} mapped to Ammo {}/0x{:08x}",
			m_ref->GetFormID(), m_ref->GetBaseObject()->GetName(), m_ref->GetBaseObject()->GetFormID(),
			m_lootable->GetName(), m_lootable->GetFormID());
	}
	else if (scope == INIFile::SecondaryType::itemObjects)
	{
		DataCase* data = DataCase::GetInstance();
		m_lootable = data->ConvertIfLeveledItem(m_ref->GetBaseObject());
		if (m_ref->formType == RE::FormType::ActorCharacter)
		{
			// derived from REFR directly
			m_objectType = ObjectType::actor;
		}
		else if (m_lootable->formType == RE::FormType::Activator && HasAshPile(m_ref))
		{
			m_objectType = ObjectType::unknown;
		}
		else
		{
			m_objectType = GetBaseObjectType(m_lootable);
		}
		DBG_MESSAGE("REFR 0x{:08x} Base {}/0x{:08x} Object Type {}",
			m_ref->GetFormID(), m_lootable->GetName(), m_lootable->GetFormID(), GetObjectTypeName(m_objectType));
	}
	m_typeName = GetObjectTypeName(m_objectType);
	m_critter = m_objectType == ObjectType::critter;
	m_flora = HasIngredient();
}

std::pair<bool, CollectibleHandling> LootableREFR::TreatAsCollectible(void) const
{
	TESFormHelper itemEx(m_lootable ? m_lootable : m_ref->GetBaseObject(), m_objectType, m_scope);
	static const bool recordDups(true);		// final decision to loot the item happens here
	return itemEx.TreatAsCollectible(recordDups);
}

bool LootableREFR::IsValuable() const
{
	TESFormHelper itemEx(m_lootable ? m_lootable : m_ref->GetBaseObject(), m_objectType, m_scope);
	return itemEx.IsValuable();
}

bool LootableREFR::IsHarvestable() const
{
	bool canHarvest(m_critter || m_flora);
#if DEBUG
	auto target(m_lootable ? m_lootable : m_ref->GetBaseObject());
	DBG_VMESSAGE("check bound object {}/0x{:08x} with type {} harvestable={}",
		target->GetName(), target->GetFormID(), target->GetFormType(), canHarvest);
#endif
	return canHarvest;
}

bool LootableREFR::IsCritter() const
{
	return m_critter;
}

bool LootableREFR::HasIngredient() const
{
	// flora, but not those that produce cash money
	static const std::string septimsName(GetObjectTypeName(ObjectType::septims));
	auto target(m_lootable ? m_lootable : m_ref->GetBaseObject());
	return (m_typeName != septimsName && (target->As<RE::TESObjectTREE>() || target->As<RE::TESFlora>()));
}

bool LootableREFR::HarvestForbiddenForForm() const
{
	// flora, but not those that produce cash money
	if (HasIngredient())
	{
		return SettingsCache::Instance().ObjectLootingType(ObjectType::flora) == LootingType::LeaveBehind;
	}
	else if (m_critter)
	{
		return SettingsCache::Instance().ObjectLootingType(ObjectType::critter) == LootingType::LeaveBehind;
	}
	return false;
}

bool LootableREFR::IsItemLootableInPopulationCenter(ObjectType objectType) const
{
	// Config setting overrides
	if (!SettingsCache::Instance().LootAllowedItemsInSettlement())
		return false;
	// Allow auto-mining in settlements, which Mines mostly are. No picks for you!
	// Harvestables are fine too. We mostly don't want to clear the shelves of every building we walk into here.
	return IsValueWeightExempt(objectType) || IsHarvestable();
}

void LootableREFR::SetEffectiveObjectType(const ObjectType effectiveType)
{
	DBG_VMESSAGE("Update 0x{:08x} from {} to {}", m_lootable ? m_lootable->GetFormID() : m_ref->GetBaseObject()->GetFormID(),
		GetObjectTypeName(m_objectType), GetObjectTypeName(effectiveType));
	m_objectType = effectiveType;
	m_typeName = GetObjectTypeName(m_objectType);
}

const RE::TESBoundObject* LootableREFR::GetLootable() const
{
	return m_lootable;
}

void LootableREFR::SetLootable(const RE::TESBoundObject* lootable)
{
	m_lootable = lootable;
}

uint32_t LootableREFR::CalculateWorth(void) const
{
	TESFormHelper itemEx(m_lootable ? m_lootable : m_ref->GetBaseObject(), m_objectType, m_scope);
	return itemEx.GetWorth();
}

double LootableREFR::GetWeight(void) const
{
	TESFormHelper itemEx(m_lootable ? m_lootable : m_ref->GetBaseObject(), m_objectType, m_scope);
	return itemEx.GetWeight();
}

const char* LootableREFR::GetName() const
{
	return m_ref->GetName();
}

uint32_t LootableREFR::GetFormID() const
{
	return m_ref->GetBaseObject()->formID;
}

int16_t LootableREFR::GetItemCount() const
{
	if (!m_ref)
		return 1;
	if (!m_ref->GetBaseObject())
		return 1;

	const RE::ExtraCount* exCount(m_ref->extraList.GetByType<RE::ExtraCount>());
	if (exCount)
	{
		DBG_VMESSAGE("Pick up {} instances of {}/0x{:08x}", exCount->count,
			m_ref->GetBaseObject()->GetName(), m_ref->GetBaseObject()->GetFormID());
		return exCount->count;
	}
	if (m_lootable)
		return 1;
	if (m_objectType == ObjectType::oreVein)
	{
		// limit ore harvesting to constrain Player Home mining
		return SettingsCache::Instance().MaxMiningItems();
	}
	return 1;
}

}