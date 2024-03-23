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
#include "Looting/TheftCoordinator.h"
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
    ScanStatus status(UIState::Instance().OKToScan());
    decltype(m_queuedGlow) queued;
    {
        RecursiveLockGuard lock(m_queueLock);
        if (m_queuedGlow.size())
        {
            if (status != ScanStatus::GoodToGo)
            {
                REL_WARNING("Delay {} queued Glow requests, scan status {}", m_queuedGlow.size(), status);
                return;
            }
            queued.swap(m_queuedGlow);
            DBG_VMESSAGE("Dispatch queue of {} Glow requests", queued.size());
        }
        else
        {
	        DBG_VMESSAGE("No pending glow requests");
            return;
        }

    }
    // Pass in current queued requests by value, as this executes asynchronously
    m_taskInterface->AddTask([=] (void) {
        RE::TESObjectREFR* refr;
        int duration;
        GlowReason glowReason;
        std::unordered_set<RE::FormID> doneRefrs;
        for (const auto request: queued) {
            std::tie(refr, duration, glowReason) = request;
            if (!refr)
            {
	            REL_WARNING("Skipping invalid glow request for null REFR");
                continue;
            }
            if (!doneRefrs.insert(refr->GetFormID()).second)
            {
	            REL_WARNING("Skipping repeat glow request for REFR 0x{:08x}", refr->GetFormID());
                continue;
            }
            if (refr->Is3DLoaded() && !refr->IsDisabled())
            {
                RE::TESEffectShader* shader(nullptr);
                if (static_cast<int>(glowReason) >= 0 && static_cast<int>(glowReason) <= static_cast<int>(GlowReason::SimpleTarget))
                {
                    shader = m_shaders[static_cast<int>(glowReason)];
                }
                else
                {
                    shader = m_shaders[static_cast<int>(GlowReason::SimpleTarget)];
                }
                if (!shader)
                {
	                REL_WARNING("Skipping glow request for REFR 0x{:08x}, no shader", refr->GetFormID());
                    continue;
                }
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

void TaskDispatcher::EnqueueCheckIfUndetected(RE::Actor* actor, const bool dryRun)
{
    m_taskInterface->AddTask([=] (void) {
        // Logic from po3 Papyrus Extender
        // https://github.com/powerof3/PapyrusExtenderSSE/blob/master/include/Papyrus/Functions/Detection.h
        std::string message;
        bool detected(false);
        if (!actor)
        {
            message = "No Actor for detection check";
            REL_ERROR(message);
            detected = true;
        }
        else
        {
            ScanStatus status(UIState::Instance().OKToScan());
            if (status != ScanStatus::GoodToGo)
            {
                message = "UI Open : Actor Detection interrupted";
                REL_WARNING(message);
                detected = true;
            }
            else if (actor->GetActorRuntimeData().currentProcess)
            {
                if (const auto processLists = RE::ProcessLists::GetSingleton(); processLists)
                {
                    for (auto& targetHandle : processLists->highActorHandles)
                    {
                        if (const auto target = targetHandle.get(); target && target->GetActorRuntimeData().currentProcess)
                        {
                            if (const auto base = target->GetActorBase(); base && !base->AffectsStealthMeter())
                            {
                                continue;
                            }
                            if (target->RequestDetectionLevel(actor) > 0)
                            {
                                detected = true;
                                message = "Player detected by ";
                                message.append(target->GetActorBase()->GetName());
                                DBG_MESSAGE(message);
                                break;
                            }
                        }
                    }
                }
            }
        }

        if (dryRun)
        {
            RE::DebugNotification(message.c_str());
        }
        else
        {
		    TheftCoordinator::Instance().StealOrForgetItems(detected);
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

// should only fire if we are managing carry weight
void TaskDispatcher::EnqueueCarryWeightDelta(int weightDelta)
{
    m_taskInterface->AddTask([=] (void) {
        RE::ActorValueOwner* actorValueOwner(RE::PlayerCharacter::GetSingleton()->AsActorValueOwner());
        if (actorValueOwner)
        {
            int carryWeight = static_cast<int>(actorValueOwner->GetActorValue(RE::ActorValue::kCarryWeight));
            REL_VMESSAGE("Adjusting Player.CarryWeight delta by {} from {}", weightDelta, carryWeight);
            actorValueOwner->ModActorValue(RE::ActorValue::kCarryWeight, static_cast<float>(weightDelta));
        }
    });
}

void TaskDispatcher::EnqueueResetCarryWeight()
{
    static const int InfiniteWeight(100000);
    m_taskInterface->AddTask([=] (void) {
        RE::ActorValueOwner* actorValueOwner(RE::PlayerCharacter::GetSingleton()->AsActorValueOwner());
        if (actorValueOwner)
        {
            int carryWeight = static_cast<int>(actorValueOwner->GetActorValue(RE::ActorValue::kCarryWeight));
            int weightDelta(0);
            while (carryWeight > InfiniteWeight)
            {
                weightDelta -= InfiniteWeight;
                carryWeight -= InfiniteWeight;
            }
            while (carryWeight < 0)
            {
                weightDelta += InfiniteWeight;
                carryWeight += InfiniteWeight;
            }

            if (weightDelta != 0)
            {
                actorValueOwner->ModActorValue(RE::ActorValue::kCarryWeight, static_cast<float>(weightDelta));
                REL_VMESSAGE("Removing Player.CarryWeight delta={} from {}", weightDelta, carryWeight);
            }
            else
            {
                REL_VMESSAGE("Player.CarryWeight left as is {}", carryWeight);
            }
        }
    });
}

}
