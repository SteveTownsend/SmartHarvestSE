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

#include "Looting/NPCFilter.h"
#include "Utilities/Exception.h"
#include "Utilities/utils.h"
#include "WorldState/PlayerState.h"

namespace shse
{

std::unique_ptr<NPCFilter> NPCFilter::m_instance;

NPCFilter& NPCFilter::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<NPCFilter>();
	}
	return *m_instance;
}

NPCFilter::NPCFilter() : m_active(false), m_logResults(false), m_defaultLoot(false), m_excludePlayerRace(false)
{
}

OrderedFilter::OrderedFilter(const nlohmann::json& j) : m_priority(0)
{
	// parse includes/excludes, if either are present we proceed to build a dead body RACE/KYWD filter
	std::unordered_set<std::string> excludeKeywords;
	std::unordered_set<std::string> excludeRaces;
	std::unordered_set<std::string> excludeFactions;
	std::unordered_set<std::string> includeKeywords;
	std::unordered_set<std::string> includeRaces;
	std::unordered_set<std::string> includeFactions;
	m_priority = j["priority"].get<unsigned int>();
	if (j.contains("exclude"))
	{
		const nlohmann::json& excluded(j["exclude"]);
		if (excluded.contains("race"))
		{
			const nlohmann::json& races(excluded["race"]);
			for (const std::string& next : races)
			{
				DBG_MESSAGE("NPC Race {} excluded", next);
				excludeRaces.insert(next);
			}
		}
		if (excluded.contains("faction"))
		{
			const nlohmann::json& factions(excluded["faction"]);
			for (const std::string& next : factions)
			{
				DBG_MESSAGE("NPC Faction {} excluded", next);
				excludeFactions.insert(next);
			}
		}
		if (excluded.contains("keyword"))
		{
			const nlohmann::json keywords(excluded["keyword"]);
			for (const std::string& next : keywords)
			{
				DBG_MESSAGE("NPC Keyword {} excluded", next);
				excludeKeywords.insert(next);
			}
		}
	}
	if (j.contains("include"))
	{
		const nlohmann::json included(j["include"]);
		if (included.contains("race"))
		{
			const nlohmann::json races(included["race"]);
			for (const std::string& next : races)
			{
				DBG_MESSAGE("NPC Race {} included", next);
				includeRaces.insert(next);
			}
		}
		if (included.contains("faction"))
		{
			const nlohmann::json& factions(included["faction"]);
			for (const std::string& next : factions)
			{
				DBG_MESSAGE("NPC Faction {} included", next);
				includeFactions.insert(next);
			}
		}
		if (included.contains("keyword"))
		{
			const nlohmann::json keywords(included["keyword"]);
			for (const std::string& next : keywords)
			{
				DBG_MESSAGE("NPC Keyword {} included", next);
				includeKeywords.insert(next);
			}
		}
	}

	for (const auto& includeRace : includeRaces)
	{
		if (excludeRaces.contains(includeRace))
		{
			REL_WARNING("NPC Race {} is in include and exclude list", includeRace);
		}
	}
	for (const auto& includeKeyword : includeKeywords)
	{
		if (excludeKeywords.contains(includeKeyword))
		{
			REL_WARNING("NPC Keyword {} is in include and exclude list", includeKeyword);
		}
	}
	// Bucket configured Keywords into fast filters
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	for (const RE::BGSKeyword* keyword : dhnd->GetFormArray<RE::BGSKeyword>())
	{
		std::string keywordName(FormUtils::SafeGetFormEditorID(keyword));
		if (keywordName.empty())
		{
			continue;
		}
		if (excludeKeywords.contains(keywordName))
		{
			m_excludeKeywords.insert(keyword);
		}
		else if (includeKeywords.contains(keywordName))
		{
			m_includeKeywords.insert(keyword);
		}
	}
	// Bucket configured Factions, Races and their Keywords into fast filters
	for (const RE::TESFaction* faction : dhnd->GetFormArray<RE::TESFaction>())
	{
		std::string factionName(FormUtils::SafeGetFormEditorID(faction));
		if (factionName.empty())
			continue;
		if (excludeFactions.contains(factionName))
		{
			REL_MESSAGE("Faction {}/0x{:08x} is excluded", factionName, faction->GetFormID());
			m_excludeFactions.insert(faction);
		}
		else if (includeFactions.contains(factionName))
		{
			REL_MESSAGE("Faction {}/0x{:08x} is included", factionName, faction->GetFormID());
			m_includeFactions.insert(faction);
		}
	}
	for (const RE::TESRace* race : dhnd->GetFormArray<RE::TESRace>())
	{
		std::string raceName(FormUtils::SafeGetFormEditorID(race));
		if (raceName.empty())
			continue;
		std::unordered_set<const RE::BGSKeyword*> keywords;
		for (std::uint32_t index = 0; index < race->GetNumKeywords(); ++index)
		{
			std::optional<RE::BGSKeyword*> keyword(race->GetKeywordAt(index));
			if (keyword.has_value())
			{
				std::string keywordName(FormUtils::SafeGetFormEditorID(keyword.value()));
				if (keywordName.empty())
				{
					continue;
				}
				if (excludeKeywords.contains(keywordName))
				{
					REL_MESSAGE("Race {}/0x{:08x} has excluded Keyword {}/0x{:08x}", raceName, race->GetFormID(),
						keywordName, keyword.value()->GetFormID());
					m_excludeKeywords.insert(keyword.value());
				}
				else if (includeKeywords.contains(keywordName))
				{
					REL_MESSAGE("Race {}/0x{:08x} has included Keyword {}/0x{:08x}", raceName, race->GetFormID(),
						keywordName, keyword.value()->GetFormID());
					m_includeKeywords.insert(keyword.value());
				}
			}
		}
		if (excludeRaces.contains(raceName))
		{
			REL_MESSAGE("Race {}/0x{:08x} is excluded", raceName, race->GetFormID());
			m_excludeRaces.insert(race);
		}
		else if (includeRaces.contains(raceName))
		{
			REL_MESSAGE("Race {}/0x{:08x} is included", raceName, race->GetFormID());
			m_includeRaces.insert(race);
		}
	}
}

