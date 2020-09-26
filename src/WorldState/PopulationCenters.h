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
namespace shse
{

// Population Center Looting Size, overloaded to check Looting Permissions
enum class PopulationCenterSize {
	None = 0,
	Settlements,
	Towns,		// implies Settlements
	Cities,		// implies Towns and Settlements
	MAX
};

inline std::string PopulationCenterSizeName(const PopulationCenterSize centerSize)
{
	switch (centerSize) {
	case PopulationCenterSize::None:
		return "None";
	case PopulationCenterSize::Settlements:
		return "Settlement";
	case PopulationCenterSize::Towns:
		return "Town";
	case PopulationCenterSize::Cities:
		return "Cities";
	default:
		return "Unknown";
	}
}

class PopulationCenters
{
public:
	static PopulationCenters& Instance();
	PopulationCenters() : m_excludedCenterSize(PopulationCenterSize::None) {}

	void Categorize(void);
	void RefreshConfig(void);
	bool CannotLoot(const RE::FormID cellID, const RE::BGSLocation* location) const;

private:
	PopulationCenterSize PopulationCenterSizeFromIniSetting(const double iniSetting) const;
	void AddOtherPlaces(void);

	static std::unique_ptr<PopulationCenters> m_instance;

	PopulationCenterSize m_excludedCenterSize;
	std::unordered_map<const RE::BGSLocation*, PopulationCenterSize> m_centers;
	std::unordered_map<RE::FormID, PopulationCenterSize> m_cells;
	mutable RecursiveLock m_centersLock;
};

}