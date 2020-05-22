#pragma once

#include <vector>

inline bool IsBookObject(ObjectType objType)
{
	return objType >= ObjectType::books && objType <= ObjectType::skillbookRead;
}

class TESObjectREFRHelper : public IHasValueWeight
{
public:
	explicit TESObjectREFRHelper(const RE::TESObjectREFR* ref);

	SInt16 GetItemCount();
	RE::NiTimeController* GetTimeController(void);
	bool IsQuestItem(const bool requireFullQuestFlags);
	double GetPosValue(void);
	RE::TESContainer * GetContainer() const;
	std::vector<RE::TESObjectREFR*> GetLinkedRefs(RE::BGSKeyword* keyword);
	bool IsPlayerOwned(void);

	RE::TESForm* GetLootable() const;
	void SetLootable(RE::TESForm* lootable);
	virtual double GetWeight(void) const override;
	virtual double GetWorth(void) const override;
	inline const RE::TESObjectREFR* GetReference() const { return m_ref; }

protected:
	virtual const char* GetName() const;
	virtual UInt32 GetFormID() const;

private:
	const RE::TESObjectREFR* m_ref;
	RE::TESForm* m_lootable;
};

class ActorHelper
{
public:
	ActorHelper(RE::Actor* actor) : m_actor(actor) {}
	bool IsSneaking(void);
	bool IsPlayerAlly(void);
	bool IsEssential(void);
	bool IsSummonable(void);
private:
	RE::Actor* m_actor;
};

bool HasAshPile(const RE::TESObjectREFR* refr);
RE::TESObjectREFR* GetAshPile(const RE::TESObjectREFR* refr);
bool IsBossContainer(const RE::TESObjectREFR * refr);
bool IsContainerLocked(const RE::TESObjectREFR * refr);
ObjectType ClassifyType(const RE::TESObjectREFR* refr, bool ignoreUserlist = false);
ObjectType ClassifyType(RE::TESForm* baseForm, bool ignoreUserlist = false);
std::string GetObjectTypeName(SInt32 num);
std::string GetObjectTypeName(const RE::TESObjectREFR* refr);
std::string GetObjectTypeName(RE::TESForm* pForm);
std::string GetObjectTypeName(ObjectType type);
ObjectType GetObjectTypeByTypeName(const std::string& name);

inline bool ValueWeightExempt(ObjectType objectType)
{
	return objectType == ObjectType::ammo ||
		objectType == ObjectType::lockpick ||
		objectType == ObjectType::keys ||
		objectType == ObjectType::oreVein ||
		objectType == ObjectType::septims;
}
bool IsPlayable(const RE::TESForm * pForm);
