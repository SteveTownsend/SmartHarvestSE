/*************************************************************************
SmartHarvest SE
Copyright (c) Steve Townsend 2024

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
#include <tuple>

namespace shse
{

class TaskDispatcher {
public:
	static TaskDispatcher& Instance();
	TaskDispatcher();
	void EnqueueObjectGlow(RE::TESObjectREFR* refr, const int duration, const GlowReason glowReason);
    void GlowObjects();
    void SetShader(const int index, RE::TESEffectShader* shader);
    void EnqueueLootFromNPC(
        RE::TESObjectREFR* npc, RE::TESBoundObject* item, const int count, const ObjectType objectType);
    void LootNPCs();
	void EnqueueCheckIfUndetected(RE::Actor* a_actor, const bool dryRun);
    void SetPlayer(RE::Actor* player);
    void EnqueueCarryWeightDelta(int weightDelta);
    void EnqueueResetCarryWeight();

private:
    typedef std::tuple<RE::TESObjectREFR*, const int, const GlowReason> GlowRequest;
    typedef std::tuple<RE::TESObjectREFR*, RE::TESBoundObject*, const int, const ObjectType> NPCLootRequest;
	static TaskDispatcher* m_instance;
    const SKSE::TaskInterface* m_taskInterface;
    std::deque<GlowRequest> m_queuedGlow;
    std::deque<NPCLootRequest> m_queuedNPCLoot;
    RecursiveLock m_queueLock;
    std::array<RE::TESEffectShader*, static_cast<int>(GlowReason::NumberOfShaders)> m_shaders;
    RE::Actor* m_player;
};

}
