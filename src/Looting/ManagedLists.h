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

// blacklist and whitelist - can contain Container/Dead Actor (REFR), location, cell or item (base object, not REFR)
class ManagedTargets;

class ManagedList
{
public:
	static ManagedList& BlackList();
	static ManagedList& WhiteList();
	static ManagedList& EquippedOrWorn();
	static ManagedTargets& TransferList();
	ManagedList() = delete;
	ManagedList(const char* label) : m_label(label) {}
	virtual ~ManagedList() {}

	virtual void Reset();
	void Add(RE::TESForm* entry);
	virtual bool Contains(const RE::TESForm* entry) const;
	bool ContainsID(const RE::FormID entryID) const;

protected:
	std::unordered_map<RE::FormID, std::string> m_members;
	mutable RecursiveLock m_listLock;
	std::string m_label;

private:
	bool HasEntryWithSameName(const RE::TESForm* form) const;

	static std::unique_ptr<ManagedList> m_blackList;
	static std::unique_ptr<ManagedList> m_whiteList;
	static std::unique_ptr<ManagedList> m_equippedOrWorn;
	static std::unique_ptr<ManagedTargets> m_transferList;
};

typedef std::pair<RE::TESForm*, std::string> ManagedTarget;

class ManagedTargets : public ManagedList
{
public:
	using ManagedList::ManagedList;

	virtual void Reset() override;
	void AddNamed(RE::TESForm* entry, const std::string& name);
	virtual bool Contains(const RE::TESForm* entry) const override;
	bool HasContainer(RE::FormID container) const;
	ManagedTarget ByIndex(const size_t index) const;

private:
	std::vector<ManagedTarget> m_orderedList;
	std::unordered_set<RE::FormID> m_containers;
};

}