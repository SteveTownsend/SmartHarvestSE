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

RE::EnchantmentItem * ExtraDataListHelper::GetEnchantment(void)
{
	if (!m_extraData)
		return false;

	auto exEnchant = m_extraData->GetByType<RE::ExtraEnchantment>();
	return (exEnchant && exEnchant->enchantment) ? exEnchant->enchantment : nullptr;
}

bool ExtraDataListHelper::IsQuestObject(const bool requireFullQuestFlags)
{
	if (!m_extraData)
		return false;

	auto exAliasArray = m_extraData->GetByType<RE::ExtraAliasInstanceArray>();
	if (!exAliasArray)
		return false;

	return std::find_if(exAliasArray->aliases.cbegin(), exAliasArray->aliases.cend(),
		[=](const RE::BGSRefAliasInstanceData* alias) -> bool {
			if (alias->alias->IsQuestObject() || (!requireFullQuestFlags && alias->quest)) {
				DBG_VMESSAGE("Quest Item confirmed in alias for quest 0x%08x, alias quest object %s",
					alias->quest ? alias->quest->formID : 0, alias->alias->IsQuestObject() ? "true" : "false");
				return true;
			}
			return false;
		}) != exAliasArray->aliases.cend();
}