bool OrderedFilter::DeterminesLootability(const RE::TESNPC* npc, bool& isLootable) const
{
	// NPC Race is checked present before entry
	const RE::TESRace* race(const_cast<RE::TESNPC*>(npc)->GetRace());
	if (m_excludeRaces.contains(race))
	{
		isLootable = false;
		return true;
	}
	// check Factions
	bool hasIncludeFaction(false);
	if (!m_excludeFactions.empty() || !m_excludeFactions.empty())
	{
		for (const auto& factionRank : npc->factions)
		{
			if (!factionRank.faction)
				continue;
			if (m_excludeFactions.contains(factionRank.faction))
			{
				// immediately dispositive
				isLootable = false;
				return true;
			}
			if (m_includeFactions.contains(factionRank.faction))
			{
				// delayed disposition, in case a later faction excludes this NPC
				hasIncludeFaction = true;
			}
		}
	}
	// check for exclude/include keywords, if filter uses them
	bool hasIncludeKeyword(false);
	if (!m_excludeKeywords.empty() || !m_includeKeywords.empty())
	{
		for (std::uint32_t index = 0; index < race->GetNumKeywords(); ++index)
		{
			std::optional<RE::BGSKeyword*> keyword(race->GetKeywordAt(index));
			if (keyword.has_value())
			{
				std::string keywordName(FormUtils::SafeGetFormEditorID(keyword.value()));
				if (keywordName.empty())
				{
					continue;
				}
				if (m_excludeKeywords.contains(keyword.value()))
				{
					// immediately dispositive
					isLootable = false;
					return true;
				}
				if (!hasIncludeKeyword && m_includeKeywords.contains(keyword.value()))
				{
					// delayed disposition, in case a later keyword excludes this NPC
					hasIncludeKeyword = true;
				}
			}
		}
	}

	if (hasIncludeFaction || hasIncludeKeyword || m_includeRaces.contains(race))
	{
		isLootable = true;
		return true;
	}
	// I could not make up my mind
	return false;
}

