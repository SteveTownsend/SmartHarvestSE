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

namespace shse
{

class LootableREFR : public IHasValueWeight
{
public:
	explicit LootableREFR(const RE::TESObjectREFR* ref, const INIFile::SecondaryType scope);

	int16_t GetItemCount() const;
	std::pair<bool, CollectibleHandling> TreatAsCollectible(void) const;
	bool IsValuable(void) const;
	bool IsItemLootableInPopulationCenter(ObjectType objectType) const;
	void SetEffectiveObjectType(const ObjectType effectiveType);
	bool HasIngredient() const;
	bool IsHarvestable() const;
	bool IsCritter() const;
	bool HarvestForbiddenForForm() const;

	const RE::TESBoundObject* GetLootable() const;
	const RE::TESBoundObject* GetTarget() const;
	void SetLootable(const RE::TESBoundObject* lootable);
	virtual double GetWeight(void) const override;
	inline const RE::TESObjectREFR* GetReference() const { return m_ref; }
	inline INIFile::SecondaryType Scope() const { return m_scope; }

protected:
	virtual const char* GetName() const override;
	virtual uint32_t GetFormID() const override;
	virtual uint32_t CalculateWorth(void) const override;

	const RE::TESObjectREFR* m_ref;
	const INIFile::SecondaryType m_scope;
	const RE::TESBoundObject* m_lootable;
	bool m_critter;
	bool m_flora;
};

}