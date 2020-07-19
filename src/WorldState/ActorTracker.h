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

#include <deque>
#include "Looting/IRangeChecker.h"

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
	void ReleaseIfReliablyDead(DistanceToTarget& refs);
	void AddDetective(const RE::Actor*, const double distance);
	std::vector<const RE::Actor*> ReadDetectives();
	void ClearDetectives();

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
	// possible detecting NPCs, ordered by proximity to Player to expedite detection
	std::map<double, const RE::Actor*> m_detectives;

	mutable RecursiveLock m_actorLock;
};

}