#include "PrecompiledHeaders.h"
#include "PlayerHouses.h"

std::unique_ptr<PlayerHouses> PlayerHouses::m_instance;

PlayerHouses& PlayerHouses::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<PlayerHouses>();
	}
	return *m_instance;
}

PlayerHouses::PlayerHouses() : m_keyword(nullptr)
{
}

void PlayerHouses::SetKeyword(RE::BGSKeyword* keyword)
{
	m_keyword = keyword;
}

void PlayerHouses::Clear()
{
	RecursiveLockGuard guard(m_housesLock);
	m_houses.clear();
}

bool PlayerHouses::Add(const RE::BGSLocation* location)
{
	RecursiveLockGuard guard(m_housesLock);
	return location && m_houses.insert(location).second;
}

bool PlayerHouses::Remove(const RE::BGSLocation* location)
{
	RecursiveLockGuard guard(m_housesLock);
	return location && m_houses.erase(location) > 0;
}

// Check indeterminate status of the location, because a requested UI check is pending
bool PlayerHouses::Contains(const RE::BGSLocation* location) const
{
	RecursiveLockGuard guard(m_housesLock);
	return location && m_houses.count(location);
}

bool PlayerHouses::IsValidHouse(const RE::BGSLocation* location) const
{
	RecursiveLockGuard guard(m_housesLock);
	return location && location->HasKeyword(m_keyword);
}
