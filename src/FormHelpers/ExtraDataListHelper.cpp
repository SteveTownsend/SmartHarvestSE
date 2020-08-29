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

#include "FormHelpers/ExtraDataListHelper.h"

namespace shse
{

RE::EnchantmentItem * ExtraDataListHelper::GetEnchantment(void) const
{
	if (!m_extraData)
		return false;

	auto exEnchant = m_extraData->GetByType<RE::ExtraEnchantment>();
	return (exEnchant && exEnchant->enchantment) ? exEnchant->enchantment : nullptr;
}

bool ExtraDataListHelper::IsItemQuestObject(const RE::TESBoundObject* item) const
{
	if (!m_extraData)
		return false;

	auto exAliasArray = m_extraData->GetByType<RE::ExtraAliasInstanceArray>();
	if (!exAliasArray)
		return false;

	return std::find_if(exAliasArray->aliases.cbegin(), exAliasArray->aliases.cend(),
		[=](const RE::BGSRefAliasInstanceData* alias) -> bool {
			if (alias->alias->IsQuestObject()) {
				REL_VMESSAGE("Quest Target Item {}/0x{:08x} confirmed in alias for quest {}/0x{:08x}", item->GetName(), item->GetFormID(),
					alias->quest ? alias->quest->GetName() : "", alias->quest ? alias->quest->GetFormID() : 0);
				return true;
			}
			return false;
		}) != exAliasArray->aliases.cend();
}

bool ExtraDataListHelper::IsREFRQuestObject(const RE::TESObjectREFR* refr) const
{
	if (!m_extraData)
		return false;

	auto exAliasArray = m_extraData->GetByType<RE::ExtraAliasInstanceArray>();
	if (!exAliasArray)
		return false;

	return std::find_if(exAliasArray->aliases.cbegin(), exAliasArray->aliases.cend(),
		[=](const RE::BGSRefAliasInstanceData* alias) -> bool {
		if (alias->alias->IsQuestObject()) {
			REL_VMESSAGE("Quest Target REFR {}/0x{:08x} confirmed in alias for quest {}/0x{:08x}", refr->GetName(), refr->GetFormID(),
				alias->quest ? alias->quest->GetName() : "", alias->quest ? alias->quest->GetFormID() : 0);
			return true;
		}
		return false;
	}) != exAliasArray->aliases.cend();
}

}
