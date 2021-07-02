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
std::unique_ptr<ManagedTargets> ManagedList::m_transferList;

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

ManagedTargets& ManagedList::TransferList()
{
	if (!m_transferList)
	{
		m_transferList = std::make_unique<ManagedTargets>();
	}
	return *m_transferList;
}

void ManagedList::Reset()
{
	// No baseline for whitelist or transfer-list. Blacklist has a list of known no-loot places.
	RecursiveLockGuard guard(m_listLock);
	if (this == m_blackList.get())
	{
		REL_MESSAGE("Reset BlackList");
		// seed with the always-forbidden
		m_members = DataCase::GetInstance()->OffLimitsLocations();
	}
	else
	{
		// whitelist and transferlist are rebuilt from scratch
		if (this == m_whiteList.get())
		{
			REL_MESSAGE("Reset WhiteList");
		}
		m_members.clear();
	}
}

void ManagedList::Add(RE::TESForm* entry)
{
	std::string name;
	const RE::TESObjectREFR* refr(entry->As<RE::TESObjectREFR>());
	if (refr)
	{
		name = refr->GetName();
	}
	if (name.empty())
	{
		name = entry->GetName();
	}
	REL_MESSAGE("{}/0x{:08x} added to {}", name, entry->GetFormID(), this == m_blackList.get() ? "BlackList" : "WhiteList");
	RecursiveLockGuard guard(m_listLock);
	m_members.insert({ entry->GetFormID(), name });
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

void ManagedTargets::Reset()
{
	REL_MESSAGE("Reset TransferList");
	ManagedList::Reset();
	m_orderedList.clear();
	m_containers.clear();
}

void ManagedTargets::AddNamed(RE::TESForm* entry, const std::string& name)
{
	RecursiveLockGuard guard(m_listLock);
	m_orderedList.push_back({ entry, name });
	// list is sparse, check form present
	if (entry)
	{
		REL_MESSAGE("{}/0x{:08x} added to TransferList", name, entry->GetFormID());
		m_members.insert({ entry->GetFormID(), name });

		// Record any underlying _linked_ container for checking of multiplexed CONTs. We do not always check CONT for match as many are reused
		// _without_ linked-ref indirection.
		RE::TESObjectREFR* refr(entry->As<RE::TESObjectREFR>());
		if (refr)
		{
			const RE::TESObjectACTI* activator(refr->GetBaseObject()->As<RE::TESObjectACTI>());
			if (activator)
			{
				DBG_VMESSAGE("Check ACTI {}/0x{:08x} -> linked container", activator->GetFullName(), activator->GetFormID());
				RE::TESObjectREFR* linkedRefr(refr->GetLinkedRef(nullptr));
				if (linkedRefr)
				{
					RE::TESObjectCONT* container = linkedRefr->GetBaseObject()->As<RE::TESObjectCONT>();
					if (container)
					{
						DBG_VMESSAGE("ACTI {}/0x{:08x} has linked container {}/0x{:08x}", activator->GetFullName(), activator->GetFormID(),
							container->GetFullName(), container->GetFormID());
						m_containers.insert(container->GetFormID());
					}
				}
			}
		}
	}
	else
	{
		REL_MESSAGE("Free entry in TransferList at index {}", m_orderedList.size() - 1);
	}
}

bool ManagedTargets::Contains(const RE::TESForm* entry) const
{
	if (!entry)
		return false;
	RecursiveLockGuard guard(m_listLock);
	if (m_members.contains(entry->GetFormID()))
		return true;
	// Check for matching linked REFR - if we autoloot the container in this instance, we may transfer loot back to it and enter a toxic loop
	RE::TESObjectREFR* refr(const_cast<RE::TESForm*>(entry)->As<RE::TESObjectREFR>());
	if (refr)
	{
		const RE::TESObjectACTI* activator(refr->GetBaseObject()->As<RE::TESObjectACTI>());
		if (activator)
		{
			DBG_VMESSAGE("Check ACTI {}/0x{:08x} for linked container", activator->GetFullName(), activator->GetFormID());
			RE::TESObjectREFR* linkedRefr(refr->GetLinkedRef(nullptr));
			if (linkedRefr)
			{
				RE::TESObjectCONT* container(linkedRefr->GetBaseObject()->As<RE::TESObjectCONT>());
				return m_containers.contains(container->GetFormID());
			}
		}
	}
	return false;
}

bool ManagedTargets::HasContainer(RE::FormID container) const
{
	RecursiveLockGuard guard(m_listLock);
	return m_containers.contains(container);
}

ManagedTarget ManagedTargets::ByIndex(const size_t index) const
{
	if (index < m_orderedList.size())
		return m_orderedList[index];
	return { nullptr, "" };
}

}