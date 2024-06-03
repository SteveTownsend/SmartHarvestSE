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
#pragma once

namespace shse
{

class PlacedObjects
{
public:
	static PlacedObjects& Instance();
	PlacedObjects();

	void RecordPlacedObjects(void);
	size_t NumberOfInstances(const RE::TESForm* form) const;

private:
	void RecordPlacedItem(const RE::TESForm* item, const RE::TESObjectREFR* refr);
	void SaveREFRIfPlaced(const RE::TESObjectREFR* refr);
	void RecordPlacedObjectsForCell(const RE::TESWorldSpace* worldSpace, const RE::TESObjectCELL* cell);

	static std::unique_ptr<PlacedObjects> m_instance;
	mutable RecursiveLock m_placedLock;

	std::unordered_set<const RE::TESForm*> m_placedItems;
	std::unordered_map<const RE::TESForm*, std::set<const RE::TESObjectREFR*>> m_placedObjects;
	std::unordered_set<const RE::TESObjectCELL*> m_checkedForPlacedObjects;
};

}
