#pragma once

//#include "skse64/PapyrusVM.h"

#include "objects.h"
#include "TESFormHelper.h"
#include "iniSettings.h"

#include <unordered_set>
#include <mutex>

#define QUEST_ID	0x01D8C
#define NGEF_ID		0x00D64

enum class LootingType {
	LeaveBehind = 0,
	LootAlwaysNotify,
	LootAlwaysSilent,
	LootIfValuableEnoughNotify,
	LootIfValuableEnoughSilent,
	MAX
};

inline bool LootingRequiresNotification(const LootingType lootingType)
{
	return lootingType == LootingType::LootIfValuableEnoughSilent || lootingType == LootingType::LootAlwaysSilent;
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
	SearchTask(const std::vector<std::pair<RE::TESObjectREFR*, INIFile::SecondaryType>>& candidates);

	static void Init(void);
	virtual void Run();
	static void UnlockAll();

	static bool IsLockedForAutoHarvest(const RE::TESObjectREFR* refr);
	static size_t PendingAutoHarvest();
	static bool UnlockAutoHarvest(const RE::TESObjectREFR* refr);
	static bool IsPossiblePlayerHouse(const RE::BGSLocation* location);
	static bool UnlockPossiblePlayerHouse(const RE::BGSLocation* location);

	static void MergeExcludeList();
	static void ResetExcludedLocations();
	static void AddLocationToExcludeList(const RE::TESForm* location);
	static void DropLocationFromExcludeList(const RE::TESForm* location);

	static void Start();
	static void Allow();
	static void Disallow();
	static bool IsAllowed();
	static SInt32 StartSearch(SInt32 type1);
	static void SetPlayerHouseKeyword(RE::BGSKeyword* keyword);

	static bool GoodToGo();

	void TriggerGetCritterIngredient();
	static void TriggerCarryWeightDelta(const int delta);
	void TriggerAutoHarvest(const ObjectType objType, int itemCount, const bool isSilent, const bool ignoreBlocking);
	static bool LockAutoHarvest(const RE::TESObjectREFR* refr);
	static bool LockPossiblePlayerHouse(const RE::BGSLocation* location);

	void TriggerContainerLootMany(const std::vector<std::tuple<RE::TESBoundObject*, int, bool>>& targets, const bool animate);
	void TriggerObjectGlow();
	static void StopObjectGlow(const RE::TESObjectREFR* refr);

	static bool IsLocationExcluded();

	static INIFile* m_ini;
	static UInt64 m_aliasHandle;
	static bool firstTime;

	std::vector<std::pair<RE::TESObjectREFR*, INIFile::SecondaryType>> m_candidates;

	static std::unordered_set<const RE::TESObjectREFR*> m_autoHarvestLock;
	static std::unordered_set<const RE::BGSLocation*> m_possiblePlayerHouse;

#if 0
	static VMClassRegistry* m_registry;
#endif
	static RecursiveLock m_searchLock;
	static bool m_threadStarted;
	static bool m_searchAllowed;
	static bool m_sneaking;
	static RE::TESObjectCELL* m_playerCell;
	static RE::BGSLocation* m_playerLocation;
	static RE::BGSKeyword* m_playerHouseKeyword;
	static bool m_carryAdjustedForCombat;
	static bool m_carryAdjustedForPlayerHome;
	static int m_currentCarryWeightChange;

#if 0
	static const BSFixedString critterIngredientEvent;
	static const BSFixedString carryWeightDeltaEvent;
	static const BSFixedString autoHarvestEvent;
	static const BSFixedString objectGlowEvent;
	static const BSFixedString objectGlowStopEvent;
	static const BSFixedString playerHouseCheckEvent;
#endif
	RE::TESObjectREFR* m_refr;

	static std::unordered_set<const RE::TESForm*> m_excludeLocations;
	static bool m_pluginSynced;
	static RecursiveLock m_lock;
	static std::unordered_map<const RE::TESObjectREFR*, std::chrono::time_point<std::chrono::high_resolution_clock>> m_glowExpiration;
	static std::unordered_map<const RE::TESObjectREFR*, std::vector<RE::TESForm*>> m_lootTargetByREFR;
	static std::unordered_set<const RE::BGSLocation*> m_locationCheckedForPlayerHouse;
};
