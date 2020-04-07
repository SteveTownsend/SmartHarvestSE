#include "PrecompiledHeaders.h"

#include "iniSettings.h"
#include "utils.h"
#include "objects.h"
#include "dataCase.h"
#include "basketfile.h"
#include "containerLister.h"
#include "TESFormHelper.h"
#include <string>
#include <vector>
#include <unordered_map>
#include "ExtraDataListHelper.h"

#include "CommonLibSSE/include/RE/BGSProjectile.h"

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

TESObjectREFRHelper::TESObjectREFRHelper(const RE::TESObjectREFR* ref) : m_ref(ref), m_ingredient(nullptr)
{
	m_objectType = ClassifyType(m_ref);
	m_typeName = GetObjectTypeName(m_objectType);
}

double TESObjectREFRHelper::GetPosValue()
{
	if (!m_ref)
		return -1.0;
	double dx = m_ref->GetPositionX();
	double dy = m_ref->GetPositionY();
	double dz = m_ref->GetPositionZ();
	return (dx*dx + dy*dy + dz*dz);
}

bool TESObjectREFRHelper::IsQuestItem(const bool requireFullQuestFlags)
{
	if (!m_ref)
		return false;

	RE::RefHandle handle;
	RE::CreateRefHandle(handle, const_cast<RE::TESObjectREFR*>(m_ref));
	/* TODO CommonLibSSE logic unclear. Maybe does not matter as LookupReferenceByHandle will fail if invalid?
	if (handle == *(g_invalidRefHandle.GetPtr()))
		return false;
	*/

	RE::NiPointer<RE::TESObjectREFR> targetRef;
	RE::LookupReferenceByHandle(handle, targetRef);

	if (!targetRef)
		targetRef.reset(const_cast<RE::TESObjectREFR*>(m_ref));

	ExtraDataListHelper extraListEx(&targetRef->extraList);
	if (!extraListEx.m_extraData)
		return false;

	return extraListEx.IsQuestObject(requireFullQuestFlags);
}

std::vector<RE::TESObjectREFR*> TESObjectREFRHelper::GetLinkedRefs(RE::BGSKeyword* keyword)
{
	std::vector<RE::TESObjectREFR*> result;
	const RE::ExtraLinkedRef* exLinkRef(m_ref->extraList.GetByType<RE::ExtraLinkedRef>());
	if (!exLinkRef)
		return result;

	for (const auto pair : exLinkRef->linkedRefs)
	{
		if (!pair.refr || pair.keyword != keyword)
			continue;
		result.push_back(pair.refr);
	}
	return result;
}

RE::TESContainer* TESObjectREFRHelper::GetContainer() const
{
	if (!m_ref->data.objectReference)
		return nullptr;

	RE::TESContainer *container = nullptr;
	if (m_ref->data.objectReference->formType == RE::FormType::Container ||
	    m_ref->data.objectReference->formType == RE::FormType::NPC)
		container = skyrim_cast<RE::TESContainer*, RE::TESBoundObject>(m_ref->data.objectReference);

	return container;
}

bool TESObjectREFRHelper::IsPlayerOwned()
{
	const RE::TESForm* owner = m_ref->GetOwner();
	if (owner)
	{
		if (owner->formType == RE::FormType::NPC)
		{
			RE::TESNPC* npc = skyrim_cast<RE::TESNPC*, RE::TESForm>(owner);
			RE::TESNPC* playerBase = skyrim_cast<RE::TESNPC*, RE::TESForm>(RE::PlayerCharacter::GetSingleton()->data.objectReference);
			return (npc && npc == playerBase);
		}
		else if (owner->formType == RE::FormType::Faction)
		{
			RE::TESFaction* faction = skyrim_cast<RE::TESFaction*, RE::TESForm>(owner);
			if (faction)
			{
				return RE::PlayerCharacter::GetSingleton()->IsInFaction(faction);
			}
		}
	}
	return false;
}

const RE::IngredientItem* TESObjectREFRHelper::GetIngredient() const
{
	return m_ingredient;
}

void TESObjectREFRHelper::SetIngredient(const RE::IngredientItem* ingredient)
{
	m_ingredient = ingredient;
}

double TESObjectREFRHelper::GetWorth(void) const
{
	TESFormHelper itemEx(m_ingredient ? m_ingredient : m_ref->data.objectReference);
	return itemEx.GetWorth();
}

double TESObjectREFRHelper::GetWeight(void) const
{
	TESFormHelper itemEx(m_ingredient ? m_ingredient : m_ref->data.objectReference);
	return itemEx.GetWeight();
}

