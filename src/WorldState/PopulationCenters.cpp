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
#include "Data/iniSettings.h"
#include "Utilities/utils.h"

std::unique_ptr<PopulationCenters> PopulationCenters::m_instance;

PopulationCenters& PopulationCenters::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<PopulationCenters>();
	}
	return *m_instance;
}

bool PopulationCenters::CannotLoot(const RE::BGSLocation* location) const
{
	PopulationCenterSize excludedCenterSize(PopulationCenterSizeFromIniSetting(
		INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "PreventPopulationCenterLooting")));
	if (excludedCenterSize == PopulationCenterSize::None)
		return false;

	RecursiveLockGuard guard(m_centersLock);
	const auto locationRecord(m_centers.find(location));
	// if small locations are excluded we automatically exclude any larger, so use >= here, assuming this is
	// a population center
	return locationRecord != m_centers.cend() && locationRecord->second >= excludedCenterSize;
}

PopulationCenterSize PopulationCenters::PopulationCenterSizeFromIniSetting(const double iniSetting) const
{
	uint32_t intSetting(static_cast<uint32_t>(iniSetting));
	if (intSetting >= static_cast<int32_t>(PopulationCenterSize::MAX))
	{
		return PopulationCenterSize::Cities;
	}
	return static_cast<PopulationCenterSize>(intSetting);
}

// Classify items by their keywords
void PopulationCenters::Categorize()
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
			DBG_MESSAGE("{}/0x{:08x} is population center of type {}", location->GetName(), location->GetFormID(), largestMatch.c_str());
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
		"zzzBMLocVampireDungeon"
	};
#if _DEBUG
	std::unordered_set<std::string> childKeywords;
#endif
	for (RE::BGSLocation* location : dhnd->GetFormArray<RE::BGSLocation>())
	{
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
					antecedent->GetName(), antecedent->GetFormID(), parentSize);
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
				DBG_MESSAGE("{}/0x{:08x} is lootable child location due to keyword {}", location->GetName(), location->GetFormID(), keywordName.c_str());
				break;
			}
		}
		if (allowLooting)
			continue;

		// Store the child location with the same criterion as parent, unless it's inherently lootable
		// e.g. dungeon within the city limits like Whiterun Sewers, parts of the Ratway
		DBG_MESSAGE("{}/0x{:08x} stored with same rule as its parent population center", location->GetName(), location->GetFormID());
		m_centers.insert(std::make_pair(location, parentSize));
	}
#if _DEBUG
	// this debug output from a given load order drives the list of 'really lootable' child location types above
	for (const std::string& keyword : childKeywords)
	{
		DBG_MESSAGE("Population center child keyword: {}", keyword.c_str());
	}
#endif
}
