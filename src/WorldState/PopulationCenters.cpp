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
#include "PrecompiledHeaders.h"
#include "WorldState/PopulationCenters.h"
#include "Data/dataCase.h"
#include "Data/iniSettings.h"
#include "Utilities/utils.h"

namespace shse
{

std::unique_ptr<PopulationCenters> PopulationCenters::m_instance;

PopulationCenters& PopulationCenters::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<PopulationCenters>();
	}
	return *m_instance;
}

bool PopulationCenters::CannotLoot(const RE::FormID cellID, const RE::BGSLocation* location) const
{
	RecursiveLockGuard guard(m_centersLock);
	PopulationCenterSize centerSize(SettingsCache::Instance().PreventPopulationCenterLooting());
	if (centerSize == PopulationCenterSize::None)
		return false;

	const auto locationRecord(m_centers.find(location));
	// if small locations are excluded we automatically exclude any larger, so use >= here, assuming this is
	// a population center
	if (locationRecord != m_centers.cend())
	{
		return locationRecord->second >= centerSize;
	}
	const auto cellRecord(m_cells.find(cellID));
	if (cellRecord != m_cells.cend())
	{
		return cellRecord->second >= centerSize;
	}
	return false;
}

PopulationCenterSize PopulationCenterSizeFromIniSetting(const double iniSetting)
{
	uint32_t intSetting(static_cast<uint32_t>(iniSetting));
	if (intSetting >= static_cast<int32_t>(PopulationCenterSize::MAX))
	{
		return PopulationCenterSize::Cities;
	}
	return static_cast<PopulationCenterSize>(intSetting);
}

// Classify items by their keywords
void PopulationCenters::Categorize(void)
{
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	if (!dhnd)
		return;

	std::unordered_map<std::string, PopulationCenterSize> sizeByKeyword =
	{
		// Skyrim core
		{"LocTypeSettlement", PopulationCenterSize::Settlements},
		{"LocTypeTown", PopulationCenterSize::Towns},
		{"LocTypeCity", PopulationCenterSize::Cities}
	};

	for (RE::BGSLocation* location : dhnd->GetFormArray<RE::BGSLocation>())
	{
		if (std::string(location->GetName()).empty())
			continue;
		// Scan location keywords to check if it's a settlement
		uint32_t numKeywords(location->GetNumKeywords());
		PopulationCenterSize size(PopulationCenterSize::None);
		std::string largestMatch;
		for (uint32_t next = 0; next < numKeywords; ++next)
		{
			std::optional<RE::BGSKeyword*> keyword(location->GetKeywordAt(next));
			if (!keyword.has_value() || !keyword.value())
				continue;

			std::string keywordName(FormUtils::SafeGetFormEditorID(keyword.value()));
			const auto matched(sizeByKeyword.find(keywordName));
			if (matched == sizeByKeyword.cend())
				continue;
			if (matched->second > size)
			{
				size = matched->second;
				largestMatch = keywordName;
			}
		}
		// record population center size in case looting is selectively prevented
		if (size != PopulationCenterSize::None)
		{
			REL_MESSAGE("{}/0x{:08x} is population center of type {}", location->GetName(), location->GetFormID(), largestMatch);
			m_centers.insert(std::make_pair(location, size));
		}
		else
		{
			DBG_MESSAGE("{}/0x{:08x} is not a population center", location->GetName(), location->GetFormID());
		}
	}

	// We also categorize descendants of population centers. Not all will follow the same rule as the parent. For example,
	// preventing looting in Whiterun should also prevent looting in the Bannered Mare, but not in Whiterun Sewers. Use
	// child location keywords to control this.
	std::unordered_set<std::string> lootableChildLocations =
	{
		// not all Skyrim core, necessarily
		"LocTypeClearable",
		"LocTypeDungeon",
		"LocTypeDraugrCrypt",
		"LocTypeNordicRuin",
		"LocTypeSprigganGrove",
		"zzzBMLocVampireDungeon"
	};
#if _DEBUG
	std::unordered_set<std::string> childKeywords;
#endif
	for (RE::BGSLocation* location : dhnd->GetFormArray<RE::BGSLocation>())
	{
		if (std::string(location->GetName()).empty())
			continue;
		// check if this is a descendant of a population center
		RE::BGSLocation* antecedent(location->parentLoc);
		PopulationCenterSize parentSize(PopulationCenterSize::None);
		while (antecedent != nullptr)
		{
			const auto matched(m_centers.find(antecedent));
			if (matched != m_centers.cend())
			{
				parentSize = matched->second;
				DBG_MESSAGE("{}/0x{:08x} is a descendant of population center {}/0x{:08x} with size {}", location->GetName(), location->GetFormID(),
					antecedent->GetName(), antecedent->GetFormID(), PopulationCenterSizeName(parentSize));
				break;
			}
			antecedent = antecedent->parentLoc;
		}

		if (!antecedent)
			continue;

		// Scan location keywords to determine if lootable, or bucketed with its population center antecedent
		uint32_t numKeywords(location->GetNumKeywords());
		bool allowLooting(false);
		for (uint32_t next = 0; !allowLooting && next < numKeywords; ++next)
		{
			std::optional<RE::BGSKeyword*> keyword(location->GetKeywordAt(next));
			if (!keyword.has_value() || !keyword.value())
				continue;

			std::string keywordName(keyword.value()->GetFormEditorID());
#if _DEBUG
			childKeywords.insert(keywordName);
#endif
			if (lootableChildLocations.find(keywordName) != lootableChildLocations.cend())
			{
				allowLooting = true;
				REL_MESSAGE("{}/0x{:08x} is lootable child location due to keyword {}", location->GetName(), location->GetFormID(), keywordName);
				break;
			}
		}
		if (allowLooting)
			continue;

		// Store the child location with the same criterion as parent, unless it's inherently lootable
		// e.g. dungeon within the city limits like Whiterun Sewers, parts of the Ratway
		REL_MESSAGE("{}/0x{:08x} stored with same rule {} as its parent population center", location->GetName(), location->GetFormID(),
			PopulationCenterSizeName(parentSize));
		m_centers.insert(std::make_pair(location, parentSize));
	}
#if _DEBUG
	// this debug output from a given load order drives the list of 'really lootable' child location types above
	for (const std::string& keyword : childKeywords)
	{
		DBG_MESSAGE("Population center child keyword: {}", keyword.c_str());
	}
#endif
	AddOtherPlaces();
}

