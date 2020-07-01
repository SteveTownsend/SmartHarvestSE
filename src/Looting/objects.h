#pragma once

#include "FormHelpers/IHasValueWeight.h"
#include "Data/iniSettings.h"

inline bool IsBookObject(ObjectType objType)
{
	return objType >= ObjectType::book && objType <= ObjectType::skillbookRead;
}

class TESObjectREFRHelper : public IHasValueWeight
{
public:
	explicit TESObjectREFRHelper(const RE::TESObjectREFR* ref, const INIFile::SecondaryType scope);

	SInt16 GetItemCount();
	RE::NiTimeController* GetTimeController(void) const;
	bool IsQuestItem(const bool requireFullQuestFlags);
	double GetPosValue(void);
	const RE::TESContainer* GetContainer() const;
	std::vector<RE::TESObjectREFR*> GetLinkedRefs(RE::BGSKeyword* keyword);
	bool IsPlayerOwned(void) const;
	std::pair<bool, SpecialObjectHandling> TreatAsCollectible(void) const;
	bool IsValuable(void) const;

	RE::TESForm* GetLootable() const;
	void SetLootable(RE::TESForm* lootable);
	virtual double GetWeight(void) const override;
	inline const RE::TESObjectREFR* GetReference() const { return m_ref; }

protected:
	virtual const char* GetName() const;
	virtual UInt32 GetFormID() const;
	virtual double CalculateWorth(void) const override;

private:
	const RE::TESObjectREFR* m_ref;
	const INIFile::SecondaryType m_scope;
	RE::TESForm* m_lootable;
};

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
bool IsBossContainer(const RE::TESObjectREFR * refr);
bool IsContainerLocked(const RE::TESObjectREFR * refr);
ObjectType GetREFRObjectType(const RE::TESObjectREFR* refr, const INIFile::SecondaryType scope, bool ignoreWhiteList);
ObjectType GetBaseFormObjectType(const RE::TESForm* baseForm, const INIFile::SecondaryType scope, bool ignoreWhiteList);
// this overload deliberately has no definition, to trap at link-time misuse of the baseForm function to handle a REFR
ObjectType GetBaseFormObjectType(const RE::TESObjectREFR* refr, const INIFile::SecondaryType scope, bool ignoreWhiteList);
// end link-time misuse guard
inline bool IsAlwaysHarvested(ObjectType objectType) { return objectType == ObjectType::whitelist || objectType == ObjectType::collectible; }
std::string GetObjectTypeName(ObjectType objectType);
ObjectType GetObjectTypeByTypeName(const std::string& name);
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
