#pragma once

#include "skse64/PapyrusVM.h"
#include "skse64/GameReferences.h"
#include "RE/BSScript/IFunctionArguments.h"

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
	LootEventFunctor(TESObjectREFR* refr, SInt32 type, SInt32 count, bool silent, bool ignoreBlocking);
	virtual bool Copy(Output* dst);
private:
	TESObjectREFR* m_refr;
	SInt32 m_type;
	SInt32 m_count;
	bool m_silent;
	bool m_ignoreBlocking;
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

class ObjectGlowStopEventFunctor : public IFunctionArguments
{
public:
	ObjectGlowStopEventFunctor(TESObjectREFR* refr);
	virtual bool Copy(Output* dst);
private:
	TESObjectREFR* m_refr;
};

class PlayerHouseCheckEventFunctor : public IFunctionArguments
{
public:
	PlayerHouseCheckEventFunctor(TESForm* location);
	virtual bool Copy(Output* dst);
private:
	TESForm* m_location;
};

class CarryWeightDeltaEventFunctor : public IFunctionArguments
{
public:
	CarryWeightDeltaEventFunctor(const int delta);
	virtual bool Copy(Output* dst);
private:
	const int m_delta;
};
