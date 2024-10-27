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
#pragma once

namespace shse
{

class LeveledItemCategorizer
{
public:
	LeveledItemCategorizer(const RE::TESLevItem* rootItem);
	virtual ~LeveledItemCategorizer();
	void CategorizeContents();

private:
	void ProcessContentsAtLevel(const RE::TESLevItem* leveledItem);

protected:
	virtual void ProcessContentLeaf(RE::TESBoundObject* itemForm, ObjectType itemType) = 0;

	const RE::TESLevItem* m_rootItem;
	// prevent infinite recursion
	std::unordered_set<const RE::TESLevItem*> m_lvliSeen;
};

}