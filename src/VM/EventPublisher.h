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
#include <random>

#include "Looting/InventoryItem.h"

namespace shse
{

constexpr RE::FormID QuestAliasFormID = 0x800;

class EventPublisher{
public:
	static EventPublisher& Instance();
	EventPublisher();
	bool GoodToGo();

	void TriggerGetProducerLootable(RE::TESObjectREFR* refr);
	void TriggerCarryWeightDelta(const int delta);
	void TriggerResetCarryWeight();
	void TriggerMining(RE::TESObjectREFR* refr, const ResourceType resourceType, const bool manualLootNotify, const bool isFirehose);
	void TriggerHarvest(RE::TESObjectREFR* refr, const RE::TESBoundObject* lootable, const ObjectType objType, int itemCount, const bool isSilent,
		const bool collectible, const float ingredientCount, const bool isWhitelisted);
	void TriggerHarvestSyntheticFlora(RE::TESObjectREFR* refr, const RE::TESBoundObject* lootable, const ObjectType objType, int itemCount, const bool isSilent,
		const bool collectible, const float ingredientCount, const bool isWhitelisted);
	void TriggerLootFromNPC(RE::TESObjectREFR* npc, RE::TESForm* item, int itemCount, ObjectType objectType, const bool collectible);
	void TriggerFlushAddedItems(void);
	void TriggerObjectGlow(RE::TESObjectREFR* refr, const int duration, const GlowReason glowReason);
	void TriggerCheckOKToScan(const int nonce);
	void TriggerStealIfUndetected(const size_t actorCount, const bool dryRun);
	void TriggerGameReady(void);

private:
	RE::BGSRefAlias* GetScriptTarget(const char* espName, RE::FormID questID);
	void HookUp();

	static EventPublisher* m_instance;
	RE::BGSRefAlias* m_eventTarget;

	SKSE::RegistrationSet<RE::TESObjectREFR*> m_onGetProducerLootable;
	SKSE::RegistrationSet<int> m_onCarryWeightDelta;
	SKSE::RegistrationSet<> m_onResetCarryWeight;
	SKSE::RegistrationSet<RE::TESObjectREFR*, RE::TESForm*, std::string, int, int, bool, bool, float, bool> m_onHarvest;
	SKSE::RegistrationSet<RE::TESObjectREFR*, RE::TESForm*, std::string, int, int, bool, bool, float, bool> m_onHarvestSyntheticFlora;
	SKSE::RegistrationSet<RE::TESObjectREFR*, int, bool, bool> m_onMining;
	SKSE::RegistrationSet<RE::TESObjectREFR*, RE::TESForm*, int, int, bool> m_onLootFromNPC;
	SKSE::RegistrationSet<> m_onFlushAddedItems;
	SKSE::RegistrationSet<RE::TESObjectREFR*, int, int> m_onObjectGlow;
	SKSE::RegistrationSet<int> m_onCheckOKToScan;
	SKSE::RegistrationSet<int, bool> m_onStealIfUndetected;
	SKSE::RegistrationSet<> m_onGameReady;
};

}
