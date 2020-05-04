#pragma once
#include "CommonLibSSE/include/RE/EnchantmentItem.h"
#include "CommonLibSSE/include/RE/TESObjectWEAP.h"

class TESObjectWEAPHelper
{
public:
	TESObjectWEAPHelper(RE::TESObjectWEAP* weapon) : m_weapon(weapon) {}
	SInt16 GetMaxCharge(void) const;
	UInt32 GetGoldValue(void) const;
	bool IsPlayable(void) { return m_weapon->GetPlayable(); }
private:
	RE::TESObjectWEAP* m_weapon;
};

double GetGameSettingFloat(const RE::BSFixedString& name);
