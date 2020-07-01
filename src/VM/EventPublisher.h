#pragma once

#include <deque>
#include <random>

#include "Looting/InventoryItem.h"

constexpr RE::FormID QuestAliasFormID = 0x800;

class EventPublisher{
public:
	static EventPublisher& Instance();
	EventPublisher();
	bool GoodToGo();

	void TriggerGetProducerLootable(RE::TESObjectREFR* refr);
	void TriggerCarryWeightDelta(const int delta);
	void TriggerResetCarryWeight();
	void TriggerMining(RE::TESObjectREFR* refr, const ResourceType resourceType, const bool manualLootNotify);
	void TriggerHarvest(RE::TESObjectREFR* refr, const ObjectType objType, int itemCount, const bool isSilent, const bool manualLootNotify);
	void TriggerLootFromNPC(RE::TESObjectREFR* npc, RE::TESForm* item, int itemCount, ObjectType objectType, const INIFile::SecondaryType scope);
	void TriggerFlushAddedItems(void);
	void TriggerObjectGlow(RE::TESObjectREFR* refr, const int duration, const GlowReason glowReason);
	void TriggerCheckOKToScan(const int nonce);

private:
	RE::BGSRefAlias* GetScriptTarget(const char* espName, RE::FormID questID);
	void HookUp();

	static std::unique_ptr<EventPublisher> m_instance;
	RE::BGSRefAlias* m_eventTarget;

	SKSE::RegistrationSet<RE::TESObjectREFR*> m_onGetProducerLootable;
	SKSE::RegistrationSet<int> m_onCarryWeightDelta;
	SKSE::RegistrationSet<> m_onResetCarryWeight;
	SKSE::RegistrationSet<RE::TESObjectREFR*, int, int, bool, bool, bool> m_onHarvest;
	SKSE::RegistrationSet<RE::TESObjectREFR*, int, bool> m_onMining;
	SKSE::RegistrationSet<RE::TESObjectREFR*, RE::TESForm*, int, int, bool> m_onLootFromNPC;
	SKSE::RegistrationSet<> m_onFlushAddedItems;
	SKSE::RegistrationSet<RE::TESObjectREFR*, int, int> m_onObjectGlow;
	SKSE::RegistrationSet<int> m_onCheckOKToScan;
};