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
namespace ExtraDataList
{

RE::EnchantmentItem* GetEnchantment(const RE::ExtraDataList* extraData)
{
	if (!extraData)
		return nullptr;

	auto exEnchant = extraData->GetByType<RE::ExtraEnchantment>();
	return (exEnchant && exEnchant->enchantment) ? exEnchant->enchantment : nullptr;
}

bool IsItemQuestObject(const RE::TESBoundObject* item, const RE::ExtraDataList* extraData)
{
	if (!extraData)
		return false;

	auto exAliasArray = extraData->GetByType<RE::ExtraAliasInstanceArray>();
	if (!exAliasArray)
		return false;

	return std::find_if(exAliasArray->aliases.cbegin(), exAliasArray->aliases.cend(),
		[=](const RE::BGSRefAliasInstanceData* alias) -> bool {
		if (alias->alias->IsQuestObject()) {
			DBG_VMESSAGE("Quest Target Item {}/0x{:08x} confirmed in alias for quest {}/0x{:08x}", item->GetName(), item->GetFormID(),
				alias->quest ? alias->quest->GetName() : "", alias->quest ? alias->quest->GetFormID() : 0);
			return true;
		}
		return false;
	}) != exAliasArray->aliases.cend();
}

bool IsREFRQuestObject(const RE::TESObjectREFR* refr, const RE::ExtraDataList* extraData)
{
	if (!extraData)
		return false;

	auto exAliasArray = extraData->GetByType<RE::ExtraAliasInstanceArray>();
	if (!exAliasArray)
		return false;

	return std::find_if(exAliasArray->aliases.cbegin(), exAliasArray->aliases.cend(),
		[=](const RE::BGSRefAliasInstanceData* alias) -> bool {
		if (alias->alias->IsQuestObject()) {
			DBG_VMESSAGE("Quest Target REFR {}/0x{:08x} confirmed in alias for quest {}/0x{:08x}", refr->GetName(), refr->GetFormID(),
				alias->quest ? alias->quest->GetName() : "", alias->quest ? alias->quest->GetFormID() : 0);
			return true;
		}
		return false;
	}) != exAliasArray->aliases.cend();

}

}
}
