/*************************************************************************
SmartHarvest SE
Copyright (c) Steve Townsend 2024

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

namespace shse
{

void LeveledItemCategorizer::CategorizeContents()
{
	ProcessContentsAtLevel(m_rootItem);
}

LeveledItemCategorizer::LeveledItemCategorizer(const RE::TESLevItem* rootItem) :
	m_rootItem(rootItem)
{
	m_lvliSeen.insert(m_rootItem);
}

LeveledItemCategorizer::~LeveledItemCategorizer()
{
}

void LeveledItemCategorizer::ProcessContentsAtLevel(const RE::TESLevItem* leveledItem)
{
	for (const RE::LEVELED_OBJECT& leveledObject : leveledItem->entries)
	{
		RE::TESForm* itemForm(leveledObject.form);
		if (!itemForm)
			continue;
		// Handle nesting of leveled items
		RE::TESLevItem* leveledItemForm(itemForm->As<RE::TESLevItem>());
		if (leveledItemForm)
		{
			// only process LVLI if not already seen
			if (m_lvliSeen.insert(leveledItemForm).second)
			{
				ProcessContentsAtLevel(leveledItemForm);
			}
			continue;
		}
		ObjectType itemType(DataCase::GetInstance()->GetObjectTypeForForm(itemForm));
		if (itemType != ObjectType::unknown)
		{
			RE::TESBoundObject* boundItem = itemForm->As<RE::TESBoundObject>();
			if (!boundItem)
			{
				REL_WARNING("LVLI 0x{:08x} has content leaf {}/0x{:08x} that is not TESBoundObject", m_rootItem->GetFormID(),
					itemForm->GetName(), itemForm->GetFormID());
				return;
			}
			ProcessContentLeaf(boundItem, itemType);
		}
	}
}
    
}
