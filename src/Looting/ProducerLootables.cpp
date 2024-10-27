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
#include "Looting/objects.h"
#include "Looting/ProducerLootables.h"

namespace shse
{

ProducerLootables::SyntheticFloraCategorizer::SyntheticFloraCategorizer(
	const RE::TESObjectACTI* producer, const RE::TESLevItem* rootItem) :
	LeveledItemCategorizer(rootItem), m_producer(producer), m_contents(nullptr), m_contentsType(ObjectType::unknown)
{
}

void ProducerLootables::SyntheticFloraCategorizer::ProcessContentLeaf(RE::TESBoundObject* itemForm, ObjectType itemType)
{
	DBG_VMESSAGE("Target 0x{:08x} has concrete item {}/0x{:08x}", m_rootItem->GetFormID(), itemForm->GetName(), itemForm->GetFormID());
	if (!m_contents)
	{
		REL_VMESSAGE("Target 0x{:08x} has contents type {} in form {}/0x{:08x}", m_rootItem->GetFormID(),
			GetObjectTypeName(itemType), itemForm->GetName(), itemForm->GetFormID());
		if (!ProducerLootables::Instance().SetLootableForProducer(m_producer, itemForm ))
		{
			REL_WARNING("Leveled Item 0x{:08x} contents already present", m_rootItem->GetFormID());
		}
		else
		{
			DataCase::GetInstance()->SetObjectTypeForForm(itemForm, itemType);
			m_contents = itemForm;
			m_contentsType = itemType;
		}
	}
	else if (m_contents == itemForm)
	{
		DBG_VMESSAGE("Target 0x{:08x} contents type {} already recorded", m_rootItem->GetFormID(),
			GetObjectTypeName(itemType));
	}
	else
	{
		// warn about the conflict
		ObjectType existingType(DataCase::GetInstance()->GetObjectTypeForForm(m_contents));
		REL_WARNING("Target 0x{:08x} contents type {} conflicts with type {} for form {}/0x{:08x}", m_rootItem->GetFormID(),
			GetObjectTypeName(itemType), GetObjectTypeName(existingType), m_contents->GetName(), m_contents->GetFormID());
	}
}

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
bool ProducerLootables::SetLootableForProducer(const RE::TESForm* producer, RE::TESBoundObject* lootable)
{
	RecursiveLockGuard guard(m_producerIngredientLock);
	if (!lootable)
	{
		DBG_VMESSAGE("Producer {}/0x{:08x} needs resolving to lootable, or has none", producer->GetName(), producer->formID);
		// return value signals entry pending resolution found/not found
		return m_producerLootable.insert(std::make_pair(producer, nullptr)).second;
	}
	else
	{
		REL_VMESSAGE("Producer {}/0x{:08x} has lootable {}/0x{:08x}", producer->GetName(), producer->formID, lootable->GetName(), lootable->formID);
		m_producerLootable[producer] = lootable;
		return true;
	}
}

bool ProducerLootables::ResolveLootableForProducer(RE::TESForm* producer, RE::TESLevItem* lootableList)
{
	RecursiveLockGuard guard(m_producerIngredientLock);
	// lootable/nonIngredientLootable could be a FormList possibly empty, or a simple Form: resolved by script
	if (!lootableList)
	{
		DBG_VMESSAGE("LVLI Producer {}/0x{:08x} needs resolving to lootable, or has none", producer->GetName(), producer->formID);
		return m_producerLootable.insert(std::make_pair(producer, nullptr)).second;
	}
	else
	{
		RE::TESObjectACTI * activator(producer->As<RE::TESObjectACTI>());
		if (activator)
		{
			REL_VMESSAGE("LVLI Producer {}/0x{:08x} has lootable {}/0x{:08x}", producer->GetName(), producer->formID, lootableList->GetName(), lootableList->formID);
			SyntheticFloraCategorizer categorizer(activator, lootableList);
			categorizer.CategorizeContents();
			return categorizer.ContentsType() != ObjectType::unknown;
		}
		else
		{
			REL_WARNING("LVLI Producer {}/0x{:08x} is not ACTI - unsupported", producer->GetName(), producer->formID);
			return false;
		}
	}
}

// ingredient nullptr indicates this Producer failed resolution. We have to remove it or we will retry resolution forever
void ProducerLootables::ClearLootableForProducer(const RE::TESForm* producer)
{
	if (!producer)
		return;
	RecursiveLockGuard guard(m_producerIngredientLock);
	DBG_VMESSAGE("Producer {}/0x{:08x} failed resolution to lootable", producer->GetName(), producer->formID);
	m_producerLootable.erase(producer);
	DataCase::GetInstance()->ClearSyntheticFlora(producer->As<RE::TESBoundObject>());
}

RE::TESBoundObject* ProducerLootables::GetLootableForProducer(RE::TESForm* producer) const
{
	RecursiveLockGuard guard(m_producerIngredientLock);
	const auto matched(m_producerLootable.find(producer));
	if (matched != m_producerLootable.cend())
		return matched->second;
	return nullptr;
}

}