#pragma once
#include "CommonLibSSE/include/RE/EnchantmentItem.h"
#include "CommonLibSSE/include/RE/TESObjectARMO.h"

class TESObjectARMOHelper
{
public:
	TESObjectARMOHelper(RE::TESObjectARMO* armor) : m_armor(armor) {}
	UInt32 GetGoldValue(void) const;

private:
	RE::TESObjectARMO* m_armor;
};
