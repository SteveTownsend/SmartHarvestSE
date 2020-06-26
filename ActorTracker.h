#pragma once

#include <deque>

#include "BoundedList.h"

namespace shse
{

class ActorTracker
{
public:
	static ActorTracker& Instance();
	ActorTracker();

	void Reset();
	void RecordLiveSighting(const RE::TESObjectREFR* actorRef);
	bool SeenAlive(const RE::TESObjectREFR* actorRef) const;

	void RecordTimeOfDeath(RE::TESObjectREFR* actorRef);
	bool ReleaseIfReliablyDead(BoundedList<RE::TESObjectREFR*>& refs);

private:
	static std::unique_ptr<ActorTracker> m_instance;

	// allow extended interval before looting if 'leveled list on death' perks apply to player
	static constexpr int ReallyDeadWaitIntervalSeconds = 2;
	static constexpr int ReallyDeadWaitIntervalSecondsLong = 4;

	// Dead bodies pending looting, in order of our encountering them, with the time of registration.
	// Used to delay looting bodies until the game has sorted out their state for unlocked introspection using GetContainer().
	// When looting during combat, we could try to loot _very soon_ (microseconds, potentially) after game registers the Actor's demise.
	std::deque<std::pair<RE::TESObjectREFR*, std::chrono::time_point<std::chrono::high_resolution_clock>>> m_apparentTimeOfDeath;

	// Actors we encountered alive at any point of this visit to the cell
	std::unordered_set<const RE::TESObjectREFR*> m_seenAlive;

	mutable RecursiveLock m_actorLock;
};

}