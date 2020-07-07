#include "PrecompiledHeaders.h"

#include "Data/dataCase.h"
#include "Looting/objects.h"
#include "Utilities/utils.h"
#include "Looting/containerLister.h"
#include "FormHelpers/FormHelper.h"
#include "FormHelpers/ExtraDataListHelper.h"
#include "Looting/ManagedLists.h"
#include "Collections/CollectionManager.h"

bool IsBossContainer(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return false;

	const RE::ExtraDataList* extraList = &refr->extraList;

	if (!extraList)
		return nullptr;

	const RE::ExtraLocationRefType* exLocRefType = extraList->GetByType<RE::ExtraLocationRefType>();
	return exLocRefType && exLocRefType->locRefType->formID == 0x0130F8;
}

// this logic is essential - TESObjectREFR::IsLocked() is not reliable
bool IsContainerLocked(const RE::TESObjectREFR* refr)
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
	if (!refr)
		return false;

	const RE::ExtraDataList* extraList = &refr->extraList;
	if (!extraList)
		return false;

	return extraList->HasType(RE::ExtraDataType::kAshPileRef);
}

// This is unsafe to call during combat - used only during deferred looting of Dead Bodies
RE::TESObjectREFR* GetAshPile(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return nullptr;

	const RE::ExtraDataList* extraList = &refr->extraList;
	if (!extraList)
		return nullptr;

	RE::ObjectRefHandle ashHandle = const_cast<RE::ExtraDataList*>(extraList)->GetAshPileRefHandle();
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

RE::NiTimeController* GetTimeController(RE::TESObjectREFR* refr)
{
	const RE::NiAVObject* node = refr->Get3D2();
	return (node && node->GetControllers()) ? node->GetControllers() : nullptr;
}

bool ActorHelper::IsSneaking() const
{
	return m_actor->IsSneaking();
}

bool ActorHelper::IsPlayerAlly() const
{
	if (m_actor->IsPlayerTeammate())
	{
		DBG_DMESSAGE("Actor is teammate");
		return true;
	}
	static const RE::TESFaction* followerFaction = RE::TESForm::LookupByID(CurrentFollowerFaction)->As<RE::TESFaction>();
	if (followerFaction)
	{
		bool result(m_actor->IsInFaction(followerFaction));
		DBG_DMESSAGE("Actor is follower = %s", result ? "true" : "false");
		return result;
	}
	return false;
}

bool ActorHelper::IsEssential() const 
{
	return m_actor->IsEssential();
}

// applies only if NPC
bool ActorHelper::IsSummoned(void) const
{
	const RE::TESNPC* npc(m_actor->GetActorBase());
	bool result(npc && npc->IsSummonable());
	DBG_DMESSAGE("Actor summoned = %s", result ? "true" : "false");
	return result;
}

// this is the pivotal function that maps a REFR to its loot category
ObjectType GetREFRObjectType(const RE::TESObjectREFR* refr)
{
	if (!refr || !refr->GetBaseObject())
		return ObjectType::unknown;

	if (refr->formType == RE::FormType::ActorCharacter)
		// derived from REFR directly
		return ObjectType::actor;

	if (refr->GetBaseObject()->formType == RE::FormType::Activator && HasAshPile(refr))
		return ObjectType::unknown;

	return GetBaseFormObjectType(refr->GetBaseObject());
}

ObjectType GetBaseFormObjectType(const RE::TESForm* baseForm)
{
	// Leveled items typically redirect to their contents
	DataCase* data = DataCase::GetInstance();
	baseForm = data->ConvertIfLeveledItem(baseForm);
	if (!baseForm)
		return ObjectType::unknown;

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
		if (!weapon || !weapon->GetPlayable())
			return ObjectType::unknown;

		return (weapon->formEnchanting) ? ObjectType::enchantedWeapon : ObjectType::weapon;
	}
	else if (baseForm->formType == RE::FormType::Armor)
	{
		// done here instead of during init because state of object can change during gameplay
		const RE::TESObjectARMO* armor = baseForm->As<RE::TESObjectARMO>();
		if (!armor || !armor->GetPlayable())
			return ObjectType::unknown;

		return (armor->formEnchanting) ? ObjectType::enchantedArmor : ObjectType::armor;
	}
	return ObjectType::unknown;
}

const std::unordered_map<ObjectType, std::string> nameByType({
	{ObjectType::unknown, "unknown"},
	{ObjectType::flora, "flora"},
	{ObjectType::critter, "critter"},
	{ObjectType::ingredient, "ingredient"},
	{ObjectType::septims, "septims"},
	{ObjectType::gem, "gem"},
	{ObjectType::lockpick, "lockpick"},
	{ObjectType::animalHide, "animalhide"},
	{ObjectType::oreIngot, "oreingot"},
	{ObjectType::soulgem, "soulgem"},
	{ObjectType::key, "key"},
	{ObjectType::clutter, "clutter"},
	{ObjectType::light, "light"},
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
	{ObjectType::actor, "actor"},
	{ObjectType::manualLoot, "manualloot"}
	});

std::string GetObjectTypeName(ObjectType objectType)
{
	const auto result(nameByType.find(objectType));
	if (result != nameByType.cend())
		return result->second;
	return "unknown";
}

ObjectType GetObjectTypeByTypeName(const std::string& name)
{
	// normalize from BSFixedString
	std::string lcName;
	std::transform(name.cbegin(), name.cend(), std::back_inserter(lcName), [](const char& c) { return std::tolower(c); });
	for (const auto& nextPair : nameByType)
	{
		if (nextPair.second == lcName)
		{
			DBG_VMESSAGE("Mapped name %s to ObjectType %d", name.c_str(), nextPair.first);
			return nextPair.first;
		}
	}
	DBG_WARNING("Unmapped ObjectType name %s", name.c_str());
	return ObjectType::unknown;
}

const std::unordered_map<std::string, ResourceType> resourceTypeByName({
	{"Ore", ResourceType::ore},
	{"Geode", ResourceType::geode},
	{"Volcanic", ResourceType::volcanic}
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
		ExtraDataListHelper exListHelper(*extraList);
		enchantment = exListHelper.GetEnchantment();
		if (enchantment)
			return enchantment;
	}
	return nullptr;
}
