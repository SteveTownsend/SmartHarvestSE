#pragma once

namespace shse
{
	
class TESObjectWEAPHelper
{
public:
	TESObjectWEAPHelper(const RE::TESObjectWEAP* weapon) : m_weapon(weapon) {}
	SInt16 GetMaxCharge(void) const;
	UInt32 GetGoldValue(void) const;
	bool IsPlayable(void) const { return m_weapon->GetPlayable(); }
private:
	const RE::TESObjectWEAP* m_weapon;
};

double GetGameSettingFloat(const RE::BSFixedString& name);

}