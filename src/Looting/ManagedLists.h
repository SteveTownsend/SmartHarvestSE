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

// blacklist and whitelist, can contain container (REFR), location, cell or item (base object, not REFR)
class ManagedList
{
public:
	static ManagedList& BlackList();
	static ManagedList& WhiteList();
	ManagedList() {}

	void Reset(const bool reloadGame);
	void Add(const RE::TESForm* entry);
	void Drop(const RE::TESForm* entry);
	bool Contains(const RE::TESForm* entry) const;

private:
	bool HasEntryWithSameName(const RE::TESForm* form) const;

	static std::unique_ptr<ManagedList> m_blackList;
	static std::unique_ptr<ManagedList> m_whiteList;

	std::unordered_set<const RE::TESForm*> m_members;
	mutable RecursiveLock m_listLock;
};

}