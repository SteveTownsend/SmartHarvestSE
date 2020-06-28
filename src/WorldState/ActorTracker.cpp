#include "PrecompiledHeaders.h"
#include "ActorTracker.h"
#include "PlayerState.h"

namespace shse
{

std::unique_ptr<ActorTracker> ActorTracker::m_instance;

ActorTracker& ActorTracker::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<ActorTracker>();
	}
	return *m_instance;
}

ActorTracker::ActorTracker()
{
}

void ActorTracker::Reset()
{
	RecursiveLockGuard guard(m_actorLock);
	m_seenAlive.clear();
	m_apparentTimeOfDeath.clear();
}

void ActorTracker::RecordLiveSighting(const RE::TESObjectREFR* actorRef)
{
	RecursiveLockGuard guard(m_actorLock);
	m_seenAlive.insert(actorRef);
}

bool ActorTracker::SeenAlive(const RE::TESObjectREFR* actorRef) const
{
	RecursiveLockGuard guard(m_actorLock);
	return m_seenAlive.contains(actorRef);
}

// looting during combat is unstable, so if that option is enabled, we store the combat victims and loot them once combat ends, no sooner 
// than N seconds after their death
void ActorTracker::RecordTimeOfDeath(RE::TESObjectREFR* refr)
{
	RecursiveLockGuard guard(m_actorLock);
	m_apparentTimeOfDeath.emplace_back(std::make_pair(refr, std::chrono::high_resolution_clock::now()));
	DBG_MESSAGE("Enqueued dead body to loot later 0x%08x", refr->GetFormID());
}

// return false iff output list is full
bool ActorTracker::ReleaseIfReliablyDead(BoundedList<RE::TESObjectREFR*>& refs)
{
	RecursiveLockGuard guard(m_actorLock);
	const int interval(shse::PlayerState::Instance().PerksAddLeveledItemsOnDeath() ? ReallyDeadWaitIntervalSecondsLong : ReallyDeadWaitIntervalSeconds);
	const auto cutoffPoint(std::chrono::high_resolution_clock::now() - std::chrono::milliseconds(static_cast<long long>(interval * 1000.0)));
	while (!m_apparentTimeOfDeath.empty() && m_apparentTimeOfDeath.front().second <= cutoffPoint)
	{
		// this actor died long enough ago that we trust actor->GetContainer not to crash, provided the ID is still usable
		RE::TESObjectREFR* refr(m_apparentTimeOfDeath.front().first);
		if (!RE::TESForm::LookupByID<RE::TESObjectREFR>(refr->GetFormID()))
		{
			DBG_MESSAGE("Process enqueued dead body 0x%08x", refr->GetFormID());
		}
		else
		{
			DBG_MESSAGE("Suspect enqueued dead body ID 0x%08x", refr->GetFormID());
		}
		m_apparentTimeOfDeath.pop_front();
		if (!refs.Add(refr))
			return false;
	}
	return true;
}

}
