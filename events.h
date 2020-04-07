#pragma once

#include "skse64/PapyrusVM.h"
#include "skse64/GameForms.h"
#include "skse64/GameObjects.h"
#include "skse64/GameReferences.h"

class CritterIngredientEventFunctor : public IFunctionArguments
{
public:
	CritterIngredientEventFunctor(TESObjectREFR* refr);
	virtual bool Copy(Output* dst);
private:
	TESObjectREFR* m_refr;
};

class LootEventFunctor : public IFunctionArguments
{
public:
	LootEventFunctor(TESObjectREFR* refr, SInt32 type, SInt32 count, bool silent);
	virtual bool Copy(Output* dst);
private:
	TESObjectREFR* m_refr;
	SInt32 m_type;
	SInt32 m_count;
	bool m_silent;
};

class ContainerLootManyEventFunctor : public IFunctionArguments
{
public:
	ContainerLootManyEventFunctor(TESObjectREFR* refr, const SInt32 count, const bool animate);
	virtual bool Copy(Output* dst);
private:
	TESObjectREFR* m_refr;
	SInt32 m_count;
	bool m_animate;
};

class ObjectGlowEventFunctor : public IFunctionArguments
{
public:
	ObjectGlowEventFunctor(TESObjectREFR* refr, const int glowDuration);
	virtual bool Copy(Output* dst);
private:
	TESObjectREFR* m_refr;
	const int m_glowDuration;
};

class PlayerHouseCheckEventFunctor : public IFunctionArguments
{
public:
	PlayerHouseCheckEventFunctor(TESForm* location);
	virtual bool Copy(Output* dst);
private:
	TESForm* m_location;
};
