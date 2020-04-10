#pragma once

#include "skse64/gamethreads.h"  // TaskDelegate
#include "skse64/PapyrusVM.h"

#include "objects.h"
#include "TESFormHelper.h"
#include "iniSettings.h"

#include <unordered_set>
#include <concrt.h>

#define QUEST_ID	0x01D8C
#define NGEF_ID		0x00D64

class TaskBase : public TaskDelegate
{
public:
	TaskBase(void);

};

class SearchTask : public TaskDelegate
{
public:
	SearchTask(const std::vector<std::pair<RE::TESObjectREFR*, INIFile::SecondaryType>>& candidates);
	virtual void Run() override;
	virtual void Dispose() override;

	static bool LockREFR(const RE::TESObjectREFR* refr);
	bool UnlockREFR();
	static void UnlockAll();

	static bool IsLockedForAutoHarvest(const RE::TESObjectREFR* refr);
	static size_t PendingAutoHarvest();
	static bool UnlockAutoHarvest(const RE::TESObjectREFR* refr);
	static bool IsPossiblePlayerHouse(const RE::BGSLocation* location);
	static bool UnlockPossiblePlayerHouse(const RE::BGSLocation* location);

	static void ResetExcludedLocations();
	static void AddLocationToExcludeList(const RE::TESForm* location);
	static void DropLocationFromExcludeList(const RE::TESForm* location);

	static void Start();
	static void Allow();
	static void Disallow();
	static bool IsAllowed();
	static SInt32 StartSearch(SInt32 type1);
	static void SetPlayerHouseKeyword(RE::BGSKeyword* keyword);

	class Unlocker {
	public:
		Unlocker(SearchTask* task) : m_task(task) {}
		~Unlocker() { m_task->UnlockREFR(); }
	private:
		SearchTask* m_task;
	};

private:
	static bool GoodToGo();

	void TriggerGetCritterIngredient();
	void TriggerAutoHarvest(const ObjectType objType, int itemCount, bool isSilent);
	static bool LockAutoHarvest(const RE::TESObjectREFR* refr);
	static bool LockPossiblePlayerHouse(const RE::BGSLocation* location);

	void TriggerContainerLootMany(const std::vector<std::pair<RE::TESBoundObject*, int>>& targets, const std::vector<bool>& notifications, const bool animate);
	void TriggerObjectGlow();
	static void StopObjectGlow(const RE::TESObjectREFR* refr);
	static void UnglowAll();

	static bool IsLocationExcluded();

	static INIFile* m_ini;
	static UInt64 m_aliasHandle;

	std::vector<std::pair<RE::TESObjectREFR*, INIFile::SecondaryType>> m_candidates;

	static std::unordered_set<const RE::TESObjectREFR*> m_autoHarvestLock;
	static std::unordered_set<const RE::BGSLocation*> m_possiblePlayerHouse;

	static VMClassRegistry* m_registry;
	static concurrency::critical_section m_searchLock;
	static bool m_threadStarted;
	static bool m_searchAllowed;
	static bool m_sneaking;
	static RE::TESObjectCELL* m_playerCell;
	static RE::BGSLocation* m_playerLocation;
	static RE::BGSKeyword* m_playerHouseKeyword;
	static bool m_carryAdjustedForCombat;
	static bool m_carryAdjustedForPlayerHome;
	static float m_currentCarryWeightChange;

	static const BSFixedString critterIngredientEvent;
	static const BSFixedString autoHarvestEvent;
	static const BSFixedString objectGlowEvent;
	static const BSFixedString objectGlowStopEvent;
	static const BSFixedString playerHouseCheckEvent;
	RE::TESObjectREFR* m_refr;

	static std::unordered_set<const RE::TESForm*> m_excludeLocations;
	static concurrency::critical_section m_lock;
	static std::unordered_set<const RE::TESObjectREFR*> m_pendingSearch;
	static std::unordered_map<const RE::TESObjectREFR*, std::chrono::time_point<std::chrono::high_resolution_clock>> m_glowExpiration;
	static std::unordered_map<const RE::TESObjectREFR*, std::vector<RE::TESForm*>> m_lootTargetByREFR;
	static std::unordered_set<const RE::BGSLocation*> m_locationCheckedForPlayerHouse;
};

namespace tasks
{
	void Init(void);
}