void NPCFilter::Load()
{
	try {
		// Validate the schema
		const std::string schemaFileName("SHSE.SchemaFilters.json");
		std::string filePath(FileUtils::GetPluginPath() + schemaFileName);
		nlohmann::json_schema::json_validator validator;
		try {
			std::ifstream schemaFile(filePath);
			if (schemaFile.fail()) {
				throw FileNotFound(filePath.c_str());
			}
			nlohmann::json schema(nlohmann::json::parse(schemaFile));
			validator.set_root_schema(schema); // insert root-schema
		}
		catch (const std::exception& e) {
			REL_ERROR("JSON Filters Schema {} not loadable, error:\n{}", filePath, e.what());
			return;
		}

		REL_MESSAGE("JSON Filters Schema {} parsed and validated", filePath.c_str());
		// check if dead body race filtering file is present
		const std::string filterFileName("SHSE.Filter.DeadBody.json");
		filePath = FileUtils::GetPluginPath() + filterFileName;
		std::ifstream filterFile(filePath);
		if (filterFile.fail()) {
			REL_MESSAGE("NPC AutoLoot Filtering not configured in {}", filterFileName);
			return;
		}

		const nlohmann::json filterData(nlohmann::json::parse(filterFile));
		const nlohmann::json filters(filterData["npc"]);
		m_defaultLoot = filters["defaultLoot"].get<bool>();
		DBG_MESSAGE("Default looting = {}", m_defaultLoot ? "true" : "false");

		m_excludePlayerRace = filters["excludePlayerRace"].get<bool>();
		DBG_MESSAGE("Exclude Player Race = {}", m_excludePlayerRace ? "true" : "false");

		for (const auto& nextFilter : filters["orderedFilter"])
		{
			m_orderedFilters.insert(std::move(new OrderedFilter(nextFilter)));
		}
		REL_MESSAGE("NPC AutoLoot Filtering JSON loaded OK from {} :\n{}", filePath, filterData.dump(2));
		m_active = true;
	}
	catch (const std::exception& exc) {
		REL_ERROR("NPC AutoLoot Filtering failed to initalize:\n{}", exc.what());
		return;
	}

	// pretest all NPCs
	m_logResults = true;
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	for (RE::TESNPC* npc : dhnd->GetFormArray<RE::TESNPC>())
	{
		IsLootable(npc);
	}
	m_logResults = false;
}

bool NPCFilter::IsLootable(const RE::TESNPC* npc) const
{
	if (!m_active)
		return true;
	const RE::TESRace* npcRace(const_cast<RE::TESNPC*>(npc)->GetRace());
	if (npcRace)
	{
		if (m_excludePlayerRace && npcRace == PlayerState::Instance().GetRace())
		{
			if (m_logResults)
			{ 
				REL_VMESSAGE("NPC {}/0x{:08x} matches Player Race {}/0x{:08x}, Not Lootable", npc->GetName(), npc->GetFormID(),
					FormUtils::SafeGetFormEditorID(npcRace), npcRace->GetFormID());
			}
			return false;
		}

		bool isLootable(false);
		for (const auto& orderedFilter : m_orderedFilters)
		{
			if (orderedFilter->DeterminesLootability(npc, isLootable))
			{
				if (m_logResults)
				{
					REL_VMESSAGE("NPC {}/0x{:08x} with Race {}/0x{:08x}, Lootable={} @ filter #{}", npc->GetName(), npc->GetFormID(),
						FormUtils::SafeGetFormEditorID(npcRace), npcRace->GetFormID(), isLootable ? "true" : "false", orderedFilter->Priority());
				}
				return isLootable;
			}
		}
	}
	if (m_logResults)
	{
		REL_VMESSAGE("NPC {}/0x{:08x} with Race {}/0x{:08x}, Lootable={} @ DefaultLoot", npc->GetName(), npc->GetFormID(),
			FormUtils::SafeGetFormEditorID(npcRace), npcRace->GetFormID(), m_defaultLoot ? "true" : "false");
	}
	return m_defaultLoot;
}

}
