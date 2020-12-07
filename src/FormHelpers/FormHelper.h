/*************************************************************************
SmartHarvest SE
Copyright (c) Steve Townsend 2020

>>> SOURCE LICENSE >>>
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation (www.fsf.org); either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License is available at
http://www.fsf.org/licensing/licenses
>>> END OF LICENSE >>>
*************************************************************************/
#pragma once

#include "FormHelpers/IHasValueWeight.h"
#include "Data/iniSettings.h"
#include "Collections/Condition.h"

namespace shse
{

class TESFormHelper : public IHasValueWeight
{
public:
	TESFormHelper(const RE::TESForm* form, const INIFile::SecondaryType scope);

	RE::BGSKeywordForm* GetKeywordForm(void) const;
	RE::EnchantmentItem* GetEnchantment(void);
	uint32_t GetGoldValue(void) const;
	std::pair<bool, CollectibleHandling> TreatAsCollectible(const bool recordDups) const;
	inline const RE::TESForm* Form() const { return m_form; }

	virtual double GetWeight(void) const override;

protected:
	const RE::TESForm* m_form;
	const shse::ConditionMatcher m_matcher;

	virtual const char* GetName() const override;
	virtual uint32_t GetFormID() const override;
	virtual uint32_t CalculateWorth(void) const override;
};

}