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

#include "Data/DataCase.h"
#include "Looting/ManagedLists.h"

namespace shse
{

std::unique_ptr<ManagedList> ManagedList::m_blackList;
std::unique_ptr<ManagedList> ManagedList::m_whiteList;

ManagedList& ManagedList::BlackList()
{
	if (!m_blackList)
	{
		m_blackList = std::make_unique<ManagedList>();
	}
	return *m_blackList;
}

ManagedList& ManagedList::WhiteList()
{
	if (!m_whiteList)
	{
		m_whiteList = std::make_unique<ManagedList>();
	}
	return *m_whiteList;
}

void ManagedList::Reset(const bool reloadGame)
{
	// No baseline for whitelist. Blacklist has a list of known no-loot places.
	if (this == m_blackList.get())
	{
		DBG_MESSAGE("Reset list of locations excluded from looting");
		RecursiveLockGuard guard(m_listLock);
		// seed with the always-forbidden
		m_members = DataCase::GetInstance()->OffLimitsLocations();
	}
	else
	{
		// whitelist is rebuilt from scratch
		m_members.clear();
	}
}

void ManagedList::Add(const RE::TESForm* entry)
{
	DBG_MESSAGE("Location/cell/item {}/0x{:08x} {} for looting", entry->GetName(), entry->GetFormID(),
		this == m_blackList.get() ? "blacklisted" : "whitelisted");
	RecursiveLockGuard guard(m_listLock);
	m_members.insert(entry);
}

void ManagedList::Drop(const RE::TESForm* entry)
{
	DBG_MESSAGE("Location/cell/item {}/0x{:08x} no longer {} for looting", entry->GetName(), entry->GetFormID(),
		this == m_blackList.get() ? "blacklisted" : "whitelisted");
	RecursiveLockGuard guard(m_listLock);
	m_members.erase(entry);
}

// sometimes multiple items use the same name - we treat them all the same
bool ManagedList::Contains(const RE::TESForm* entry) const
{
	if (!entry)
		return false;
	RecursiveLockGuard guard(m_listLock);
	return m_members.contains(entry) || HasEntryWithSameName(entry->GetName());
}

bool ManagedList::HasEntryWithSameName(const std::string& name) const
{
	return !name.empty() && std::find_if(m_members.cbegin(), m_members.cend(), [&](const RE::TESForm* form) -> bool
	{
		return form->GetName() == name;
	}) != m_members.cend();
}

}