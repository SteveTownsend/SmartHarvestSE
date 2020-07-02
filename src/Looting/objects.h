#pragma once

#include "Data/iniSettings.h"

inline bool IsBookObject(ObjectType objType)
{
	return objType >= ObjectType::book && objType <= ObjectType::skillbookRead;
}

class ActorHelper
{
public:
	ActorHelper(RE::Actor* actor) : m_actor(actor) {}
	bool IsSneaking(void) const;
	bool IsPlayerAlly(void) const;
	bool IsEssential(void) const;
	bool IsSummoned(void) const;
private:
	RE::Actor* m_actor;
};

bool HasAshPile(const RE::TESObjectREFR* refr);
RE::TESObjectREFR* GetAshPile(const RE::TESObjectREFR* refr);
bool IsPlayerOwned(const RE::TESObjectREFR* refr);
RE::NiTimeController* GetTimeController(RE::TESObjectREFR* refr);
bool IsBossContainer(const RE::TESObjectREFR * refr);
bool IsContainerLocked(const RE::TESObjectREFR * refr);
ObjectType GetREFRObjectType(const RE::TESObjectREFR* refr);
ObjectType GetBaseFormObjectType(const RE::TESForm* baseForm);
// this overload deliberately has no definition, to trap at link-time misuse of the baseForm function to handle a REFR
ObjectType GetBaseFormObjectType(const RE::TESObjectREFR* refr);
// end link-time misuse guard
std::string GetObjectTypeName(ObjectType objectType);
ObjectType GetObjectTypeByTypeName(const std::string& name);
RE::EnchantmentItem* GetEnchantmentFromExtraLists(RE::BSSimpleList<RE::ExtraDataList*>* extraLists);
ResourceType ResourceTypeByName(const std::string& name);

inline bool IsHarvestable(RE::TESBoundObject* target, ObjectType objectType)
{
	DBG_VMESSAGE("check bound object %s/0x%08x with type %d", target->GetName(), target->GetFormID(), target->GetFormType());
	return objectType == ObjectType::critter || objectType == ObjectType::flora ||
		target->GetFormType() == RE::FormType::Tree || target->GetFormType() == RE::FormType::Flora;
}

inline bool IsValueWeightExempt(ObjectType objectType)
{
	return objectType == ObjectType::ammo ||
		objectType == ObjectType::lockpick ||
		objectType == ObjectType::key ||
		objectType == ObjectType::oreVein ||
		objectType == ObjectType::septims;
}

inline bool IsItemLootableInPopulationCenter(RE::TESBoundObject* target, ObjectType objectType)
{
	// Allow auto - mining in settlements, which Mines mostly are. No picks for you!
	// Harvestables are fine too. We don't want to clear the shelves of every building we walk into.
	return IsValueWeightExempt(objectType) || IsHarvestable(target, objectType);
}

bool IsPlayable(const RE::TESForm * pForm);
