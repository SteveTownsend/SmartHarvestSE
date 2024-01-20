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
#include "Looting/objects.h"
#include "Utilities/Exception.h"
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
	std::unordered_set<std::string> includeKeywords;
	m_priority = j["priority"].get<unsigned int>();
	if (j.contains("exclude"))
	{
		const nlohmann::json& excluded(j["exclude"]);
		if (excluded.contains("race"))
		{
			m_excludeRaces = JSONUtils::ToForms<RE::TESRace>(excluded["race"]);
		}
		if (excluded.contains("faction"))
		{
			m_excludeFactions = JSONUtils::ToForms<RE::TESFaction>(excluded["faction"]);
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
			m_includeRaces = JSONUtils::ToForms<RE::TESRace>(included["race"]);
		}
		if (included.contains("faction"))
		{
			m_includeFactions = JSONUtils::ToForms<RE::TESFaction>(included["faction"]);
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

	for (const auto& includeRace : m_includeRaces)
	{
		if (m_excludeRaces.contains(includeRace))
		{
			REL_WARNING("NPC Race {}/0x{:08x} is in include and exclude list", includeRace->GetName(), includeRace->GetFormID());
		}
	}
	for (const auto& includeFaction : m_includeFactions)
	{
		if (m_excludeFactions.contains(includeFaction))
		{
			REL_WARNING("NPC Faction {}/0x{:08x} is in include and exclude list", includeFaction->GetName(), includeFaction->GetFormID());
		}
	}
	for (const auto& includeKeyword : m_includeKeywords)
	{
		if (m_excludeKeywords.contains(includeKeyword))
		{
			REL_WARNING("NPC Keyword {}/0x{:08x} is in include and exclude list", FormUtils::SafeGetFormEditorID(includeKeyword), includeKeyword->GetFormID());
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
		std::string filePath(StringUtils::FromUnicode(FileUtils::GetPluginPath()) + schemaFileName);
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
		filePath = StringUtils::FromUnicode(FileUtils::GetPluginPath()) + filterFileName;
		std::ifstream filterFile(filePath);
		if (filterFile.fail()) {
			REL_MESSAGE("NPC AutoLoot Filtering not configured in {}", filterFileName);
			return;
		}

		const nlohmann::json filterData(nlohmann::json::parse(filterFile));
		const nlohmann::json filters(filterData["npc"]);
		m_defaultLoot = filters["defaultLoot"].get<bool>();
		REL_MESSAGE("Default looting = {}", m_defaultLoot ? "true" : "false");

		m_excludePlayerRace = filters["excludePlayerRace"].get<bool>();
		REL_MESSAGE("Exclude Player Race = {}", m_excludePlayerRace ? "true" : "false");

		for (const auto& nextFilter : filters["orderedFilter"])
		{
			m_orderedFilters.insert(std::move(new OrderedFilter(nextFilter)));
		}
		REL_MESSAGE("NPC AutoLoot Filtering JSON loaded OK from {}", filePath);
		m_active = true;
	}
	catch (const std::exception& exc) {
		REL_ERROR("NPC AutoLoot Filtering failed to initalize:\n{}", exc.what());
		return;
	}

	// pretest all NPCs
	m_logResults = true;
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	for (const RE::TESNPC* npc : dhnd->GetFormArray<RE::TESNPC>())
	{
		// Unnamed NPCs are often Leveled at runtime, we won't loot such Records
		if (std::string(npc->GetName()).empty())
		{
			REL_VMESSAGE("Skip unnamed NPC {}/0x{:08x}", npc->GetName(), npc->GetFormID());
			continue;
		}
		// specific check for concreteness - follow chain of TPLTs: if we find LVLN then we won't see this NPC_ Record in the wild
		if (IsLeveled(npc))
		{
			REL_VMESSAGE("Skip Leveled NPC {}/0x{:08x}", npc->GetName(), npc->GetFormID());
			continue;
		}
		IsLootable(npc);
	}
	m_logResults = false;
}

bool NPCFilter::IsLeveled(const RE::TESNPC* npc) const
{
	return NPCIsLeveled(npc);
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
