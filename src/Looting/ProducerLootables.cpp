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
#include "Data/dataCase.h"
#include "ProducerLootables.h"

namespace shse
{

std::unique_ptr<ProducerLootables> ProducerLootables::m_instance;

ProducerLootables& ProducerLootables::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<ProducerLootables>();
	}
	return *m_instance;
}

// ingredient nullptr indicates this Producer is pending resolution
bool ProducerLootables::SetLootableForProducer(RE::TESForm* producer, RE::TESForm* lootable)
{
	RecursiveLockGuard guard(m_producerIngredientLock);
	if (!lootable)
	{
		DBG_VMESSAGE("Producer {}/0x{:08x} needs resolving to lootable", producer->GetName(), producer->formID);
		// return value signals entry pending resolution found/not found
		return m_producerLootable.insert(std::make_pair(producer, nullptr)).second;
	}
	else
	{
		// lootable/nonIngredientLootable could be a FormList possibly empty, or a simple Form: resolved by script
		if (!lootable)
		{
			REL_WARNING("Producer {}/0x{:08x} has no lootable", producer->GetName(), producer->formID);
			DataCase::GetInstance()->BlockForm(producer, Lootability::ProducerHasNoLootable);
		}
		else
		{
			DBG_VMESSAGE("Producer {}/0x{:08x} has lootable {}/0x{:08x}", producer->GetName(), producer->formID, lootable->GetName(), lootable->formID);
			m_producerLootable[producer] = lootable;
		}
		return true;
	}
}

RE::TESForm* ProducerLootables::GetLootableForProducer(RE::TESForm* producer) const
{
	RecursiveLockGuard guard(m_producerIngredientLock);
	const auto matched(m_producerLootable.find(producer));
	if (matched != m_producerLootable.cend())
		return matched->second;
	return nullptr;
}

}