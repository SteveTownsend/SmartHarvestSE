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

namespace shse
{

class PartyUpdate {
public:
	PartyUpdate(const RE::Actor* follower, const PartyUpdateType eventType, const float gameTime);

	void AsJSON(nlohmann::json& j) const;

private:
	const RE::Actor* m_follower;
	const PartyUpdateType m_eventType;
	const float m_gameTime;
};

void to_json(nlohmann::json& j, const PartyUpdate& partyUpdate);

typedef std::unordered_set<const RE::Actor*> Followers;
class PartyMembers
{
public:
	static PartyMembers& Instance();
	PartyMembers() {}

	void Reset();
	void AdjustParty(const Followers& followers, const float gameTime);

	void AsJSON(nlohmann::json& j) const;

private:
	static std::unique_ptr<PartyMembers> m_instance;
	std::vector<PartyUpdate> m_partyUpdates;
	Followers m_followers;
	mutable RecursiveLock m_partyLock;
};

void to_json(nlohmann::json& j, const PartyMembers& partyMembers);

}
