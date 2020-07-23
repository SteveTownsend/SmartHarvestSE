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
#include "PluginFacade.h"

#include "Utilities/versiondb.h"
#include "Collections/CollectionManager.h"
#include "Data/dataCase.h"
#include "Data/LoadOrder.h"
#include "VM/UIState.h"
#include "WorldState/ActorTracker.h"
#include "WorldState/LocationTracker.h"
#include "WorldState/PlayerHouses.h"
#include "WorldState/PlayerState.h"
#include "WorldState/PopulationCenters.h"

namespace shse
{

std::unique_ptr<PluginFacade> PluginFacade::m_instance;

PluginFacade& PluginFacade::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<PluginFacade>();
	}
	return *m_instance;
}

PluginFacade::PluginFacade() : m_pluginOK(false), m_threadStarted(false), m_pluginSynced(false)
{
}

bool PluginFacade::Init()
{
	if (!m_pluginOK)
	{
		__try
		{
			// Use structured exception handling during game data load
			REL_MESSAGE("Plugin not initialized - Game Data load executing");
			WindowsUtils::LogProcessWorkingSet();
			if (!Load())
				return false;
			WindowsUtils::LogProcessWorkingSet();
		}
		__except (LogStackWalker::LogStack(GetExceptionInformation()))
		{
			REL_FATALERROR("Fatal Exception during Game Data load");
			return false;
		}
	}
	if (!m_threadStarted)
	{
		// Start the thread once data is loaded
		m_threadStarted = true;
		Start();
	}
	return true;
}


void PluginFacade::Start()
{
	// do not start the thread if we failed to initialize
	if (!m_pluginOK)
		return;
	std::thread([]()
	{
		// use structured exception handling to get stack walk on windows exceptions
		__try
		{
			ScanThread();
		}
		__except (LogStackWalker::LogStack(GetExceptionInformation()))
		{
		}
	}).detach();
}

bool PluginFacade::Load()
{
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Startup: Load Game Data");
#endif
#if _DEBUG
	VersionDb db;

	// Try to load database of version 1.5.97.0 regardless of running executable version.
	if (!db.Load(1, 5, 97, 0))
	{
		DBG_FATALERROR("Failed to load database for 1.5.97.0!");
		return false;
	}

	// Write out a file called offsets-1.5.97.0.txt where each line is the ID and offset.
	db.Dump("offsets-1.5.97.0.txt");
	DBG_MESSAGE("Dumped offsets for 1.5.97.0");
#endif
	if (!shse::LoadOrder::Instance().Analyze())
	{
		REL_FATALERROR("Load Order unsupportable");
		return false;
	}
	DataCase::GetInstance()->CategorizeLootables();
	PopulationCenters::Instance().Categorize();

	// Collections are layered on top of categorized objects
	REL_MESSAGE("*** LOAD *** Build Collections");
	shse::CollectionManager::Instance().ProcessDefinitions();

	m_pluginOK = true;
	REL_MESSAGE("Plugin now in sync - Game Data load complete!");
	return true;
}

void PluginFacade::TakeNap()
{
	double delay(INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "IntervalSeconds"));
	delay = std::max(MinThreadDelaySeconds, delay);
	if (ScanGovernor::Instance().Calibrating())
	{
		// use hard-coded delay to make UX comprehensible
		delay = CalibrationThreadDelaySeconds;
	}

	DBG_MESSAGE("wait for %d milliseconds", static_cast<long long>(delay * 1000.0));
	auto nextRunTime = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(static_cast<long long>(delay * 1000.0));
	std::this_thread::sleep_until(nextRunTime);
}

bool PluginFacade::IsSynced() const {
	RecursiveLockGuard guard(m_pluginLock);
	return m_pluginSynced;
}