const char* TESObjectREFRHelper::GetName() const
{
	return m_ref->GetName();
}

UInt32 TESObjectREFRHelper::GetFormID() const
{
	return m_ref->data.objectReference->formID;
}

RE::TESObjectREFR* TESObjectREFRHelper::GetAshPileRefr()
{
	return GetAshPile(m_ref);
}

SInt16 TESObjectREFRHelper::GetItemCount()
{
	if (!m_ref)
		return 1;
	if (!m_ref->data.objectReference)
		return 1;
	if (m_ingredient)
		return 1;
	if (m_objectType == ObjectType::oreVein)
	{
		// limit ore harvesting to constrain Player Home mining
		return static_cast<SInt16>(INIFile::GetInstance()->GetInstance()->GetSetting(INIFile::autoharvest, INIFile::config, "maxMiningItems"));
	}

	const RE::ExtraCount* exCount(m_ref->extraList.GetByType<RE::ExtraCount>());
	return (exCount) ? exCount->count : 1;
}

RE::NiTimeController* TESObjectREFRHelper::GetTimeController()
{
	const RE::NiAVObject* node = m_ref->Get3D2();
	return (node && node->GetControllers()) ? node->GetControllers() : nullptr;
}

inline bool ActorHelper::IsSneaking()
{
	return (m_actor && m_actor->IsSneaking());
}

bool ActorHelper::IsPlayerFollower()
{
	if (!m_actor)
		return false;
	if (m_actor->IsPlayerTeammate())
		return true;
	static RE::TESFaction* faction = skyrim_cast<RE::TESFaction*, RE::TESForm>(RE::TESForm::LookupByID(CurrentFollowerFaction));
	return (faction) ? m_actor->IsInFaction(faction) : false;
}

bool ActorHelper::IsEssential()
{
	if (!m_actor)
		return false;
	RE::TESActorBaseData* baseData(skyrim_cast<RE::TESActorBaseData*, RE::Actor>(m_actor));
	if (!baseData)
		return false;
	return baseData->IsEssential();
}

bool ActorHelper::IsSummonable(void)
{
	if (!m_actor)
		return false;
	RE::TESActorBaseData* baseData(skyrim_cast<RE::TESActorBaseData*, RE::Actor>(m_actor));
	if (!baseData)
		return false;
	return baseData->IsSummonable();
}

ObjectType ClassifyType(const RE::TESObjectREFR* refr, bool ignoreUserlist)
{
	if (!refr || !refr->data.objectReference)
		return ObjectType::unknown;

	if (refr->formType == RE::FormType::ActorCharacter)
	{
		return ObjectType::actor;
	}
	else if (GetAshPile(refr) && refr->data.objectReference->formType == RE::FormType::Activator)
	{
		return ObjectType::unknown;
	}
	else if (refr->formType == RE::FormType::ProjectileArrow)
	{
		return ObjectType::ammo;
	}
	else if (refr->data.objectReference->formType == RE::FormType::SoulGem)
	{
		return ObjectType::soulgem;
	}

	return ClassifyType(refr->data.objectReference, ignoreUserlist);
}

