/*************************************************************************
SmartHarvest SE
Copyright (c) Steve Townsend 2021

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

#include "WorldState/CraftingItems.h"

namespace shse
{

std::unique_ptr<CraftingItems> CraftingItems::m_instance;

CraftingItems& CraftingItems::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<CraftingItems>();
	}
	return *m_instance;
}

CraftingItems::CraftingItems()
{
}

bool CraftingItems::IsCraftingItem(const RE::TESForm* item) const
{
	return m_craftingItems.contains(item->GetFormID());
}

bool CraftingItems::AddIfNew(const RE::TESForm* item)
{
	if (m_craftingItems.insert(item->GetFormID()).second)
	{
		DBG_MESSAGE("Crafting item {}/0x{:08x}", item->GetName(), item->GetFormID());
		return true;
	}
	return false;
}

}