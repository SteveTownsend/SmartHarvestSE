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

#include "PluginFacade.h"

#include "Looting/containerLister.h"
#include "Looting/IRangeChecker.h"
#include "VM/EventPublisher.h"
#include "VM/UIState.h"

#include <mutex>


namespace shse
{

class SearchTask
{
public:
#if _DEBUG
	// make sure load spike handling works OK
	static constexpr size_t MaxREFRSPerPass = 25;
#else
	static constexpr size_t MaxREFRSPerPass = 75;
#endif

	SearchTask(RE::TESObjectREFR* target, INIFile::SecondaryType targetType, const bool stolen);

	void Run();

	static void Clear(const bool gameReload);
	static bool IsLockedForHarvest(const RE::TESObjectREFR* refr);
	static bool UnlockHarvest(const RE::TESObjectREFR* refr, const bool isSilent);

	static void ToggleCalibration(const bool glowDemo);

	static void Allow();
	static void Disallow();
	static bool IsAllowed();
	static void DoPeriodicSearch();
	static inline bool Calibrating() {
		RecursiveLockGuard guard(m_searchLock);
		return m_calibrating;
	}

	static RE::FormID LootedDynamicContainerFormID(const RE::TESObjectREFR* refr);
	static bool IsLootedContainer(const RE::TESObjectREFR* refr);
	static void ResetLootedDynamicContainers();
	static void ResetLootedContainers();
	static void ForgetLockedContainers();
	static void ClearPendingHarvestNotifications();
	static void ClearGlowExpiration();

private:
	static size_t PendingHarvestNotifications();
	static bool LockHarvest(const RE::TESObjectREFR* refr, const bool isSilent);

	bool IsReferenceLockedContainer(const RE::TESObjectREFR* refr);

	static void MarkDynamicContainerLooted(const RE::TESObjectREFR* refr);

	static void MarkContainerLooted(const RE::TESObjectREFR* refr);

	bool IsLootingForbidden(const INIFile::SecondaryType targetType);
	bool IsBookGlowable() const;

	static bool HasDynamicData(RE::TESObjectREFR* refr);
	static void RegisterActorTimeOfDeath(RE::TESObjectREFR* refr);

	// special object glow - not too long, in case we loot or move away
	static constexpr int ObjectGlowDurationSpecialSeconds = 10;
	// brief glow for looted objects and other purposes
	static constexpr int ObjectGlowDurationLootedSeconds = 2;

	static INIFile* m_ini;

	bool m_stolen;
	RE::TESObjectREFR* m_candidate;
	INIFile::SecondaryType m_targetType;

	static std::unordered_set<const RE::TESObjectREFR*> m_HarvestLock;
	static int m_pendingNotifies;

	static bool m_searchAllowed;

	static RecursiveLock m_searchLock;
	static std::unordered_map<const RE::TESObjectREFR*, std::chrono::time_point<std::chrono::high_resolution_clock>> m_glowExpiration;

	// Record looted containers to avoid re-scan of empty or looted chest and dead body. Resets on game reload or MCM settings update.
	static std::unordered_map<const RE::TESObjectREFR*, RE::FormID> m_lootedDynamicContainers;
	static std::unordered_set<const RE::TESObjectREFR*> m_lootedContainers;

	// BlackList for Locked Containers. Never auto-loot unless config permits. Reset on game reload.
	static std::unordered_set<const RE::TESObjectREFR*> m_lockedContainers;

	// Loot Range calibration setting
	static bool m_calibrating;
	static int m_calibrateRadius;
	static int m_calibrateDelta;
	static bool m_glowDemo;
	static GlowReason m_nextGlow;
	static constexpr int CalibrationRangeDelta = 3;
	static constexpr int MaxCalibrationRange = 100;
	static constexpr int GlowDemoRange = 30;

	// short glow for loot range calibration and glow demo
	static constexpr int ObjectGlowDurationCalibrationSeconds = int(PluginFacade::CalibrationThreadDelay) - 2;

	GlowReason m_glowReason;
	inline void UpdateGlowReason(const GlowReason glowReason)
	{
		if (glowReason < m_glowReason)
			m_glowReason = glowReason;
	}

	void GetLootFromContainer(std::vector<std::tuple<InventoryItem, bool, bool>>& targets, const int animationType);
	void GlowObject(RE::TESObjectREFR* refr, const int duration, const GlowReason glowReason);
};

}