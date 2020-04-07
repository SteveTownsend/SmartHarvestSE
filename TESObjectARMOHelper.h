#pragma once
#include "CommonLibSSE/include/RE/EnchantmentItem.h"
#include "CommonLibSSE/include/RE/TESObjectARMO.h"

class TESObjectARMOHelper
{
public:
	TESObjectARMOHelper(const RE::TESObjectARMO* armor) : m_armor(armor) {}
	UInt32 GetGoldValue(void) const;

private:
	const RE::TESObjectARMO* m_armor;
};