void PopulationCenters::AddOtherPlaces(void)
{
	size_t baseline(0);
	// Helgen Reborn house CELLs
	std::string espName("Helgen Reborn.esp");
	std::vector<RE::FormID> helgenRebornHouses = {
		0x31f4c,			// aaaBalokHouse04
		0x31eac,			// aaaBalokHouse05
		0x7fd96,			// aaaBalokHouseHill03
		0x320dd,			// aaaBalokHouse01
		0x31d3b,			// aaaBalokHelgenInn
		0x1226d7,			// aaaBalokHamingsHouse
		0x7fe11,			// aaaBalokHouseHill04
		0x7f1d8,			// aaaBalokHouseHill01
		0x31731,			// aaaBalokBrothel
		0x7fe8c,			// aaaBalokHouseHill05
		0x60154,			// aaaBalokReinhardtInterior
		0x7fd1b,			// aaaBalokHouseHill02
		0x31f41,			// aaaBalokHouse03
		0x32059				// aaaBalokHouse02
	};
	for (const RE::FormID formID : helgenRebornHouses)
	{
		const RE::TESObjectCELL* cell(DataCase::GetInstance()->FindExactMatch<RE::TESObjectCELL>(espName, formID));
		if (cell)
		{
			REL_MESSAGE("Cell {}/0x{:08x} treated as Town", cell->GetName(), cell->GetFormID());
			m_cells.insert(std::make_pair(cell->GetFormID(), PopulationCenterSize::Towns));
		}
	}
	if (m_cells.size() > baseline)
	{
		REL_VMESSAGE("Added {} Helgen Reborn CELLs as Town, expected {}", m_cells.size() - baseline, helgenRebornHouses.size());
	}

	baseline = m_cells.size();
	// Helgen Reborn house CELLs
	espName = "LC_BuildYourNobleHouse.esp";
	std::vector<RE::FormID> LC_BuildYourNobleHouseHouses = {
		0x2baad,			// LCBYHunterHouse
		0x180d00,			// LCBYChickenHouse
		0x2bd43,			// LCBYLumberjackHouse
		0x1121d,			// LCByMaraChurch
		0x17674d,			// LCBYTavern01
		0x2b935,			// LCBYHJamirHouse
		0x11054,			// LCBYClayMineHouse
		0x2b6f4,			// LCBYCarpiteria
		0x10fc0,			// LCBYHouseIgnar
		0x2beb0,			// LCBYFarmerHouse
		0x17ba9b			// LCBYalchimist
	};
	for (const RE::FormID formID : LC_BuildYourNobleHouseHouses)
	{
		const RE::TESObjectCELL* cell(DataCase::GetInstance()->FindExactMatch<RE::TESObjectCELL>(espName, formID));
		if (cell)
		{
			REL_MESSAGE("LC_Build Your Noble House Cell {}/0x{:08x} treated as Settlement", cell->GetName(), cell->GetFormID());
			m_cells.insert(std::make_pair(cell->GetFormID(), PopulationCenterSize::Towns));
		}
	}
	if (m_cells.size() > baseline)
	{
		REL_VMESSAGE("Added {} LC_Build Your Noble House CELLs as Town, expected {}", m_cells.size() - baseline, LC_BuildYourNobleHouseHouses.size());
	}
}

}