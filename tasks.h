#pragma once

#include "objects.h"
#include "TESFormHelper.h"
#include "iniSettings.h"
#include "containerLister.h"

#include <unordered_set>
#include <mutex>

#define QUEST_ID	0x01D8C
#define NGEF_ID		0x00D64

enum class LootingType {
	LeaveBehind = 0,
	LootAlwaysSilent,
	LootAlwaysNotify,
	LootIfValuableEnoughSilent,
	LootIfValuableEnoughNotify,
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

inline bool LootingDependsOnValueWeight(const LootingType lootingType, ObjectType objectType)
{
	if (objectType == ObjectType::septims ||
		objectType == ObjectType::keys ||
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
	SearchTask(RE::TESObjectREFR* candidate, INIFile::SecondaryType targetType);

	static void Init(void);
	void Run();
	static void UnlockAll();

	static bool IsLockedForAutoHarvest(const RE::TESObjectREFR* refr);
	static size_t PendingAutoHarvest();
	static bool UnlockAutoHarvest(const RE::TESObjectREFR* refr);
	static bool IsPlayerHouse(const RE::BGSLocation* location);
	static bool AddPlayerHouse(const RE::BGSLocation* location);
	static bool RemovePlayerHouse(const RE::BGSLocation* location);

	static void MergeExcludeList();
	static void ResetExcludedLocations();
	static void AddLocationToExcludeList(const RE::TESForm* location);
	static void DropLocationFromExcludeList(const RE::TESForm* location);

	static void Start();
	static void PrepareForReload();
	static void ResetCarryWeight();
	static void Allow();
	static void Disallow();
	static bool IsAllowed();
	static void ResetRestrictions(const bool gameReload);
	static SInt32 StartSearch(SInt32 type1);
	static void SetPlayerHouseKeyword(RE::BGSKeyword* keyword);

	static bool GoodToGo();

	void TriggerGetCritterIngredient();
	static void TriggerCarryWeightDelta(const int delta);
	static void TriggerResetCarryWeight();
	void TriggerAutoHarvest(const ObjectType objType, int itemCount, const bool isSilent, const bool ignoreBlocking, const bool manualLootNotify);
	static bool LockAutoHarvest(const RE::TESObjectREFR* refr);

	void TriggerContainerLootMany(std::vector<std::pair<InventoryItem, bool>>& targets, const int animationType);
	static void TriggerObjectGlow(RE::TESObjectREFR* refr, const int duration);

	static bool IsLocationExcluded();
	bool IsLootingForbidden() const;
	bool IsBookGlowable() const;

	static INIFile* m_ini;
	static RE::BGSRefAlias* m_eventTarget;
	static bool firstTime;

	RE::TESObjectREFR* m_candidate;
	INIFile::SecondaryType m_targetType;

	static int m_crimeCheck;
	static int m_belongingsCheck;

	static std::unordered_set<const RE::TESObjectREFR*> m_autoHarvestLock;
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

	static std::unordered_set<const RE::TESForm*> m_excludeLocations;
	static bool m_pluginSynced;
	static RecursiveLock m_lock;
	static std::unordered_map<const RE::TESObjectREFR*, std::chrono::time_point<std::chrono::high_resolution_clock>> m_glowExpiration;
	static std::unordered_map<const RE::TESObjectREFR*, std::vector<RE::TESForm*>> m_lootTargetByREFR;
	static std::unordered_set<const RE::BGSLocation*> m_locationCheckedForPlayerHouse;
};
