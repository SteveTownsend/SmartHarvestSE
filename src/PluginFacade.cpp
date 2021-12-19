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
#include "Utilities/LogStackWalker.h"
#include "Collections/CollectionManager.h"
#include "Data/CosaveData.h"
#include "Data/dataCase.h"
#include "Data/LoadOrder.h"
#include "Data/SettingsCache.h"
#include "VM/UIState.h"
#include "WorldState/ActorTracker.h"
#include "WorldState/AdventureTargets.h"
#include "WorldState/LocationTracker.h"
#include "WorldState/PlacedObjects.h"
#include "WorldState/PlayerHouses.h"
#include "WorldState/PlayerState.h"
#include "WorldState/PopulationCenters.h"
#include "WorldState/QuestTargets.h"
#include "WorldState/Saga.h"

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

PluginFacade::PluginFacade() : m_loadProgress(LoadProgress::NotStarted), m_threadStarted(false), m_pluginSynced(false), m_loadedSettings(false)
{
}

bool PluginFacade::OneTimeLoad(void)
{
	__try
	{
		// Use structured exception handling during game data load
		REL_MESSAGE("Plugin not initialized - Game Data load executing");
		WindowsUtils::LogProcessWorkingSet();
		if (!Load())
			return false;
		m_loadProgress = LoadProgress::Complete;
		WindowsUtils::LogProcessWorkingSet();
	}
	__except (LogStackWalker::LogStack(GetExceptionInformation()))
	{
		REL_FATALERROR("Fatal Exception during Game Data load");
		return false;
	}
	return true;
}

bool PluginFacade::Init()
{
	// Thread safety is vital to ensure Load() only fires once.
	// cf. https://github.com/SteveTownsend/SmartHarvestSE/issues/230
	// SKSE::MessagingInterface::kPostLoadGame fired twice
	bool loadRequired(false);
	{
		RecursiveLockGuard guard(m_pluginLock);
		if (m_loadProgress == LoadProgress::NotStarted)
		{
			m_loadProgress = LoadProgress::Started;
			loadRequired = true;
		}
		else if (m_loadProgress == LoadProgress::Started)
		{
			return false;
		}
		// LoadProgress::LoadComplete
	}
	if (loadRequired)
	{
		if (!OneTimeLoad())
			return false;
	}

	while (!EventPublisher::Instance().GoodToGo())
	{
		REL_MESSAGE("Event publisher not ready yet");
		WindowsUtils::TakeNap(0.1);
	}

	if (!m_threadStarted)
	{
		// Start the thread once data is loaded
		m_threadStarted = true;
		Start();
	}
	// here we go
	EventPublisher::Instance().TriggerGameReady();
	return true;
}


void PluginFacade::Start()
{
	// do not start the thread if we failed to initialize
	if (!Loaded())
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
	if (!LoadOrder::Instance().Analyze())
	{
		REL_FATALERROR("Load Order unsupportable");
		return false;
	}
	DataCase::GetInstance()->CategorizeLootables();
	PopulationCenters::Instance().Categorize();
	AdventureTargets::Instance().Categorize();

	REL_MESSAGE("*** LOAD *** Record Placed Objects");
	PlacedObjects::Instance().RecordPlacedObjects();

	// Quest Target identification relies on Placed Objects analysis
	REL_MESSAGE("*** LOAD *** Analyze Quest Targets");
	QuestTargets::Instance().Analyze();

	// Collections are layered on top of categorized and placed objects
	REL_MESSAGE("*** LOAD *** Build Collections");
	CollectionManager::Instance().ProcessDefinitions();

	REL_MESSAGE("Plugin Data load complete!");
	return true;
}

bool PluginFacade::IsSynced() const {
	RecursiveLockGuard guard(m_pluginLock);
	return m_pluginSynced && m_loadedSettings;
}

bool PluginFacade::ScanAllowed() const {
	RecursiveLockGuard guard(m_pluginLock);
	// Limited looting is possible on a per-item basis, so proceed with scan if this is the only reason to skip
	static const bool allowIfRestricted(true);
	static const bool allowIfRestrictedHome(SettingsCache::Instance().LootAllowedItemsInPlayerHouse());
	if (!LocationTracker::Instance().IsPlayerInLootablePlace(allowIfRestricted, allowIfRestrictedHome))
	{
		DBG_MESSAGE("Location cannot be looted");
		return false;
	}
	else if (!PlayerState::Instance().CanLoot())
	{
		DBG_MESSAGE("Player State prevents looting");
		return false;
	}
	else if (!ScanGovernor::Instance().CanSearch())
	{
		DBG_MESSAGE("search disallowed or paused");
		return false;
	}
	return true;
}

