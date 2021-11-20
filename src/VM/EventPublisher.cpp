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

#include "VM/EventPublisher.h"
#include "Collections/CollectionManager.h"
#include "Utilities/utils.h"
#include "Utilities/version.h"

namespace shse
{

/* do not clean this class up: SKSE::Impl::~RegistrationSetBase (?) faults per https://github.com/SteveTownsend/SmartHarvestSE/issues/200
* 
From Map file
	0001:0009a310       ??1RegistrationSetBase@Impl@SKSE@@QEAA@XZ 000000018009b310 f   CommonLibSSE:RegistrationSet.obj

WER output crash address:
	Sig[3].Value=SmartHarvestSE.dll
	Sig[6].Name=Exception Code
	Sig[6].Value=c0000005
	Sig[7].Name=Exception Offset
	Sig[7].Value=000000000009b343
*/
EventPublisher* EventPublisher::m_instance(nullptr);

EventPublisher& EventPublisher::Instance()
{
	if (!m_instance)
	{
		m_instance = new EventPublisher;
	}
	return *m_instance;
}

EventPublisher::EventPublisher() : m_eventTarget(nullptr),
	m_onGetProducerLootable("OnGetProducerLootable"),
	m_onCarryWeightDelta("OnCarryWeightDelta"),
	m_onResetCarryWeight("OnResetCarryWeight"),
	m_onHarvest("OnHarvest"),
	m_onMining("OnMining"),
	m_onLootFromNPC("OnLootFromNPC"),
	m_onFlushAddedItems("OnFlushAddedItems"),
	m_onObjectGlow("OnObjectGlow"),
	m_onCheckOKToScan("OnCheckOKToScan"),
	m_onStealIfUndetected("OnStealIfUndetected"),
	m_onGameReady("OnGameReady")
{
}

RE::BGSRefAlias* EventPublisher::GetScriptTarget(const char* espName, RE::FormID questID)
{
	static RE::TESQuest* quest(nullptr);
	static RE::BGSRefAlias* alias(nullptr);
	if (!quest)
	{
		RE::TESForm* questForm(RE::TESDataHandler::GetSingleton()->LookupForm(questID, espName));
		if (questForm)
		{
			DBG_MESSAGE("Got Base Form {}", questForm ? FormUtils::SafeGetFormEditorID(questForm).c_str() : "nullptr");
			quest = questForm ? questForm->As<RE::TESQuest>() : nullptr;
			DBG_MESSAGE("Got Quest Form {}", quest ? FormUtils::SafeGetFormEditorID(quest).c_str() : "nullptr");
		}
	}
	if (quest && quest->IsRunning())
	{
		DBG_MESSAGE("Quest {} is running", FormUtils::SafeGetFormEditorID(quest).c_str());
		RE::BGSBaseAlias* baseAlias(quest->aliases[0]);
		if (!baseAlias)
		{
			DBG_MESSAGE("Quest has no alias at index 0");
			return nullptr;
		}

		alias = static_cast<RE::BGSRefAlias*>(baseAlias);
		if (!alias)
		{
			REL_WARNING("Quest is not type BGSRefAlias");
			return nullptr;
		}
		DBG_MESSAGE("Got BGSRefAlias for Mod's Quest");
	}
	return alias;
}

bool EventPublisher::GoodToGo()
{
	if (!m_eventTarget)
	{
		m_eventTarget = GetScriptTarget(MODNAME, QuestAliasFormID);
		// register the events
		if (m_eventTarget)
		{
		// register the events
			HookUp();
		}
	}
	return m_eventTarget != nullptr;
}

void EventPublisher::HookUp()
{
	m_onGetProducerLootable.Register(m_eventTarget);
	m_onCarryWeightDelta.Register(m_eventTarget);
	m_onResetCarryWeight.Register(m_eventTarget);
	m_onObjectGlow.Register(m_eventTarget);
	m_onHarvest.Register(m_eventTarget);
	m_onMining.Register(m_eventTarget);
	m_onLootFromNPC.Register(m_eventTarget);
	m_onFlushAddedItems.Register(m_eventTarget);
	m_onCheckOKToScan.Register(m_eventTarget);
	m_onStealIfUndetected.Register(m_eventTarget);
	m_onGameReady.Register(m_eventTarget);
}

void EventPublisher::TriggerGetProducerLootable(RE::TESObjectREFR* refr)
{
	m_onGetProducerLootable.SendEvent(refr);
}

void EventPublisher::TriggerCarryWeightDelta(const int delta)
{
	m_onCarryWeightDelta.SendEvent(delta);
}

void EventPublisher::TriggerResetCarryWeight()
{
	m_onResetCarryWeight.SendEvent();
}

void EventPublisher::TriggerMining(RE::TESObjectREFR* refr, const ResourceType resourceType, const bool manualLootNotify, const bool isFirehose)
{
	// We always block the REFR before firing this
	m_onMining.SendEvent(refr, static_cast<int>(resourceType), manualLootNotify, isFirehose);
}

void EventPublisher::TriggerHarvest(RE::TESObjectREFR* refr, const RE::TESBoundObject* lootable, const ObjectType objType, int itemCount,
	const bool isSilent, const bool collectible, const float ingredientCount, const bool isWhitelisted)
{
	// We always lock the REFR from more harvesting before firing this
	m_onHarvest.SendEvent(refr, const_cast<RE::TESBoundObject*>(lootable), lootable->GetName(), static_cast<int>(objType), itemCount,
		isSilent, collectible, ingredientCount, isWhitelisted);
}

void EventPublisher::TriggerFlushAddedItems()
{
	m_onFlushAddedItems.SendEvent();
}

void EventPublisher::TriggerLootFromNPC(RE::TESObjectREFR* npc, RE::TESForm* item, int itemCount, ObjectType objectType, const bool collectible)
{
	m_onLootFromNPC.SendEvent(npc, item, itemCount, static_cast<int>(objectType), collectible);
}

void EventPublisher::TriggerObjectGlow(RE::TESObjectREFR* refr, const int duration, const GlowReason glowReason)
{
	m_onObjectGlow.SendEvent(refr, duration, static_cast<int>(glowReason));
}

void EventPublisher::TriggerCheckOKToScan(const int nonce)
{
	m_onCheckOKToScan.SendEvent(nonce);
}

void EventPublisher::TriggerStealIfUndetected(const size_t actorCount, const bool dryRun)
{
	m_onStealIfUndetected.SendEvent(static_cast<int>(actorCount), dryRun);
}

void EventPublisher::TriggerGameReady()
{
	m_onGameReady.SendEvent();
}

}