ObjectType ClassifyType(const RE::TESForm* baseForm, bool ignoreUserlist)
{
	// Leveled items typically redirect to their contents
	baseForm = DataCase::GetInstance()->ConvertIfLeveledItem(baseForm);
	if (!baseForm)
		return ObjectType::unknown;

	if (!ignoreUserlist && BasketFile::GetSingleton()->IsinList(BasketFile::USERLIST, baseForm))
	{
		return ObjectType::userlist;
	}

	DataCase* data = DataCase::GetInstance();
	ObjectType objectType(data->GetObjectTypeForForm(baseForm));
	if (objectType != ObjectType::unknown)
	{
    	return objectType;
	}
	if (baseForm->formType == RE::FormType::Misc)
	{
		const RE::TESObjectMISC* miscObject = baseForm->As<RE::TESObjectMISC>();
		if (!miscObject)
			return ObjectType::unknown;

		return objectType;
	}
	else if (baseForm->formType == RE::FormType::Book)
	{
		const RE::TESObjectBOOK* book = baseForm->As<RE::TESObjectBOOK>();
		if (!book || !book->CanBeTaken())
			return ObjectType::unknown;
		bool isRead = book->IsRead();
		if (book->TeachesSkill())
			return (isRead) ? ObjectType::skillbookRead : ObjectType::skillbook;
		else if (book->TeachesSpell())
			return (isRead) ? ObjectType::spellbookRead : ObjectType::spellbook;
		return (isRead) ? ObjectType::booksRead : ObjectType::books;
	}
	else if (baseForm->formType == RE::FormType::Ammo || baseForm->formType == RE::FormType::Light)
	{
		if (baseForm->GetPlayable())
			return objectType;
	}
	else if (baseForm->formType == RE::FormType::Weapon)
	{
		const static RE::FormID Artifacts1 = 0x0A8668;
		const static RE::FormID Artifacts2 = 0x0917E8;

		const RE::TESObjectWEAP* weapon = baseForm->As<RE::TESObjectWEAP>();
		if (!weapon || !weapon->GetPlayable())
			return ObjectType::unknown;

		if (weapon->HasKeyword(Artifacts1) || weapon->HasKeyword(Artifacts2))
			return ObjectType::unknown;

		return (weapon->formEnchanting) ? ObjectType::enchantedWeapon : ObjectType::weapon;
	}
	else if (baseForm->formType == RE::FormType::Armor || objectType == ObjectType::jewelry)
	{
		const RE::TESObjectARMO* armor = baseForm->As<RE::TESObjectARMO>();
		if (!armor || !armor->GetPlayable())
			return ObjectType::unknown;

		if (objectType == ObjectType::jewelry)
			return (armor->formEnchanting) ? ObjectType::enchantedJewelry : ObjectType::jewelry;
		return (armor->formEnchanting) ? ObjectType::enchantedArmor : ObjectType::armor;
	}
	return ObjectType::unknown;
}

std::string GetObjectTypeName(SInt32 num)
{
	ObjectType objType = static_cast<ObjectType>(num);
	return GetObjectTypeName(objType);
}

std::string GetObjectTypeName(const RE::TESObjectREFR* refr)
{
	ObjectType objType = ClassifyType(refr);
	return GetObjectTypeName(objType);
}

std::string GetObjectTypeName(RE::TESForm* pForm)
{
	return GetObjectTypeName(static_cast<const RE::TESForm*>(pForm));
}

std::string GetObjectTypeName(const RE::TESForm* pForm)
{
	ObjectType objType = ClassifyType(pForm);
	return GetObjectTypeName(objType);
}

std::string GetObjectTypeName(ObjectType type)
{
	static const std::unordered_map<ObjectType, std::string> nameByType({
		{ObjectType::unknown, "unknown"},
		{ObjectType::flora, "flora"},
		// {ObjectType::fieldCrops, "fieldCrops"},
		{ObjectType::critter, "critter"},
		{ObjectType::ingredients, "ingredients"},
		{ObjectType::septims, "septims"},
		{ObjectType::gems, "gems"},
		{ObjectType::lockpick, "lockpick"},
		{ObjectType::animalHide, "animalHide"},
		{ObjectType::animalParts, "animalParts"},
		{ObjectType::oreIngot, "oreIngot"},
		{ObjectType::soulgem, "soulgem"},
		{ObjectType::keys, "keys"},
		{ObjectType::clutter, "clutter"},
		{ObjectType::clutterDwemer, "clutterDwemer"},
		{ObjectType::clutterBroken, "clutterBroken"},
		{ObjectType::light, "light"},
		{ObjectType::books, "books"},
		{ObjectType::spellbook, "spellbook"},
		{ObjectType::skillbook, "skillbook"},
		{ObjectType::booksRead, "booksRead"},
		{ObjectType::spellbookRead, "spellbookRead"},
		{ObjectType::skillbookRead, "skillbookRead"},
		{ObjectType::scrolls, "scrolls"},
		{ObjectType::ammo, "ammo"},
		{ObjectType::weapon, "weapon"},
		{ObjectType::enchantedWeapon, "enchantedWeapon"},
		{ObjectType::armor, "armor"},
		{ObjectType::enchantedArmor, "enchantedArmor"},
		{ObjectType::jewelry, "jewelry"},
		{ObjectType::enchantedJewelry, "enchantedJewelry"},
		{ObjectType::potion, "potion"},
		{ObjectType::poison, "poison"},
		{ObjectType::food, "food"},
		// {ObjectType::foodIngredients, "foodIngredients"},
		{ObjectType::drink, "drink"},
		{ObjectType::oreVein, "oreVein"},
		{ObjectType::userlist, "userlist"},
		{ObjectType::container, "container"},
		{ObjectType::actor, "actor"},
		{ObjectType::ashPile, "ashPile"}
		});
	const auto result(nameByType.find(type));
	if (result != nameByType.cend())
		return result->second;
	return "unknown";
}

