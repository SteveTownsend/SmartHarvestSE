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

#include "Data/iniSettings.h"

namespace shse
{

inline bool IsBookObject(ObjectType objType)
{
	return objType >= ObjectType::book && objType <= ObjectType::skillbookRead;
}

PlayerAffinity GetPlayerAffinity(const RE::Actor* actor);
bool IsSummoned(const RE::Actor* actor);
bool IsQuestTargetNPC(const RE::Actor* actor);

bool HasAshPile(const RE::TESObjectREFR* refr);
RE::TESObjectREFR* GetAshPile(const RE::TESObjectREFR* refr);
bool IsPlayerOwned(const RE::TESObjectREFR* refr);
void PrintManualLootMessage(const std::string& name);
void ProcessManualLootREFR(const RE::TESObjectREFR* refr);
void ProcessManualLootItem(const RE::TESBoundObject* item);
RE::NiTimeController* GetTimeController(RE::TESObjectREFR* refr);
bool IsBossContainer(const RE::TESObjectREFR * refr);
bool IsLocked(const RE::TESObjectREFR * refr);
ObjectType GetREFRObjectType(const RE::TESObjectREFR* refr);
ObjectType GetBaseFormObjectType(const RE::TESForm* baseForm);
// this overload deliberately has no definition, to trap at link-time misuse of the baseForm function to handle a REFR
ObjectType GetBaseFormObjectType(const RE::TESObjectREFR* refr);
// end link-time misuse guard
std::string GetFormTypeName(const RE::FormType formType);
std::string GetObjectTypeName(const ObjectType objectType);
ObjectType GetObjectTypeByTypeName(const std::string& name);
RE::EnchantmentItem* GetEnchantmentFromExtraLists(RE::BSSimpleList<RE::ExtraDataList*>* extraLists);
ResourceType ResourceTypeByName(const std::string& name);

inline bool IsHarvestable(RE::TESBoundObject* target, ObjectType objectType)
{
	DBG_VMESSAGE("check bound object {}/0x{:08x} with type {}", target->GetName(), target->GetFormID(), target->GetFormType());
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

}