#pragma once

namespace shse
{

class TESObjectARMOHelper
{
public:
	TESObjectARMOHelper(const RE::TESObjectARMO* armor) : m_armor(armor) {}
	UInt32 GetGoldValue(void) const;

private:
	const RE::TESObjectARMO* m_armor;
};

}