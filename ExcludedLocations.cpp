#include "PrecompiledHeaders.h"
#include "ExcludedLocations.h"

std::unique_ptr<ExcludedLocations> ExcludedLocations::m_instance;

ExcludedLocations& ExcludedLocations::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<ExcludedLocations>();
	}
	return *m_instance;
}

void ExcludedLocations::Reset()
{
	DBG_MESSAGE("Reset list of locations excluded from looting");
	RecursiveLockGuard guard(m_excludedLock);
	// seed with the always-forbidden
	m_excluded = DataCase::GetInstance()->OffLimitsLocations();
}

void ExcludedLocations::Add(const RE::TESForm* location)
{
	// confirm this is a location or cell
	if (!location->As<RE::TESObjectCELL>() && !location->As<RE::BGSLocation>())
		return;
	DBG_MESSAGE("Location/cell %s/0x%08x excluded from looting", location->GetName(), location->GetFormID());
	RecursiveLockGuard guard(m_excludedLock);
	m_excluded.insert(location);
}

void ExcludedLocations::Drop(const RE::TESForm* location)
{
	// confirm this is a location or cell
	if (!location->As<RE::TESObjectCELL>() && !location->As<RE::BGSLocation>())
		return;
	DBG_MESSAGE("Location/cell %s/0x%08x no longer excluded from looting", location->GetName(), location->GetFormID());
	RecursiveLockGuard guard(m_excludedLock);
	m_excluded.erase(location);
}

bool ExcludedLocations::Contains(const RE::TESForm* location) const
{
	RecursiveLockGuard guard(m_excludedLock);
	return m_excluded.count(location) > 0;
}
