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
#include "PrecompiledHeaders.h"

#include "VM/TaskDispatcher.h"
#include "Collections/CollectionManager.h"
#include "Utilities/utils.h"
#include "Utilities/version.h"

namespace shse
{

TaskDispatcher* TaskDispatcher::m_instance(nullptr);

TaskDispatcher& TaskDispatcher::Instance()
{
	if (!m_instance)
	{
		m_instance = new TaskDispatcher;
	}
	return *m_instance;
}

TaskDispatcher::TaskDispatcher() : m_player(nullptr)
{
    m_taskInterface = SKSE::GetTaskInterface();
}

void TaskDispatcher::EnqueueObjectGlow(RE::TESObjectREFR* refr, const int duration, const GlowReason glowReason)
{
    RecursiveLockGuard lock(m_queueLock);
    m_queuedGlow.emplace_back(refr, duration, glowReason);
}

void TaskDispatcher::GlowObjects()
{
    // Dispatch the queued glow requests via the TaskInterface
    decltype(m_queuedGlow) queued;
    {
        RecursiveLockGuard lock(m_queueLock);
        if (m_queuedGlow.size())
        {
            queued.swap(m_queuedGlow);
        }
        else
        {
	        DBG_VMESSAGE("No pending glow requests");
            return;
        }

    }
    if (!UIState::Instance().OKToScan())
    {
	    REL_WARNING("Discard queued {} queueud Glow requests, scan disallowed", queued.size());
        return;
    }
    else
    {
	    DBG_VMESSAGE("Dispatch {} queued Glow requests", queued.size());
    }
    // Pass in current queued requests by value, as this executes asynchronously
    m_taskInterface->AddTask([=] (void) {
        RE::TESObjectREFR* refr;
        int duration;
        GlowReason glowReason;
        for (const auto request: queued) {
            RE::TESEffectShader* shader(nullptr);
            std::tie(refr, duration, glowReason) = request;
            if (static_cast<int>(glowReason) > 0 && static_cast<int>(glowReason) < static_cast<int>(GlowReason::SimpleTarget))
            {
                shader = m_shaders[static_cast<int>(glowReason)];
            }
            else
            {
                shader = m_shaders[static_cast<int>(GlowReason::SimpleTarget)];
            }
            if (shader && refr && refr->Is3DLoaded() && !refr->IsDisabled())
            {
                refr->ApplyEffectShader(shader, static_cast<float>(duration));
            }
        }
    });
}

void TaskDispatcher::SetShader(const int index, RE::TESEffectShader* shader)
{
    if (!shader)
    {
        return;
    }
	REL_MESSAGE("Shader 0x{:08x} set for GlowReason {}", shader->formID, GlowName(static_cast<GlowReason>(index)));
    m_shaders[index] = shader;
}

void TaskDispatcher::EnqueueLootFromNPC(
    RE::TESObjectREFR* npc, RE::TESBoundObject* item, const int count, const ObjectType objectType)
{
    if (!npc || !item)
        return;
    RecursiveLockGuard lock(m_queueLock);
    m_queuedNPCLoot.emplace_back(npc, item, count, objectType);
}

void TaskDispatcher::LootNPCs()
{
    // Dispatch the queued NPC Loot requests via the TaskInterface
    decltype(m_queuedNPCLoot) queued;
    {
        RecursiveLockGuard lock(m_queueLock);
        if (m_queuedNPCLoot.size())
        {
            queued.swap(m_queuedNPCLoot);
        }
        else
        {
	        DBG_VMESSAGE("No pending NPC Loot requests");
            return;
        }

    }
    DBG_VMESSAGE("Dispatch {} queued Loot NPC requests", queued.size());
    // Pass in current queued requests by value, as this executes asynchronously
    m_taskInterface->AddTask([=] (void) {
        RE::TESObjectREFR* npc;
        RE::TESBoundObject* item;
        int count;
        ObjectType objectType;
        for (const auto request: queued) {
            std::tie(npc, item, count, objectType) = request;
            DBG_VMESSAGE("Loot NPC: REFR 0x{:08x} to NPC {}/0x{:08x} {} of item {}", npc->GetFormID(),
                npc->GetBaseObject()->GetName(), npc->GetBaseObject()->GetFormID(), count, item->GetName());
            // record receipt of item, if collectible
            CollectionManager::Collectibles().CheckEnqueueAddedItem(item, INIFile::SecondaryType::deadbodies, objectType);
            npc->RemoveItem(item, count, RE::ITEM_REMOVE_REASON::kRemove, nullptr, m_player);
        }
    });
}

void TaskDispatcher::SetPlayer(RE::Actor* player)
{
    if (!player)
    {
        return;
    }
	REL_MESSAGE("REFR for Player 0x{:08x} for NPC Loot transfer", player->formID);
    m_player = player;
}

}
