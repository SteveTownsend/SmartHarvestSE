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

#include "Data/SettingsCache.h"

namespace shse
{

inline bool IsBookObject(ObjectType objType)
{
	return objType >= ObjectType::book && objType <= ObjectType::skillbookRead;
}

PlayerAffinity GetPlayerAffinity(const RE::Actor* actor);
bool IsSummoned(const RE::Actor* actor);
bool StartsDead(const RE::TESObjectREFR* refr);
bool IsDisintegrating(const RE::Actor* actor);
bool IsQuestTargetNPC(const RE::Actor* actor);

bool HasAshPile(const RE::TESObjectREFR* refr);
RE::TESObjectREFR* GetAshPile(const RE::TESObjectREFR* refr);
bool IsPlayerOwned(const RE::TESObjectREFR* refr);
bool IsQuestItem(const RE::TESObjectREFR* refr);
void PrintManualLootMessage(const std::string& name);
void ProcessManualLootREFR(const RE::TESObjectREFR* refr);
void ProcessManualLootItem(const RE::TESBoundObject* item);
RE::NiTimeController* GetTimeController(RE::TESObjectREFR* refr);
bool IsBossContainer(const RE::TESObjectREFR * refr);
bool IsLocked(const RE::TESObjectREFR * refr);
ObjectType GetEffectiveObjectType(const RE::TESBoundObject* baseForm);
ObjectType GetBaseObjectType(const RE::TESForm* baseForm);
// this overload deliberately has no definition, to trap at link-time misuse of the baseForm function to handle a REFR
ObjectType GetEffectiveObjectType(const RE::TESObjectREFR* refr);
ObjectType GetBaseObjectType(const RE::TESObjectREFR* refr);
// end link-time misuse guard
ObjectType GetExcessObjectType(const RE::TESBoundObject* baseForm);
std::string GetFormTypeName(const RE::FormType formType);
std::string GetObjectTypeName(const ObjectType objectType);
ObjectType GetObjectTypeByTypeName(const std::string& name);
RE::EnchantmentItem* GetEnchantmentFromExtraLists(RE::BSSimpleList<RE::ExtraDataList*>* extraLists);
ResourceType ResourceTypeByName(const std::string& name);

inline bool IsValueWeightExempt(ObjectType objectType)
{
	return objectType == ObjectType::critter ||
		objectType == ObjectType::flora ||
		objectType == ObjectType::lockpick ||
		objectType == ObjectType::key ||
		objectType == ObjectType::oreVein ||
		objectType == ObjectType::septims;
}

inline bool AlwaysValueWeightExempt(ObjectType objectType)
{
	return objectType == ObjectType::critter ||
		objectType == ObjectType::flora ||
		objectType == ObjectType::key ||
		objectType == ObjectType::oreVein ||
		objectType == ObjectType::septims;
}

bool FormTypeIsLootableObject(const RE::FormType formType);
bool NPCIsLeveled(const RE::TESNPC* npc);
bool FormIsLeveledNPC(const RE::TESForm* form);

}