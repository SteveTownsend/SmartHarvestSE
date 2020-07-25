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

#include "Data/LoadOrder.h"
#include "Utilities/version.h"

namespace shse
{

std::unique_ptr<LoadOrder> LoadOrder::m_instance;

LoadOrder& LoadOrder::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<LoadOrder>();
	}
	return *m_instance;
}

LoadOrder::LoadOrder() : m_shsePriority(-1)
{
}

bool LoadOrder::Analyze(void)
{
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	if (!dhnd)
		return false;
	static const std::string oldName(PRIORNAME);
	static const std::string shseName(MODNAME);
	// Process each ESP and ESPFE/ESL in the master list of RE::TESFile / loaded mod instances
	int priority(0);
	for (const auto modFile : dhnd->files)
	{
		// Make sure the earlier version of the mod is not installed
		if (oldName.compare(0, oldName.length(), &modFile->fileName[0]) == 0)
		{
			REL_ERROR("Prior mod plugin version ({}) is incompatible with current plugin ({})", oldName.c_str(), &MODNAME[0]);
			return false;
		}
		if (shseName.compare(0, shseName.length(), &modFile->fileName[0]) == 0)
		{
			m_shsePriority = priority;
		}

		// validation logic from CommonLibSSE 
		if (modFile->compileIndex == 0xFF)
		{
			REL_MESSAGE("{} skipped, has load index 0xFF", modFile->fileName);
			continue;
		}

		RE::FormID formIDMask(modFile->compileIndex << (3 * 8));
		formIDMask += modFile->smallFileCompileIndex << ((1 * 8) + 4);

		m_loadInfoByName.insert(std::make_pair(modFile->fileName, LoadInfo({ formIDMask, priority })));
		REL_MESSAGE("{} has FormID mask 0x{:08x}, priority {}", modFile->fileName, formIDMask, priority);
		++priority;
	}
	return true;
}

// returns zero if mod not loaded
RE::FormID LoadOrder::GetFormIDMask(const std::string& modName) const
{
	const auto& matched(m_loadInfoByName.find(modName));
	if (matched != m_loadInfoByName.cend())
	{
		return matched->second.m_mask;
	}
	return InvalidPlugin;
}

// returns true iff mod listed, which means it is active by virtue of exclusion of 0xff above
bool LoadOrder::IncludesMod(const std::string& modName) const
{
	return m_loadInfoByName.find(modName) != m_loadInfoByName.cend();
}

// returns true iff mod is installed/active, and earlier than SHSE in Load Order
bool LoadOrder::ModPrecedesSHSE(const std::string& modName) const
{
	const auto matched(m_loadInfoByName.find(modName));
	return matched != m_loadInfoByName.cend() && matched->second.m_priority < m_shsePriority;
}

bool LoadOrder::ModOwnsForm(const std::string& modName, const RE::FormID formID) const
{
	RE::FormID modMask(GetFormIDMask(modName));
	if (modMask == InvalidPlugin)
		return false;
	return (modMask & LightFormIDSentinel) == LightFormIDSentinel ?
		modMask == (formID & LightFormIDMask) : modMask == (formID & RegularFormIDMask);
}

void LoadOrder::AsJSON(nlohmann::json& j) const
{
	j["order"] = nlohmann::json::array();
	std::map<LoadInfo, std::string> ordered;
	std::for_each(m_loadInfoByName.cbegin(), m_loadInfoByName.cend(), [&](const auto& loadInfo)
	{
		ordered.insert(std::make_pair(loadInfo.second, loadInfo.first));
	});
	for (const auto& loadInfo : ordered)
	{
		nlohmann::json entry;
		entry["formIDMask"] = int(loadInfo.first.m_mask);
		entry["priority"] = int(loadInfo.first.m_priority);
		entry["name"] = loadInfo.second;
		j["order"].push_back(entry);
	}
}

void to_json(nlohmann::json& j, const LoadOrder& loadOrder)
{
	loadOrder.AsJSON(j);
}

}

