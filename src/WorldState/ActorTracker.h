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
#include "WorldState/PartyMembers.h"
#include "Collections/CollectionManager.h"

namespace shse
{


class PartyVictim {
public:
	PartyVictim(const RE::Actor* victim, const float gameTime);
	PartyVictim(const std::string& name, const float gameTime);
	void AsJSON(nlohmann::json& j) const;
	inline float GameTime() const { return m_gameTime; }
	std::string AsString() const;

private:
	const std::string m_victim;
	const float m_gameTime;
};

void to_json(nlohmann::json& j, const PartyVictim& partyVictim);

class ActorTracker
{
public:
	static ActorTracker& Instance();
	ActorTracker();
	void AsJSON(nlohmann::json& j) const;
	void UpdateFrom(const nlohmann::json& j);

	void Reset();
	void RecordLiveSighting(const RE::TESObjectREFR* actorRef);
	bool SeenAlive(const RE::TESObjectREFR* actorRef) const;

	void RecordTimeOfDeath(RE::TESObjectREFR* actorRef);
	void RecordIfKilledByParty(const RE::Actor* actor);
	void ReleaseIfReliablyDead(DistanceToTarget& refs);

	void AddFollower(const RE::Actor* follower);
	inline Followers GetFollowers() const {	return m_followers; }
	void ClearFollowers();
	void ClearVictims();

private:
	static std::unique_ptr<ActorTracker> m_instance;

	void RecordVictim(const PartyVictim& victim);

	// allow extended interval before looting if 'leveled list on death' perks apply to player
	static constexpr int ReallyDeadWaitIntervalSeconds = 2;
	static constexpr int ReallyDeadWaitIntervalSecondsLong = 4;

	// Dead bodies pending looting, in order of our encountering them, with the time of registration.
	// Used to delay looting bodies until the game has sorted out their state for unlocked introspection using GetContainer().
	// When looting during combat, we could try to loot _very soon_ (microseconds, potentially) after game registers the Actor's demise.
	std::deque<std::pair<RE::TESObjectREFR*, std::chrono::time_point<std::chrono::high_resolution_clock>>> m_apparentTimeOfDeath;

	// Actors we encountered alive at any point of this visit to the cell
	std::unordered_set<const RE::TESObjectREFR*> m_seenAlive;
	std::unordered_set<const RE::Actor*> m_checkedBodies;
	// Followers in range i.e. the player's current party
	Followers m_followers;
	std::vector<PartyVictim> m_victims;

	mutable RecursiveLock m_actorLock;
};

void to_json(nlohmann::json& j, const ActorTracker& actorTracker);

}