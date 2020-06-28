#include "PrecompiledHeaders.h"

#include "Looting/ManagedLists.h"

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
}

void ManagedList::Add(const RE::TESForm* entry)
{
	DBG_MESSAGE("Location/cell/item %s/0x%08x excluded from looting", entry->GetName(), entry->GetFormID());
	RecursiveLockGuard guard(m_listLock);
	m_members.insert(entry);
}

void ManagedList::Drop(const RE::TESForm* entry)
{
	DBG_MESSAGE("Location/cell/item %s/0x%08x no longer excluded from looting", entry->GetName(), entry->GetFormID());
	RecursiveLockGuard guard(m_listLock);
	m_members.erase(entry);
}

bool ManagedList::Contains(const RE::TESForm* location) const
{
	RecursiveLockGuard guard(m_listLock);
	return m_members.contains(location);
}
