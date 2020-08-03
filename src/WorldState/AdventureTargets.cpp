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
#include "WorldState/AdventureTargets.h"
#include "Utilities/utils.h"

std::string AdventureTargetName(const AdventureTargetType adventureTarget)
{
	switch (adventureTarget)
	{
		case AdventureTargetType::AnimalDen:	   return "Animal Den";
		case AdventureTargetType::AshSpawnLair:	   return "Ash Spawn Lair";
		case AdventureTargetType::AyleidRuin:	   return "Ayleid Ruin";
		case AdventureTargetType::BanditCamp:	   return "Bandit Camp";
		case AdventureTargetType::Cave:			   return "Cave";
		case AdventureTargetType::Clearable:	   return "Clearable";
		case AdventureTargetType::DragonLair:	   return "Dragon Lair";
		case AdventureTargetType::DragonPriestLair: return "Dragon Priest Lair";
		case AdventureTargetType::DraugrCrypt:	   return "Draugr Crypt";
		case AdventureTargetType::DwarvenRuin:	   return "Dwarven Ruin";
		case AdventureTargetType::Dungeon:		   return "Dungeon";
		case AdventureTargetType::FalmerHive:	   return "Falmer Hive";
		case AdventureTargetType::ForswornCamp:	   return "Forsworn Camp";
		case AdventureTargetType::Fort:			   return "Fort";
		case AdventureTargetType::GiantCamp:	   return "Giant Camp";
		case AdventureTargetType::GoblinDen:	   return "Goblin Den";
		case AdventureTargetType::Grove:		   return "Grove";
		case AdventureTargetType::HagravenNest:	   return "Hagraven Nest";
		case AdventureTargetType::Mine:			   return "Mine";
		case AdventureTargetType::NordicRuin:	   return "Nordic Ruin";
		case AdventureTargetType::OgreDen:		   return "Ogre Den";
		case AdventureTargetType::OrcStronghold:   return "Orc Stronghold";
		case AdventureTargetType::RieklingCamp:	   return "Riekling Camp";
		case AdventureTargetType::RuinedFort:	   return "Ruined Fort";
		case AdventureTargetType::Settlement:	   return "Settlement";
		case AdventureTargetType::Shipwreck:	   return "Shipwreck";
		case AdventureTargetType::VampireLair:	   return "Vampire Lair";
		case AdventureTargetType::WerebeastLair:   return "Werebeast Lair";
		case AdventureTargetType::WarlockLair:	   return "Warlock Lair";
		case AdventureTargetType::WitchmanLair:	   return "Witchman Lair";
		default: return "";
	};
}

std::unique_ptr<AdventureTargets> AdventureTargets::m_instance;

AdventureTargets& AdventureTargets::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<AdventureTargets>();
	}
	return *m_instance;
}