void PluginFacade::ScanThread()
{
	REL_MESSAGE("Starting SHSE Worker Thread");
	while (true)
	{
		// Delay the scan for each loop
		Instance().TakeNap();
		{
			// Go no further if game load is in progress.
			if (!Instance().IsSynced())
			{
				REL_MESSAGE("Plugin sync still pending");
				continue;
			}
		}

		if (!EventPublisher::Instance().GoodToGo())
		{
			REL_MESSAGE("Event publisher not ready yet");
			continue;
		}

		if (!UIState::Instance().OKForSearch())
		{
			DBG_MESSAGE("UI state not good to loot");
			continue;
		}

		// Player location checked for Cell/Location change on every loop, provided UI ready for status updates
		if (!LocationTracker::Instance().Refresh())
		{
			REL_VMESSAGE("Location or cell not stable yet");
			continue;
		}

		static const bool onMCMPush(false);
		static const bool onGameReload(false);
		shse::PlayerState::Instance().Refresh(onMCMPush, onGameReload);

		// process any queued added items since last time
		shse::CollectionManager::Instance().ProcessAddedItems();

		// Skip loot-OK checks if calibrating
		ReferenceScanType scanType(ReferenceScanType::NoLoot);
		if (ScanGovernor::Instance().Calibrating())
		{
			scanType = ReferenceScanType::Calibration;
		}
		else
		{
			// Limited looting is possible on a per-item basis, so proceed with scan if this is the only reason to skip
			static const bool allowIfRestricted(true);
			if (!LocationTracker::Instance().IsPlayerInLootablePlace(LocationTracker::Instance().PlayerCell(), allowIfRestricted))
			{
				DBG_MESSAGE("Location cannot be looted");
			}
			else if (!shse::PlayerState::Instance().CanLoot())
			{
				DBG_MESSAGE("Player State prevents looting");
			}
			else if (!ScanGovernor::Instance().IsAllowed())
			{
				DBG_MESSAGE("search disallowed");
			}
			else
			{
				// looting is allowed
				scanType = ReferenceScanType::Loot;
			}
		}

		ScanGovernor::Instance().DoPeriodicSearch(scanType);
	}
}

void PluginFacade::PrepareForReload()
{
	UIState::Instance().Reset();

	// Do not scan again until we are in sync with the scripts
	RecursiveLockGuard guard(m_pluginLock);
	m_pluginSynced = false;
}

void PluginFacade::AfterReload()
{
	static const bool onMCMPush(false);
	static const bool onGameReload(true);
	PlayerState::Instance().Refresh(onMCMPush, onGameReload);
}

// this is the last function called by the scripts when re-syncing state
void PluginFacade::SyncDone()
{
	RecursiveLockGuard guard(m_pluginLock);
	// reset blocked lists to allow recheck vs current state
	static const bool reload(true);
	ResetState(reload);
	REL_MESSAGE("Restrictions reset, new/loaded game");
	// need to wait for the scripts to sync up before performing player house checks
	m_pluginSynced = true;
}

void PluginFacade::ResetState(const bool gameReload)
{
	// TODO review to make sure this does not deadlock
	RecursiveLockGuard guard(m_pluginLock);
	DataCase::GetInstance()->ListsClear(gameReload);
	ScanGovernor::Instance().Clear(gameReload);

	if (gameReload)
	{
		// unblock possible player house checks after game reload
		PlayerHouses::Instance().Clear();
		// reset Actor data
		shse::ActorTracker::Instance().Reset();
		// Reset Collections State and reapply the saved-game data
		shse::CollectionManager::Instance().OnGameReload();
	}
}

// lock not required, by construction. This is called-back in ScanThread via UIState so should be fine
void PluginFacade::OnGoodToGo()
{
	REL_MESSAGE("UI/controls now good-to-go, wait before first scan");
	TakeNap();
}

// lock not required, by construction
void PluginFacade::OnSettingsPushed()
{
	// refresh player state that could be affected
	static const bool onMCMPush(true);
	static const bool onGameReload(false);
	shse::PlayerState::Instance().Refresh(onMCMPush, onGameReload);

	// Base Object Forms and REFRs handled for the case where we are not reloading game
	DataCase::GetInstance()->ResetBlockedForms();
	DataCase::GetInstance()->ClearBlockedReferences(onGameReload);

	// clear list of dead bodies pending looting - blocked reference cleanup allows redo if still viable
	ActorTracker::Instance().Reset();

	// clear lists of looted and locked containers
	ScanGovernor::Instance().ResetLootedContainers();
	ScanGovernor::Instance().ForgetLockedContainers();
}

}
