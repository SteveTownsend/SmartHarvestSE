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
#include "WorldState/ActorTracker.h"
#include "WorldState/PlayerState.h"
#include "WorldState/Saga.h"

namespace shse
{

PartyVictim::PartyVictim(const RE::Actor* victim, const float gameTime)
	: m_victim(victim->GetName()), m_gameTime(gameTime)
{
}

PartyVictim::PartyVictim(const std::string& name, const float gameTime)
	: m_victim(name), m_gameTime(gameTime)
{
}

std::string PartyVictim::AsString() const
{
	std::ostringstream stream;
	stream << "I killed " << m_victim << '.';
	return stream.str();
}

void PartyVictim::AsJSON(nlohmann::json& j) const
{
	j["name"] = m_victim;
	j["time"] = m_gameTime;
}

void to_json(nlohmann::json& j, const PartyVictim& partyVictim)
{
	partyVictim.AsJSON(j);
}

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
	m_detectives.clear();
	m_followers.clear();
	m_checkedBodies.clear();
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
	DBG_MESSAGE("Enqueued dead body to loot later 0x{:08x}", refr->GetFormID());
}

void ActorTracker::RecordVictim(const PartyVictim& victim)
{
	m_victims.push_back(victim);
	Saga::Instance().AddEvent(m_victims.back());
}

void ActorTracker::RecordIfKilledByParty(const RE::Actor* victim)
{
	RecursiveLockGuard guard(m_actorLock);
	// only record the perpetrator of this heinous crime once
	if (!victim || !victim->myKiller || !m_checkedBodies.insert(victim).second)
		return;
	RE::Actor* killer(victim->myKiller.get().get());
	// it's always the player even if a FOllower did the deed
	if (killer == RE::PlayerCharacter::GetSingleton())
	{
		DBG_MESSAGE("Record killing of {}/0x{:08x}", victim->GetName(), victim->GetFormID());
		const float gameTime(PlayerState::Instance().CurrentGameTime());
		RecordVictim(PartyVictim(victim, gameTime));

		// Ensure Location is recorded
		LocationTracker::Instance().RecordCurrentPlace(gameTime);
	}
}

// looting during combat is unstable, so if that option is enabled, we store the combat victims and loot them once combat ends, no sooner 
// than N seconds after their death
void ActorTracker::ReleaseIfReliablyDead(DistanceToTarget& refs)
{
	RecursiveLockGuard guard(m_actorLock);
	const int interval(shse::PlayerState::Instance().PerksAddLeveledItemsOnDeath() ? ReallyDeadWaitIntervalSecondsLong : ReallyDeadWaitIntervalSeconds);
	const auto cutoffPoint(std::chrono::high_resolution_clock::now() - std::chrono::milliseconds(static_cast<long long>(interval * 1000.0)));
	while (!m_apparentTimeOfDeath.empty() && m_apparentTimeOfDeath.front().second <= cutoffPoint)
	{
		// this actor died long enough ago that we trust actor->GetContainer not to crash, provided the ID is still usable
		const auto nextActor(m_apparentTimeOfDeath.front());
		RE::TESObjectREFR* refr(nextActor.first);
		if (!RE::TESForm::LookupByID<RE::TESObjectREFR>(refr->GetFormID()))
		{
			DBG_MESSAGE("Process enqueued dead body 0x{:08x}", refr->GetFormID());
		}
		else
		{
			DBG_MESSAGE("Suspect enqueued dead body ID 0x{:08x}", refr->GetFormID());
		}
		RE::Actor* actor(refr->As<RE::Actor>());
		if (actor)
		{
			RecordIfKilledByParty(actor);
		}
		m_apparentTimeOfDeath.pop_front();
		// use distance 0. to prioritize looting
		refs.emplace_back(0., refr);
	}
}

void ActorTracker::AddDetective(const RE::Actor* detective, const double distance)
{
	RecursiveLockGuard guard(m_actorLock);
	m_detectives.insert({ distance, detective });
}

std::vector<const RE::Actor*> ActorTracker::GetDetectives()
{ 
	RecursiveLockGuard guard(m_actorLock);
	std::vector<const RE::Actor*> result;
	result.reserve(m_detectives.size());
	std::transform(m_detectives.cbegin(), m_detectives.cend(), std::back_inserter(result),
		[&](const auto& detective) { return detective.second; });
	return result;
}

void ActorTracker::ClearDetectives()
{
	RecursiveLockGuard guard(m_actorLock);
	m_detectives.clear();
}

void ActorTracker::AddFollower(const RE::Actor* detective)
{
	RecursiveLockGuard guard(m_actorLock);
	m_followers.insert(detective);
}

void ActorTracker::ClearFollowers()
{
	RecursiveLockGuard guard(m_actorLock);
	m_followers.clear();
}

void ActorTracker::ClearVictims()
{
	RecursiveLockGuard guard(m_actorLock);
	m_victims.clear();
}

void ActorTracker::AsJSON(nlohmann::json& j) const
{
	nlohmann::json victims(nlohmann::json::array());
	for (const auto& victim : m_victims)
	{
		victims.push_back(victim);
	}
	j["victims"] = victims;
}

// rehydrate from cosave data
void ActorTracker::UpdateFrom(const nlohmann::json& j)
{
	REL_MESSAGE("Cosave Party Victims\n{}", j.dump(2));
	RecursiveLockGuard guard(m_actorLock);
	m_victims.clear();
	m_victims.reserve(j["victims"].size());
	for (const nlohmann::json& victim : j["victims"])
	{
		RecordVictim(PartyVictim(victim["name"].get<std::string>(), victim["time"].get<float>()));
	}
}

void to_json(nlohmann::json& j, const ActorTracker& actorTracker)
{
	actorTracker.AsJSON(j);
}

}
