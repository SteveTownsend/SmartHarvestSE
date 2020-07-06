#pragma once

#include "Looting/containerLister.h"
#include "Looting/IRangeChecker.h"
#include "VM/EventPublisher.h"
#include "VM/UIState.h"

#include <mutex>

class SearchTask
{
public:
#if _DEBUG
	// make sure load spike handling works OK
	static constexpr size_t MaxREFRSPerPass = 10;
#else
	static constexpr size_t MaxREFRSPerPass = 75;
#endif

	SearchTask(const TargetREFR& target, INIFile::SecondaryType targetType);

	static bool Init(void);
	void Run();

	static bool IsLockedForHarvest(const RE::TESObjectREFR* refr);
	static size_t PendingHarvestNotifications();
	static bool UnlockHarvest(const RE::TESObjectREFR* refr, const bool isSilent);

	static void SyncDone(const bool reload);
	static void ToggleCalibration(const bool shaderTest);

	static void Start();
	static void PrepareForReload();
	static void AfterReload();
	static void Allow();
	static void Disallow();
	static bool IsAllowed();
	static void ResetRestrictions(const bool gameReload);
	static void DoPeriodicSearch();

	static bool LockHarvest(const RE::TESObjectREFR* refr, const bool isSilent);

	bool IsLootingForbidden(const INIFile::SecondaryType targetType);
	bool IsBookGlowable() const;

	static void MarkDynamicContainerLooted(const RE::TESObjectREFR* refr);
	static RE::FormID LootedDynamicContainerFormID(const RE::TESObjectREFR* refr);
	static void ResetLootedDynamicContainers();

	static void MarkContainerLooted(const RE::TESObjectREFR* refr);
	static bool IsLootedContainer(const RE::TESObjectREFR* refr);
	static void ResetLootedContainers();

	static bool HasDynamicData(RE::TESObjectREFR* refr);
	static void RegisterActorTimeOfDeath(RE::TESObjectREFR* refr);

	static void OnGoodToGo(void);

	static INIFile* m_ini;

	double m_distance;
	RE::TESObjectREFR* m_candidate;
	INIFile::SecondaryType m_targetType;

	static int m_crimeCheck;
	static SpecialObjectHandling m_belongingsCheck;

	static std::unordered_set<const RE::TESObjectREFR*> m_HarvestLock;
	static int m_pendingNotifies;

	static RecursiveLock m_searchLock;
	static bool m_threadStarted;
	static bool m_searchAllowed;
	
	static bool m_pluginSynced;

	static RecursiveLock m_lock;
	static std::unordered_map<const RE::TESObjectREFR*, std::chrono::time_point<std::chrono::high_resolution_clock>> m_glowExpiration;

	// Record looted containers to avoid re-scan of empty or looted chest and dead body. Resets on game reload or MCM settings update.
	static std::unordered_map<const RE::TESObjectREFR*, RE::FormID> m_lootedDynamicContainers;
	static std::unordered_set<const RE::TESObjectREFR*> m_lootedContainers;

	// special object glow - not too long, in case we loot or move away
	static constexpr int ObjectGlowDurationSpecialSeconds = 10;

private:
	// brief glow for looted objects and other purposes
	static constexpr int ObjectGlowDurationLootedSeconds = 2;

	// Loot Range calibration setting
	static bool m_calibrating;
	static int m_calibrateRadius;
	static int m_calibrateDelta;
	static bool m_glowDemo;
	static GlowReason m_nextGlow;
	static constexpr int CalibrationRangeDelta = 3;
	static constexpr int MaxCalibrationRange = 100;
	static constexpr int ShaderTestRange = 30;

	// give the debug message time to catch up during calibration
	static constexpr int CalibrationDelay = 5;
	// short glow for loot range calibration and glow demo
	static constexpr int ObjectGlowDurationCalibrationSeconds = CalibrationDelay - 2;

	// Worker thread loop smallest possible delay
	static constexpr double MinDelay = 0.1;

	static bool m_pluginOK;

	GlowReason m_glowReason;
	inline void UpdateGlowReason(const GlowReason glowReason)
	{
		if (glowReason < m_glowReason)
			m_glowReason = glowReason;
	}

	static bool Load(void);
	static void TakeNap(void);
	static void ScanThread(void);

	
	void GetLootFromContainer(std::vector<std::tuple<InventoryItem, bool, bool>>& targets, const int animationType);
	void GlowObject(RE::TESObjectREFR* refr, const int duration, const GlowReason glowReason);
};
