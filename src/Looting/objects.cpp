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

#include "Data/dataCase.h"
#include "Looting/objects.h"
#include "Utilities/utils.h"
#include "Looting/containerLister.h"
#include "FormHelpers/FormHelper.h"
#include "FormHelpers/ExtraDataListHelper.h"
#include "Looting/ManagedLists.h"
#include "Collections/CollectionManager.h"
#include "WorldState/QuestTargets.h"

namespace shse
{

constexpr RE::FormID BossContainerLCRT = 0x0130F8;

bool IsBossContainer(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return false;

	const RE::ExtraDataList* extraList = &refr->extraList;

	if (!extraList)
		return false;

	const RE::ExtraLocationRefType* exLocRefType = extraList->GetByType<RE::ExtraLocationRefType>();
	return exLocRefType && exLocRefType->locRefType->formID == BossContainerLCRT;
}

// this logic is essential - TESObjectREFR::IsLocked() is not reliable
bool IsLocked(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return false;

	const RE::ExtraDataList* extraList = &refr->extraList;
	if (!extraList)
		return false;
		
	const RE::ExtraLock* exLock(extraList->GetByType<RE::ExtraLock>());
	return exLock && (exLock->lock->flags & RE::REFR_LOCK::Flag::kLocked) == RE::REFR_LOCK::Flag::kLocked;
}

// This appears safe to call during combat - does not introspect an in-flux ExtraDataList, just a bitfield
bool HasAshPile(const RE::TESObjectREFR* refr)
{
	return refr && refr->extraList.HasType(RE::ExtraDataType::kAshPileRef);
}

// This is unsafe to call during combat - used only during deferred looting of Dead Bodies
RE::TESObjectREFR* GetAshPile(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return nullptr;
	RE::ObjectRefHandle ashHandle = const_cast<RE::TESObjectREFR*>(refr)->extraList.GetAshPileRef();
	if (!ashHandle)
		return nullptr;
	return ashHandle.get().get();
}

bool IsPlayerOwned(const RE::TESObjectREFR* refr)
{
	const RE::TESForm* owner = refr->GetOwner();
	if (owner)
	{
		if (owner->formType == RE::FormType::NPC)
		{
			const RE::TESNPC* npc = owner->As<RE::TESNPC>();
			RE::TESNPC* playerBase = RE::PlayerCharacter::GetSingleton()->GetActorBase();
			return (npc && npc == playerBase);
		}
		else if (owner->formType == RE::FormType::Faction)
		{
			const RE::TESFaction* faction = owner->As<RE::TESFaction>();
			if (faction)
			{
				return RE::PlayerCharacter::GetSingleton()->IsInFaction(faction);
			}
		}
	}
	return false;
}

bool IsQuestItem(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return false;
	// check REFR vs pre-populated Quest Targets
	if (QuestTargets::Instance().ReferencedQuestTargetLootability(refr) == Lootability::CannotLootQuestTarget)
		return true;

	RE::RefHandle handle;
	RE::CreateRefHandle(handle, const_cast<RE::TESObjectREFR*>(refr));

	RE::NiPointer<RE::TESObjectREFR> targetRef;
	RE::LookupReferenceByHandle(handle, targetRef);

	if (!targetRef)
		targetRef.reset(const_cast<RE::TESObjectREFR*>(refr));

	return ExtraDataList::IsREFRQuestObject(targetRef.get(), &targetRef->extraList);
}

void PrintManualLootMessage(const std::string& name)
{
	static RE::BSFixedString manualLootText(DataCase::GetInstance()->GetTranslation("$SHSE_MANUAL_LOOT_MSG"));
	if (!manualLootText.empty())
	{
		std::string notificationText(manualLootText);
		StringUtils::Replace(notificationText, "{ITEMNAME}", name);
		if (!notificationText.empty())
		{
			RE::DebugNotification(notificationText.c_str());
		}
	}
}

void ProcessManualLootREFR(const RE::TESObjectREFR* refr)
{
	// notify about these, just once
	PrintManualLootMessage(refr->GetName());
	DBG_VMESSAGE("notify, then block objType == ObjectType::manualLoot for REFR 0x{:08x}", refr->GetFormID());
	// this may be a dynamic Form
	DataCase::GetInstance()->BlockReferenceByID(refr->GetFormID(), Lootability::ManualLootTarget);
}

void ProcessManualLootItem(const RE::TESBoundObject* item)
{
	// notify about these, just once
	PrintManualLootMessage(item->GetName());
	DBG_VMESSAGE("notify, then block objType == ObjectType::manualLoot for Item 0x{:08x}", item->GetFormID());
	DataCase::GetInstance()->BlockForm(item, Lootability::ManualLootTarget);
}

RE::NiTimeController* GetTimeController(RE::TESObjectREFR* refr)
{
	const RE::NiAVObject* node = refr->Get3D2();
	return (node && node->GetControllers()) ? node->GetControllers() : nullptr;
}

PlayerAffinity GetPlayerAffinity(const RE::Actor* actor)
{
	if (actor == RE::PlayerCharacter::GetSingleton())
		return PlayerAffinity::Player;
	static const RE::TESFaction* followerFaction = RE::TESForm::LookupByID(CurrentFollowerFaction)->As<RE::TESFaction>();
	if (followerFaction && actor->IsInFaction(followerFaction))
	{
		DBG_DMESSAGE("Actor {}/0x{:08x} is follower", actor->GetName(), actor->GetFormID());
		return PlayerAffinity::FollowerFaction;
	}
	if (actor->IsPlayerTeammate())
	{
		DBG_DMESSAGE("Actor {}/0x{:08x} is teammate", actor->GetName(), actor->GetFormID());
		return PlayerAffinity::TeamMate;
	}
	return PlayerAffinity::Unaffiliated;
}

// applies only if NPC
bool IsSummoned(const RE::Actor* actor)
{
	const RE::TESNPC* npc(actor->GetActorBase());
	bool result(npc && npc->IsSummonable());
	DBG_DMESSAGE("Actor summoned = {}", result);
	return result;
}

// applies only if NPC
bool StartsDead(const RE::TESObjectREFR* refr)
{
	bool result(refr && refr->loadedData && ((refr->loadedData->flags & RE::TESObjectREFR::RecordFlags::kStartsDead) == 0));
	DBG_DMESSAGE("Actor starts dead = {}", result);
	return result;
}

// applies only if NPC
bool IsDisintegrating(const RE::Actor* actor)
{
	if (actor)
	{
		return actor->GetActorRuntimeData().criticalStage == RE::ACTOR_CRITICAL_STAGE::kDisintegrateStart;
	}
	return false;
}

// applies only if NPC
bool IsQuestTargetNPC(const RE::Actor* actor)
{
	const RE::TESNPC* npc(actor->GetActorBase());
	bool result(npc && QuestTargets::Instance().QuestTargetLootability(npc, nullptr) == Lootability::CannotLootQuestTarget);
	DBG_DMESSAGE("Actor is Quest Target NPC = {}", result ? "true" : "false");
	return result;
}

ObjectType GetEffectiveObjectType(const RE::TESBoundObject* baseForm)
{
	// Leveled items typically redirect to their contents
	DataCase* data = DataCase::GetInstance();
	baseForm = data->ConvertIfLeveledItem(baseForm);
	if (!baseForm)
		return ObjectType::unknown;
	return GetBaseObjectType(baseForm);
}

ObjectType GetBaseObjectType(const RE::TESForm * baseForm)
{
	DataCase* data = DataCase::GetInstance();
	ObjectType objectType(data->GetObjectTypeForForm(baseForm));
	if (objectType != ObjectType::unknown)
	{
    	return objectType;
	}
	if (baseForm->formType == RE::FormType::Book)
	{
		// done here instead of during init because state of object can change during gameplay
		const RE::TESObjectBOOK* book = baseForm->As<RE::TESObjectBOOK>();
		if (!book || !book->CanBeTaken())
			return ObjectType::unknown;
		bool isRead = book->IsRead();
		if (book->TeachesSkill())
			return (isRead) ? ObjectType::skillbookRead : ObjectType::skillbook;
		else if (book->TeachesSpell())
			return (isRead) ? ObjectType::spellbookRead : ObjectType::spellbook;
		return (isRead) ? ObjectType::bookRead : ObjectType::book;
	}
	else if (baseForm->formType == RE::FormType::Weapon)
	{
		// done here instead of during init because state of object can change during gameplay
		const RE::TESObjectWEAP* weapon = baseForm->As<RE::TESObjectWEAP>();
		if (!FormUtils::IsConcrete(weapon))
			return ObjectType::unknown;

		return (weapon->formEnchanting) ? ObjectType::enchantedWeapon : ObjectType::weapon;
	}
	else if (baseForm->formType == RE::FormType::Armor)
	{
		// done here instead of during init because state of object can change during gameplay
		const RE::TESObjectARMO* armor = baseForm->As<RE::TESObjectARMO>();
		if (!FormUtils::IsConcrete(armor))
			return ObjectType::unknown;

		return (armor->formEnchanting) ? ObjectType::enchantedArmor : ObjectType::armor;
	}
	return ObjectType::unknown;
}

ObjectType GetExcessObjectType(const RE::TESBoundObject* baseForm)
{
	// Some types are conflated to simplify management of excess inventory
	ObjectType objType = GetEffectiveObjectType(baseForm);
	if (objType >= ObjectType::spellbook && objType <= ObjectType::skillbookRead)
		return ObjectType::book;
	if (TypeIsEnchanted(objType))
		return ConvertToUnenchanted(objType);
	return objType;
}

const std::unordered_map<RE::FormType, std::string> nameByFormType({
	{RE::FormType::ActorCharacter, "ActorCharacter"},
	{RE::FormType::Container, "Container"},
	{RE::FormType::Ingredient, "Ingredient"},
	{RE::FormType::SoulGem, "SoulGem"},
	{RE::FormType::KeyMaster, "KeyMaster"},
	{RE::FormType::Scroll, "Scroll"},
	{RE::FormType::Ammo, "Ammo"},
	{RE::FormType::ProjectileArrow, "ProjectileArrow"},
	{RE::FormType::Light, "Light"}
	});

std::string GetFormTypeName(const RE::FormType formType)
{
	const auto result(nameByFormType.find(formType));
	if (result != nameByFormType.cend())
		return result->second;
	return "unknown";
}

const std::unordered_map<ObjectType, std::string> nameByObjectType({
	{ObjectType::unknown, "unknown"},
	{ObjectType::flora, ObjTypeName::Flora},
	{ObjectType::critter, ObjTypeName::Critter},
	{ObjectType::ingredient, "ingredient"},
	{ObjectType::septims, "septims"},
	{ObjectType::gem, "gem"},
	{ObjectType::lockpick, "lockpick"},
	{ObjectType::animalHide, "animalhide"},
	{ObjectType::oreIngot, "oreingot"},
	{ObjectType::soulgem, "soulgem"},
	{ObjectType::key, "key"},
	{ObjectType::clutter, "clutter"},
	{ObjectType::book, "book"},
	{ObjectType::spellbook, "spellbook"},
	{ObjectType::skillbook, "skillbook"},
	{ObjectType::bookRead, "bookread"},
	{ObjectType::spellbookRead, "spellbookread"},
	{ObjectType::skillbookRead, "skillbookread"},
	{ObjectType::scroll, "scroll"},
	{ObjectType::ammo, "ammo"},
	{ObjectType::weapon, "weapon"},
	{ObjectType::enchantedWeapon, "enchantedweapon"},
	{ObjectType::armor, "armor"},
	{ObjectType::enchantedArmor, "enchantedarmor"},
	{ObjectType::jewelry, "jewelry"},
	{ObjectType::enchantedJewelry, "enchantedjewelry"},
	{ObjectType::potion, "potion"},
	{ObjectType::poison, "poison"},
	{ObjectType::food, "food"},
	{ObjectType::drink, "drink"},
	{ObjectType::oreVein, "orevein"},
	{ObjectType::container, "container"},
	{ObjectType::actor, "actor"}
	});

std::string GetObjectTypeName(const ObjectType objectType)
{
	const auto result(nameByObjectType.find(objectType));
	if (result != nameByObjectType.cend())
		return result->second;
	return "unknown";
}

ObjectType GetObjectTypeByTypeName(const std::string& name)
{
	// normalize from BSFixedString
	std::string lcName;
	std::transform(name.cbegin(), name.cend(), std::back_inserter(lcName), [](const char& c) { return static_cast<char>(std::tolower(c)); });
	for (const auto& nextPair : nameByObjectType)
	{
		if (nextPair.second == lcName)
		{
			DBG_VMESSAGE("Mapped name {} to ObjectType {}", name.c_str(), GetObjectTypeName(nextPair.first));
			return nextPair.first;
		}
	}
	DBG_WARNING("Unmapped ObjectType name {}", name.c_str());
	return ObjectType::unknown;
}

const std::unordered_map<std::string, ResourceType> resourceTypeByName({
	{"Ore", ResourceType::ore},
	{"Geode", ResourceType::geode},
	{"Volcanic", ResourceType::volcanic},
	{"VolcanicDigSite", ResourceType::volcanicDigSite}
	});

ResourceType ResourceTypeByName(const std::string& name)
{
	const auto matched(resourceTypeByName.find(name));
	if (matched != resourceTypeByName.cend())
	{
		return matched->second;
	}
	return ResourceType::ore;
}

RE::EnchantmentItem* GetEnchantmentFromExtraLists(RE::BSSimpleList<RE::ExtraDataList*>* extraLists)
{
	if (!extraLists)
		return nullptr;
	RE::EnchantmentItem* enchantment = nullptr;
	for (auto extraList = extraLists->begin(); extraList != extraLists->end(); ++extraList)
	{
		enchantment = ExtraDataList::GetEnchantment(*extraList);
		if (enchantment)
			return enchantment;
	}
	return nullptr;
}

bool FormTypeIsLootableObject(const RE::FormType formType) {
	static const std::vector<RE::FormType> lootableObjectFormTypes =
	{
		RE::FormType::AlchemyItem,
		RE::FormType::Ammo,
		RE::FormType::Armor,
		RE::FormType::Book,
		RE::FormType::Container,
		RE::FormType::Flora,
		RE::FormType::Ingredient,
		RE::FormType::KeyMaster,
		RE::FormType::Misc,
		RE::FormType::Note,
		RE::FormType::Projectile,
		RE::FormType::Scroll,
		RE::FormType::SoulGem,
		RE::FormType::Tree,
		RE::FormType::Weapon
	};

	return std::find(std::cbegin(lootableObjectFormTypes), std::cend(lootableObjectFormTypes), formType) != std::cend(lootableObjectFormTypes);
}

bool NPCIsLeveled(const RE::TESNPC* npc)
{
	const RE::TESForm* tplt(npc->baseTemplateForm);
	while (tplt)
	{
		if (tplt->As<RE::TESLevCharacter>())
			return true;
		if (!tplt->As<RE::TESNPC>())
			return true;
		tplt = tplt->As<RE::TESNPC>()->baseTemplateForm;
	}
	return false;
}

bool FormIsLeveledNPC(const RE::TESForm* form)
{
	const RE::TESNPC* npc(form->As<RE::TESNPC>());
	if (!npc)
		return false;
	return NPCIsLeveled(npc);
}

}