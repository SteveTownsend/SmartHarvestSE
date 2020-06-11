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

// Population Center Looting Size, overloaded to check Looting Permissions
enum class PopulationCenterSize {
	None = 0,
	Settlements,
	Towns,		// implies Settlements
	Cities,		// implies Towns and Settlements
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

inline PopulationCenterSize PopulationCenterSizeFromIniSetting(const double iniSetting)
{
	UInt32 intSetting(static_cast<UInt32>(iniSetting));
	if (intSetting >= static_cast<SInt32>(PopulationCenterSize::MAX))
	{
		return PopulationCenterSize::Cities;
	}
	return static_cast<PopulationCenterSize>(intSetting);
}

class SearchTask
{
public:
	static constexpr size_t MaxREFRSPerPass = 75;

	SearchTask(RE::TESObjectREFR* candidate, INIFile::SecondaryType targetType);

	static bool Init(void);
	void Run();
	static void UnlockAll();

	static bool IsLockedForHarvest(const RE::TESObjectREFR* refr);
	static size_t PendingHarvestNotifications();
	static bool UnlockHarvest(const RE::TESObjectREFR* refr, const bool isSilent);
	static bool IsPlayerHouse(const RE::BGSLocation* location);
	static bool AddPlayerHouse(const RE::BGSLocation* location);
	static bool RemovePlayerHouse(const RE::BGSLocation* location);

	static void MergeBlackList();
	static void ResetExcludedLocations();
	static void AddLocationToBlackList(const RE::TESForm* location);
	static void DropLocationFromBlackList(const RE::TESForm* location);

	static void Start();
	static void PrepareForReload();
	static void ResetCarryWeight();
	static void Allow();
	static void Disallow();
	static bool IsAllowed();
	static void ResetRestrictions(const bool gameReload);
	static void DoPeriodicSearch();
	static void SetPlayerHouseKeyword(RE::BGSKeyword* keyword);
	static void CategorizePopulationCenters();

	static bool GoodToGo();

	static bool LockHarvest(const RE::TESObjectREFR* refr, const bool isSilent);

	static bool IsLocationExcluded();
	bool IsLootingForbidden();
	bool IsBookGlowable() const;

	static void MarkDynamicContainerLooted(RE::TESObjectREFR* refr);
	static RE::FormID LootedDynamicContainerFormID(RE::TESObjectREFR* refr);
	static void ResetLootedDynamicContainers();

	static void MarkContainerLooted(RE::TESObjectREFR* refr);
	static bool IsLootedContainer(RE::TESObjectREFR* refr);
	static void ResetLootedContainers();

	static bool HasDynamicData(RE::TESObjectREFR* refr);
	static void RegisterActorTimeOfDeath(RE::TESObjectREFR* refr);

	static void OnMenuClose(void);

	static INIFile* m_ini;

	RE::TESObjectREFR* m_candidate;
	INIFile::SecondaryType m_targetType;
	std::chrono::system_clock::time_point m_runtime;

	static int m_crimeCheck;
	static SpecialObjectHandling m_belongingsCheck;

	static std::unordered_set<const RE::TESObjectREFR*> m_HarvestLock;
	static int m_pendingNotifies;
	static std::unordered_set<const RE::BGSLocation*> m_playerHouses;

	static RecursiveLock m_searchLock;
	static bool m_threadStarted;
	static bool m_searchAllowed;
	static bool m_sneaking;
	static RE::TESObjectCELL* m_playerCell;
	static bool m_playerCellSelfOwned;
	static RE::BGSLocation* m_playerLocation;
	static RE::BGSKeyword* m_playerHouseKeyword;
	static bool m_carryAdjustedForCombat;
	static bool m_carryAdjustedForPlayerHome;
	static bool m_carryAdjustedForDrawnWeapon;
	static int m_currentCarryWeightChange;
	static bool m_perksAddLeveledItemsOnDeath;

	static std::unordered_map<const RE::BGSLocation*, PopulationCenterSize> m_populationCenters;
	static std::unordered_set<const RE::TESForm*> m_excludeLocations;
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
	static std::unordered_map<RE::TESObjectREFR*, RE::FormID> m_lootedDynamicContainers;
	static std::unordered_set<RE::TESObjectREFR*> m_lootedContainers;

	static std::unordered_set<const RE::BGSLocation*> m_locationCheckedForPlayerHouse;

	static const int ObjectGlowDurationSpecialSeconds;
	static const int ObjectGlowDurationLootedSeconds;

	// allow extended interval before looting if 'leveled list on death' perks apply to player
	static const int ActorReallyDeadWaitIntervalSeconds;
	static const int ActorReallyDeadWaitIntervalSecondsLong;

private:
	static bool m_pluginOK;

	GlowReason m_glowReason;
	inline void UpdateGlowReason(const GlowReason glowReason)
	{
		if (glowReason < m_glowReason)
			m_glowReason = glowReason;
	}
	static void ScanThread();
	static bool IsMagicallyConcealed(RE::MagicTarget* target);
	static bool IsPopulationCenterExcluded();

	void GetLootFromContainer(std::vector<std::pair<InventoryItem, bool>>& targets, const int animationType);
	void GlowObject(RE::TESObjectREFR* refr, const int duration, const GlowReason glowReason);

	static std::chrono::time_point<std::chrono::high_resolution_clock> m_lastPerkCheck;
	static const int PerkCheckIntervalSeconds;
	static void CheckPerks(const bool force);
	static bool ReleaseReliablyDeadActors(BoundedList<RE::TESObjectREFR*>& refs);
};
