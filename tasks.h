#pragma once

#include "containerLister.h"
#include "BoundedList.h"
#include "EventPublisher.h"
#include "UIState.h"

#include <mutex>
#include <deque>

enum class LootingType {
	LeaveBehind = 0,
	LootAlwaysSilent,
	LootAlwaysNotify,
	LootIfValuableEnoughSilent,
	LootIfValuableEnoughNotify,
	MAX
};

enum class SpecialObjectHandling {
	DoNotLoot = 0,
	DoLoot,
	GlowTarget,
	MAX
};

inline bool LootingRequiresNotification(const LootingType lootingType)
{
	return lootingType == LootingType::LootIfValuableEnoughNotify || lootingType == LootingType::LootAlwaysNotify;
}

inline LootingType LootingTypeFromIniSetting(const double iniSetting)
{
	UInt32 intSetting(static_cast<UInt32>(iniSetting));
	if (intSetting >= static_cast<SInt32>(LootingType::MAX))
	{
		return LootingType::LeaveBehind;
	}
	return static_cast<LootingType>(intSetting);
}

inline bool IsSpecialObjectLootable(const SpecialObjectHandling specialObjectHandling)
{
	return specialObjectHandling == SpecialObjectHandling::DoLoot;
}

inline SpecialObjectHandling SpecialObjectHandlingFromIniSetting(const double iniSetting)
{
	UInt32 intSetting(static_cast<UInt32>(iniSetting));
	if (intSetting >= static_cast<SInt32>(SpecialObjectHandling::MAX))
	{
		return SpecialObjectHandling::DoNotLoot;
	}
	return static_cast<SpecialObjectHandling>(intSetting);
}

inline bool LootingDependsOnValueWeight(const LootingType lootingType, ObjectType objectType)
{
	if (objectType == ObjectType::septims ||
		objectType == ObjectType::key ||
		objectType == ObjectType::oreVein ||
		objectType == ObjectType::ammo ||
		objectType == ObjectType::lockpick)
		return false;
	if (lootingType != LootingType::LootIfValuableEnoughNotify && lootingType != LootingType::LootIfValuableEnoughSilent)
		return false;
	return true;
}

class SearchTask
{
public:
	static constexpr size_t MaxREFRSPerPass = 75;

	SearchTask(RE::TESObjectREFR* candidate, INIFile::SecondaryType targetType);

	static bool Init(void);
	void Run();

	static bool IsLockedForHarvest(const RE::TESObjectREFR* refr);
	static size_t PendingHarvestNotifications();
	static bool UnlockHarvest(const RE::TESObjectREFR* refr, const bool isSilent);

	static void MergeBlackList();
	static void SyncDone(void);
	static void ToggleCalibration();

	static void Start();
	static void PrepareForReload();
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

	// Dead bodies pending looting, in order of our encountering them, with the time of registration.
	// Used to delay looting bodies until the game has sorted out their state for unlocked introspection using GetContainer().
	// This may not be the actual time of death if this body has been dead for some time before we arrive to loot it,
	// which would be safe. Contrast this with looting during combat, where we could try to loot _very soon_ (microseconds,
	// potentially) after game registers the Actor's demise.
	static std::deque<std::pair<RE::TESObjectREFR*, std::chrono::time_point<std::chrono::high_resolution_clock>>> m_actorApparentTimeOfDeath;
	// Record looted containers to avoid re-scan of empty or looted chest and dead body. Resets on game reload or MCM settings update.
	static std::unordered_map<const RE::TESObjectREFR*, RE::FormID> m_lootedDynamicContainers;
	static std::unordered_set<const RE::TESObjectREFR*> m_lootedContainers;

	// special object glow - not too long, in case we loot or move away
	static constexpr int ObjectGlowDurationSpecialSeconds = 10;

private:
	// brief glow for looted objects and other purposes
	static constexpr int ObjectGlowDurationLootedSeconds = 2;

	// Loot Range calibration settting
	static bool m_calibrating;
	static int m_calibrateRadius;
	static constexpr int CalibrationRangeDelta = 3;
	static constexpr int MaxCalibrationRange = 100;

	// very short glow for loot range calibration
	static constexpr int ObjectGlowDurationCalibrationSeconds = 1;
	// give the debug message time to catch up during calibration
	static constexpr int CalibrationDelay = 3;

	// allow extended interval before looting if 'leveled list on death' perks apply to player
	static constexpr int ActorReallyDeadWaitIntervalSeconds = 3;
	static constexpr int ActorReallyDeadWaitIntervalSecondsLong = 10;

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

	
	void GetLootFromContainer(std::vector<std::pair<InventoryItem, bool>>& targets, const int animationType);
	void GlowObject(RE::TESObjectREFR* refr, const int duration, const GlowReason glowReason);

	static bool ReleaseReliablyDeadActors(BoundedList<RE::TESObjectREFR*>& refs);
};
