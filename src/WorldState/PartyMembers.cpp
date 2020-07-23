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

#include "WorldState/PartyMembers.h"

namespace shse
{

PartyUpdate::PartyUpdate(const RE::Actor* follower, const PartyUpdateType eventType, const float gameTime) :
	m_follower(follower), m_eventType(eventType), m_gameTime(gameTime)
{
}

std::unique_ptr<PartyMembers> PartyMembers::m_instance;

PartyMembers& PartyMembers::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<PartyMembers>();
	}
	return *m_instance;
}

void PartyMembers::Reset()
{
	RecursiveLockGuard guard(m_partyLock);
	m_followers.clear();
}

void PartyMembers::AdjustParty(const Followers& followers, const float gameTime)
{
	RecursiveLockGuard guard(m_partyLock);
	// lists of followers do not get very large, just brute force it
	bool updated(false);
	for (const auto newFollower : followers)
	{
		if (m_followers.find(newFollower) == m_followers.cend())
		{
			updated = true;
			m_partyUpdates.push_back(PartyUpdate(newFollower, PartyUpdateType::Joined, gameTime));
		}
	}
	for (const auto existingFollower : m_followers)
	{
		if (followers.find(existingFollower) == m_followers.cend())
		{
			updated = true;
			PartyUpdateType updateType(existingFollower->IsDead(true) ? PartyUpdateType::Died : PartyUpdateType::Departed);
			m_partyUpdates.push_back(PartyUpdate(existingFollower, updateType, gameTime));
		}
	}
	if (updated)
	{
		m_followers = followers;
	}
}

}