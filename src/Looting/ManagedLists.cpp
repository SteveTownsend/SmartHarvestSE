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

void ManagedList::Reset()
{
	// No baseline for whitelist. Blacklist has a list of known no-loot places.
	RecursiveLockGuard guard(m_listLock);
	if (this == m_blackList.get())
	{
		REL_MESSAGE("Reset BlackList");
		// seed with the always-forbidden
		m_members = DataCase::GetInstance()->OffLimitsLocations();
	}
	else
	{
		REL_MESSAGE("Reset WhiteList");
		// whitelist is rebuilt from scratch
		m_members.clear();
	}
}

void ManagedList::Add(const RE::TESForm* entry)
{
	REL_MESSAGE("Location/cell/item/container/NPC {}/0x{:08x} added to {}", entry->GetName(), entry->GetFormID(),
		this == m_blackList.get() ? "BlackList" : "WhiteList");
	RecursiveLockGuard guard(m_listLock);
	m_members.insert({ entry->GetFormID(), entry->GetName() });
}

bool ManagedList::Contains(const RE::TESForm* entry) const
{
	if (!entry)
		return false;
	RecursiveLockGuard guard(m_listLock);
	return m_members.contains(entry->GetFormID()) || HasEntryWithSameName(entry);
}

bool ManagedList::ContainsID(const RE::FormID entryID) const
{
	RecursiveLockGuard guard(m_listLock);
	return m_members.contains(entryID);
}

// sometimes multiple items use the same name - we treat them all the same
bool ManagedList::HasEntryWithSameName(const RE::TESForm* form) const
{
	// skipped if entry is a container/NPC (REFR) or place (CELL/LCTN). Only REFRs right now are to Containers and Dead Actors.
	RE::FormType formType(form->GetFormType());
	if (formType == RE::FormType::Location || formType == RE::FormType::Cell || form->As<RE::TESObjectREFR>())
		return false;
	const std::string name(form->GetName());
	return !name.empty() && std::find_if(m_members.cbegin(), m_members.cend(), [&](const auto& element) -> bool
	{
		return element.second == name;
	}) != m_members.cend();
}

}