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
#pragma once

#include "Data/dataCase.h"

namespace shse
{

class CraftingItems
{
public:
	static CraftingItems& Instance();
	CraftingItems();

	bool IsCraftingItem(const RE::TESForm* item) const;
	bool AddIfNew(const RE::TESForm* item);

private:
	static std::unique_ptr<CraftingItems> m_instance;
	// lock not required, this is seeded at start and thereafter read-only
	std::unordered_set<RE::FormID> m_craftingItems;
};

}