// Classify Locations by their keywords
void AdventureTargets::Categorize()
{
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	if (!dhnd)
		return;

	std::unordered_map<std::string, AdventureTargetType> targetByKeyword =
	{
		// Skyrim.esm
		{"LocSetCave", AdventureTargetType::Cave},
		{"LocSetCaveIce", AdventureTargetType::Cave},
		{"LocSetDwarvenRuin", AdventureTargetType::DwarvenRuin},
		{"LocSetMilitaryFort", AdventureTargetType::Fort},
		{"LocSetNordicRuin", AdventureTargetType::NordicRuin},
		{"LocTypeAnimalDen", AdventureTargetType::AnimalDen},
		{"LocTypeBanditCamp", AdventureTargetType::BanditCamp},
		{"LocTypeClearable", AdventureTargetType::Clearable},
		{"LocTypeDragonLair", AdventureTargetType::DragonLair},
		{"LocTypeDragonPriestLair", AdventureTargetType::DragonPriestLair},
		{"LocTypeDraugrCrypt", AdventureTargetType::DraugrCrypt},
		{"LocTypeDungeon", AdventureTargetType::Dungeon},
		{"LocTypeFalmerHive", AdventureTargetType::FalmerHive},
		{"LocTypeForswornCamp", AdventureTargetType::ForswornCamp},
		{"LocTypeGiantCamp", AdventureTargetType::GiantCamp},
		{"LocTypeHagravenNest", AdventureTargetType::HagravenNest},
		{"LocTypeMilitaryFort", AdventureTargetType::Fort},
		{"LocTypeMine", AdventureTargetType::Mine},
		{"LocTypeOrcStronghold", AdventureTargetType::OrcStronghold},
		{"LocTypeSettlement", AdventureTargetType::Settlement},
		{"LocTypeShipwreck", AdventureTargetType::Shipwreck},
		{"LocTypeSprigganGrove", AdventureTargetType::Grove},
		{"LocTypeVampireLair", AdventureTargetType::VampireLair},
		{"LocTypeWarlockLair", AdventureTargetType::WarlockLair},
		{"LocTypeWerewolfLair", AdventureTargetType::WerebeastLair},
		// Dragonborn.esm
		{"DLC2LocTypeAshSpawn", AdventureTargetType::AshSpawnLair},
		{"DLC2LocTypeRieklingCamp", AdventureTargetType::RieklingCamp},
		{"LocTypeWerebearLair", AdventureTargetType::WerebeastLair},
		// arnima.esm
		{"LocTypeArnimaOrcs", AdventureTargetType::OrcStronghold},
		{"LocTypeDirenniRuin", AdventureTargetType::AyleidRuin},
		{"LocTypeWitchmanLair", AdventureTargetType::WitchmanLair},
		// BSHeartland.esm
		{"CYRLocTypeAyleidRuin", AdventureTargetType::AyleidRuin},
		{"CYRLocTypeDaedra", AdventureTargetType::Grove},
		{"CYRLocTypeFortRuin", AdventureTargetType::RuinedFort},
		{"CYRLocTypeGoblinDen", AdventureTargetType::GoblinDen},
		{"CYRLocTypeOgreDen", AdventureTargetType::OgreDen},
		{"CYRLocTypeUndead", AdventureTargetType::WarlockLair},
		// Falskaar.esm
		{"FSLocTypeBanditCamp", AdventureTargetType::BanditCamp},
		{"FSLocTypeDungeon", AdventureTargetType::Dungeon},
		{"FSLocTypeFalmerHive", AdventureTargetType::FalmerHive},
		{"FSLocTypeGiantCamp", AdventureTargetType::GiantCamp},
		{"FSLocTypeSprigganGrove", AdventureTargetType::Grove},
		// Midwood Isle.esp
		{"LocSetAyleidRuinMidwoodIsle", AdventureTargetType::AyleidRuin},
		// Vigilant.esm
		{"zzzBMLocVampireDungeon", AdventureTargetType::VampireLair},
	};

	for (RE::BGSLocation* location : dhnd->GetFormArray<RE::BGSLocation>())
	{
		if (!location->GetFullNameLength())
		{
			DBG_MESSAGE("SKip unnamed Location0x{:08x}", location->GetFormID());
			continue;
		}
		// Scan location keywords to check if it's a settlement
		uint32_t numKeywords(location->GetNumKeywords());
		for (uint32_t next = 0; next < numKeywords; ++next)
		{
			std::optional<RE::BGSKeyword*> keyword(location->GetKeywordAt(next));
			if (!keyword.has_value() || !keyword.value())
				continue;

			std::string keywordName(FormUtils::SafeGetFormEditorID(keyword.value()));
			const auto matched(targetByKeyword.find(keywordName));
			if (matched == targetByKeyword.cend())
				continue;
			m_locationsByType[int(matched->second)].insert(location);
		}
	}
#if _DEBUG
	AdventureTargetType adventureType(static_cast<AdventureTargetType>(0));
	for (const auto locationsForType : m_locationsByType)
	{
		if (locationsForType.empty())
		{
			DBG_WARNING("No Adventure Targets for type {}", AdventureTargetName(adventureType));
		}
		else
		{
			DBG_MESSAGE("Adventure Targets for type {}", AdventureTargetName(adventureType));
			for (RE::BGSLocation* location : locationsForType)
			{
				DBG_MESSAGE("{}/0x{:08x}", location->GetName(), location->GetFormID());
			}
		}
		adventureType = static_cast<AdventureTargetType>(uint32_t(adventureType) + 1);
	}
#endif
	// skip this one
	const RE::FormID AvoidanceExterior = 0x1b44a;
	// hard code this linkage, it's a mess otherwise
	const RE::FormID TamrielLocation = 0x130ff;
	const RE::FormID SkyrimWorldspace = 0x3C;
	// determine Worldspace for each Location - first scan Worldspace Location lists
	for (RE::TESWorldSpace* world : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESWorldSpace>())
	{
		if (world->GetFormID() == AvoidanceExterior || world->GetFullNameLength() == 0)
			continue;
		if (world->GetFormID() == SkyrimWorldspace)
		{
			RE::BGSLocation* tamriel(RE::TESForm::LookupByID<RE::BGSLocation>(TamrielLocation));
			if (tamriel)
			{
				const auto inserted(m_worldByLocation.insert({ tamriel, world }));
				if (inserted.second)
				{
					DBG_MESSAGE("Hard-coded worldspace {}/0x{:08x} for location {}/0x{:08x}",
						world->GetName(), world->GetFormID(), tamriel->GetName(), tamriel->GetFormID());
				}
			}
		}

		for (const auto location : world->locationMap)
		{
			// parent world has locations, OK to proceed
			const auto inserted(m_worldByLocation.insert({ location.second, world }));
			if (inserted.second)
			{
				DBG_MESSAGE("Found worldspace {}/0x{:08x} for location {}/0x{:08x}",
					world->GetName(), world->GetFormID(), location.second->GetName(), location.second->GetFormID());
			}
			else if (inserted.first->second != world)
			{
				DBG_WARNING("Found second worldspace {}/0x{:08x} for location {}/0x{:08x}",
					world->GetName(), world->GetFormID(), location.second->GetName(), location.second->GetFormID());
			}
		}
	}
	// now scan full Location list and match up unprocessed Location with the WorldSpace of its parents
	for (const RE::BGSLocation* target : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::BGSLocation>())
	{
		const RE::BGSLocation* location(target);
		while (location)
		{
			const auto matched(m_worldByLocation.find(location));
			if (matched != m_worldByLocation.cend())
			{
				if (target != location)
				{
					DBG_MESSAGE("Found worldspace {}/0x{:08x} for parent of location {}/0x{:08x}",
						matched->second->GetName(), matched->second->GetFormID(), target->GetName(), target->GetFormID());
					m_worldByLocation.insert({ target, matched->second });
				}
				break;
			}
			location = location->parentLoc;
		}
	}
}