void PluginFacade::ScanThread()
{
	REL_MESSAGE("Starting SHSE Worker Thread");
	while (true)
	{
		// Delay the scan for each loop
		double delaySeconds(SettingsCache::Instance().DelaySeconds());
		if (ScanGovernor::Instance().Calibrating())
		{
			// use hard-coded delay to make UX comprehensible
			delaySeconds = CalibrationThreadDelaySeconds;
		}
		WindowsUtils::TakeNap(delaySeconds);

		// Go no further if game load is in progress.
		if (!Instance().IsSynced())
		{
			continue;
		}

		// block until UI is good to go
		bool delayed(UIState::Instance().WaitUntilVMGoodToGo());

		// Player location checked for Cell/Location change on every loop, provided UI ready for status updates
		if (!LocationTracker::Instance().Refresh())
		{
			REL_VMESSAGE("Location or cell not stable yet");
			continue;
		}

		static const bool onMCMPush(delayed);
		static const bool onGameReload(false);
		PlayerState::Instance().Refresh(onMCMPush, onGameReload);

		// process any queued added items since last time
		CollectionManager::Instance().ProcessAddedItems();

		// reconcile SPERG mined items
		ScanGovernor::Instance().ReconcileSPERGMined();

		// Skip loot-OK checks if calibrating
		ReferenceScanType scanType(ReferenceScanType::NoLoot);
		if (ScanGovernor::Instance().Calibrating())
		{
			scanType = ReferenceScanType::Calibration;
		}
		else if (Instance().ScanAllowed())
		{
			scanType = ReferenceScanType::Loot;
		}

		ScanGovernor::Instance().DoPeriodicSearch(scanType);
	}
}

void PluginFacade::PrepareForReloadOrNewGame()
{
	UIState::Instance().Reset();
	CosaveData::Instance().Clear();
	Saga::Instance().Reset();

	// Do not scan again until we are in sync with the scripts
	RecursiveLockGuard guard(m_pluginLock);
	m_pluginSynced = false;
	m_loadedSettings = false;	// this comes from MCM script via OnGameReady
	REL_MESSAGE("Plugin sync required");
}

void PluginFacade::ResetTransientState(const bool gameReload)
{
	DBG_MESSAGE("Transient in-game restrictions reset, reload={}", gameReload ? "true" : "false");
	// This can be called while LocationTracker lock is held. No deadlock at present but care needed to ensure it remains so
	RecursiveLockGuard guard(m_pluginLock);
	DataCase::GetInstance()->ListsClear(gameReload);
	ScanGovernor::Instance().Clear(gameReload);
}

void PluginFacade::OnVMSync()
{
	REL_MESSAGE("Plugin sync, VM ready");
	WindowsUtils::LogProcessWorkingSet();
	ResetTransientState(true);
	// reset player state
	static const bool onMCMPush(false);
	static const bool onGameReload(true);
	PlayerState::Instance().Refresh(onMCMPush, onGameReload);
	// reset location history - also forces proper recalculation of carry-weight per refreshed PayerState
	LocationTracker::Instance().Reset();
	// unblock possible player house checks after game reload
	PlayerHouses::Instance().Clear();
	// reset Actor data
	ActorTracker::Instance().Reset();
	// seed state using cosave data
	CosaveData::Instance().SeedState();
	// Update Collections State, including saved-game data if present
	CollectionManager::Instance().OnGameReload();
	// need to wait for the scripts to sync up before performing player house checks
	m_pluginSynced = true;
	REL_MESSAGE("Plugin sync completed");
	WindowsUtils::LogProcessWorkingSet();
}

void PluginFacade::OnGameLoaded()
{
	REL_MESSAGE("MCM OnGameLoaded completed");
	m_loadedSettings = true;
}

// lock not required, by construction
void PluginFacade::OnSettingsPushed()
{
	// refresh player state that could be affected
	static const bool onMCMPush(true);
	static const bool onGameReload(false);
	PlayerState::Instance().Refresh(onMCMPush, onGameReload);

	// Base Object Forms and REFRs handled for the case where we are not reloading game
	DataCase::GetInstance()->ResetBlockedForms();
	DataCase::GetInstance()->ResetBlockedReferences(onGameReload);

	// clear list of dead bodies pending looting - blocked reference cleanup allows redo if still viable
	ActorTracker::Instance().Reset();

	// clear lists of looted and locked containers
	ScanGovernor::Instance().ResetLootedContainers();
	ScanGovernor::Instance().ForgetLockedContainers();
}

}
