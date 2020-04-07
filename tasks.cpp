#include "PrecompiledHeaders.h"

#include "events.h"
#include "tasks.h"
#include "TESFormHelper.h"
#include "objects.h"
#include "dataCase.h"
#include "iniSettings.h"
#include "containerLister.h"
#include "basketfile.h"
#include "sound.h"
#include "debugs.h"
#include "papyrus.h"
#include "GridCellArrayHelper.h"

#include "TESQuestHelper.h"
#include "RE/SkyrimScript/RemoveItemFunctor.h"

#include <chrono>
#include <thread>

INIFile* SearchTask::m_ini = nullptr;
UInt64 SearchTask::m_aliasHandle = 0;

concurrency::critical_section SearchTask::m_lock;
std::unordered_set<const RE::TESObjectREFR*> SearchTask::m_pendingSearch;
std::unordered_map<const RE::TESObjectREFR*, std::chrono::time_point<std::chrono::high_resolution_clock>> SearchTask::m_glowExpiration;

bool SearchTask::GoodToGo()
{
	if (!m_registry)
	{
		m_registry = (*g_skyrimVM)->GetClassRegistry();
	}
	if (!m_ini)
	{
		m_ini = INIFile::GetInstance();
	}
	if (!m_aliasHandle)
	{
		RE::TESQuest* quest = GetTargetQuest(MODNAME, QUEST_ID);
		m_aliasHandle = (quest) ? TESQuestHelper(quest).GetAliasHandle(0) : 0;
	}
	return m_registry && m_ini && m_aliasHandle;
}

SearchTask::SearchTask(const std::vector<std::pair<RE::TESObjectREFR*, INIFile::SecondaryType>>& candidates)
	: m_candidates(candidates), m_refr(nullptr)
{
}

void SearchTask::Dispose()
{
	{
		// Scoped unlocker to ensure we safely clean up the entry locking this REFR from more SearchTask activity
		Unlocker cleanup(this);
	}
	delete this;
}

const BSFixedString SearchTask::critterIngredientEvent = "OnGetCritterIngredient";
const BSFixedString SearchTask::autoHarvestEvent = "OnAutoHarvest";
const BSFixedString SearchTask::objectGlowEvent = "OnObjectGlow";
const BSFixedString SearchTask::objectGlowStopEvent = "OnObjectGlowStop";
const BSFixedString SearchTask::playerHouseCheckEvent = "OnPlayerHouseCheck";

const int AutoHarvestSpamLimit = 10;

void SearchTask::Run()
{
	WindowsUtils::ScopedTimer elapsed("SearchTask::Run");
	DataCase* data = DataCase::GetInstance();
	RE::BSScript::Internal::VirtualMachine* vm(RE::BSScript::Internal::VirtualMachine::GetSingleton());

	for (const auto& candidate : m_candidates)
	{
		m_refr = candidate.first;
		INIFile::SecondaryType m_targetType(candidate.second);
		TESObjectREFRHelper refrEx(m_refr);
		// Scoped unlocker to ensure we safely clean up the entry locking this REFR from more SearchTask activity
		Unlocker cleanup(this);
		if (m_targetType == INIFile::SecondaryType::itemObjects)
		{
			ObjectType objType = refrEx.GetObjectType();
			std::string typeName = refrEx.GetTypeName();
			RE::TESObjectACTI* activator(m_refr->data.objectReference->As<RE::TESObjectACTI>());
			if (activator && objType == ObjectType::critter)
			{
				const RE::IngredientItem* ingredient(DataCase::GetInstance()->GetIngredientForCritter(activator));
				if (ingredient)
				{
#if _DEBUG
					_DMESSAGE("critter %s/0x%08x has ingredient %s/0x%08x", activator->GetName(), activator->formID,
						ingredient->GetName(), ingredient->formID);
#endif
					refrEx.SetIngredient(ingredient);
				}
				else
				{
					// trigger critter -> ingredient resolution and skip until it's resolved - pending resolve recorded using nullptr,
					// only trigger if not already pending
#if _DEBUG
					_DMESSAGE("resolve critter %s/0x%08x to ingredient", activator->GetName(), activator->formID);
#endif
					if (DataCase::GetInstance()->SetIngredientForCritter(activator, nullptr))
					{
						TriggerGetCritterIngredient();
					}
					continue;
				}
			}

#if _DEBUG
			_MESSAGE("typeName  %s", typeName.c_str());
			DumpReference(refrEx, typeName.c_str());
#endif

			if (objType == ObjectType::unknown)
			{
#if _DEBUG
				_DMESSAGE("block objType == ObjectType::unknown for 0x%08x", m_refr->data.objectReference->formID);
#endif
				data->lists.blockForm.insert(m_refr->data.objectReference);
				continue;
			}

			if (BasketFile::GetSingleton()->IsinList(BasketFile::EXCLUDELIST, m_refr->data.objectReference))
			{
#if _DEBUG
				_DMESSAGE("block m_refr->data.objectReference >= 1 for 0x%08x", m_refr->data.objectReference->formID);
#endif
				data->lists.blockForm.insert(m_refr->data.objectReference);
				continue;
			}

			bool needsGlow(false);
			bool skipLooting(false);

			bool needsFullQuestFlags(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "questObjectDefinition") != 0);
			if (refrEx.IsQuestItem(needsFullQuestFlags))
			{
				SInt32 questObjectGlow = static_cast<int>(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "questObjectGlow"));
				if (questObjectGlow == 1)
				{
					needsGlow = true;
				}

				SInt32 questObjectLoot = static_cast<int>(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "questObjectLoot"));
				if (questObjectLoot == 0)
					skipLooting = true;
			}

			if (objType == ObjectType::ammo)
			{
				RE::NiPoint3 pos = refrEx.m_ref->GetPosition();
				if (pos == RE::NiPoint3())
				{
#if _DEBUG
					_MESSAGE("err %0.2f", pos);
#endif
					DataCase::GetInstance()->BlockReference(m_refr);
					skipLooting = true;
				}

				if (data->lists.arrowCheck.count(m_refr) == 0)
				{
#if _DEBUG
					_MESSAGE("pick %0.2f", pos);
#endif
					data->lists.arrowCheck.insert(std::make_pair(m_refr, pos));
					skipLooting = true;
				}
				else
				{
					RE::NiPoint3 prev = data->lists.arrowCheck.at(m_refr);
					if (prev != pos)
					{
#if _DEBUG
						_MESSAGE("moving %0.2f  prev:%0.2f", pos, prev);
#endif
						data->lists.arrowCheck[m_refr] = pos;
						skipLooting = true;
					}
					else
					{
#if _DEBUG
						_MESSAGE("catch %0.2f", pos);
#endif
						data->lists.arrowCheck.erase(m_refr);
					}
				}
			}

			if (needsGlow)
			{
				TriggerObjectGlow();
			}
			SInt32 value = static_cast<SInt32>(m_ini->GetSetting(INIFile::autoharvest, m_targetType, typeName.c_str()));
			if (value == 0)
			{
#if _DEBUG
				_MESSAGE("BLock REFR : value == 0 for 0x%08x", m_refr->data.objectReference->formID);
#endif
				DataCase::GetInstance()->BlockReference(m_refr);
				skipLooting = true;
			}
			if (skipLooting)
				continue;

			if (refrEx.ValueWeightTooLowToLoot(m_ini))
			{
#if _DEBUG
				_MESSAGE("value/weight ratio too low, skip");
#endif
				data->lists.blockRefr.insert(m_refr);
				continue;
			}

			bool isSilent = (m_ini->GetSetting(INIFile::autoharvest, m_targetType, typeName.c_str()) == 1);

#if _DEBUG
			_MESSAGE("Enqueue AutoHarvest event");
#endif
			// don't let the backlog of messages get too large, it's about 1 per second
			TriggerAutoHarvest(objType, refrEx.GetItemCount(), isSilent || PendingAutoHarvest() > AutoHarvestSpamLimit);
		}
		else if (m_targetType == INIFile::SecondaryType::containers || m_targetType == INIFile::SecondaryType::deadbodies)
		{
#if _DEBUG
			_DMESSAGE("scanning container/body %s/0x%08x", refrEx.m_ref->GetName(), refrEx.m_ref->formID);
			DumpContainer(refrEx);
#endif
			std::unordered_map<RE::TESForm*, int> lootableItems;

			SInt32 questitemDefinition = static_cast<int>(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "questObjectDefinition"));

			bool hasQuestObject = false;
			bool hasEnchantItem = false;
			ContainerLister fnOption(refrEx.m_ref, questitemDefinition);

			if (!fnOption.GetOrCheckContainerForms(lootableItems, hasQuestObject, hasEnchantItem))
				continue;

			bool needsGlow(false);
			bool skipLooting(false);
			if (m_targetType == INIFile::SecondaryType::containers)
			{
				if (IsContainerLocked(m_refr))
				{
					SInt32 lockedChestGlow = static_cast<int>(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "lockedChestGlow"));
					if (lockedChestGlow == 1)
					{
						needsGlow = true;
					}

					skipLooting = skipLooting || (static_cast<int>(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "lockedChestLoot")) == 0);
				}

				if (IsBossContainer(m_refr))
				{
					SInt32 bossChestGlow = static_cast<int>(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "bossChestGlow"));
					if (bossChestGlow == 1)
					{
						needsGlow = true;
					}

					skipLooting = skipLooting || (static_cast<int>(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "bossChestLoot")) == 0);
				}
			}

			SInt32 questObjectGlow = -1;
			if (hasQuestObject)
			{
				questObjectGlow = static_cast<int>(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "questObjectGlow"));
				if (questObjectGlow == 1)
				{
					needsGlow = true;
				}

				skipLooting = skipLooting || (static_cast<int>(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "questObjectLoot")) == 0);
			}

			SInt32 enchantItemGlow = -1;
			if (hasEnchantItem)
			{
				enchantItemGlow = static_cast<int>(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "enchantItemGlow"));
				if (enchantItemGlow == 1)
				{
					needsGlow = true;
				}
			}
			if (needsGlow)
			{
				// TODO get this to compile vm->SendEvent(m_aliasHandle, objectGlowEvent, RE::MakeFunctionArguments(m_refr, subEvent_glowObject));
				TriggerObjectGlow();
			}
			if (skipLooting)
				continue;

			// when we get to the point where looting is confirmed, block the reference to
			// avoid re-looting without a player cell or config change
#if _DEBUG
			_DMESSAGE("block looted container %s/0x%08x", refrEx.m_ref->GetName(), refrEx.m_ref->formID);
#endif
			DataCase::GetInstance()->BlockReference(refrEx.m_ref);

			std::vector<std::pair<RE::TESBoundObject*, int>> targets;
			targets.reserve(lootableItems.size());
			std::vector<bool> notifications;
			notifications.reserve(lootableItems.size());
			int count(0);
			for (auto targetSize : lootableItems)
			{
				RE::TESForm* target(targetSize.first);
				if (!target)
					continue;

				TESFormHelper itemEx(target);

				if (BasketFile::GetSingleton()->IsinList(BasketFile::EXCLUDELIST, target))
				{
#if _DEBUG
					_DMESSAGE("block due to BasketFile exclude-list for 0x%08x", target->formID);
#endif
					data->lists.blockForm.insert(target);
					continue;
				}

				ObjectType objType = ClassifyType(itemEx.m_form);
				std::string typeName = GetObjectTypeName(objType);

				SInt32 value = static_cast<SInt32>(m_ini->GetSetting(INIFile::autoharvest, m_targetType, typeName.c_str()));
				if (value == 0)
				{
#if _DEBUG
					_DMESSAGE("block - typename %s excluded for 0x%08x", typeName.c_str(), target->formID);
#endif
					data->lists.blockForm.insert(target);
					continue;
				}
				bool isSilent = (value == 1);

				if (itemEx.ValueWeightTooLowToLoot(m_ini))
				{
#if _DEBUG
					_DMESSAGE("block - v/w excludes for 0x%08x", target->formID);
#endif
					data->lists.blockForm.insert(target);
					continue;
				}

				const RE::TESBoundObject* bound(target->As<RE::TESBoundObject>());
				if (!bound)
					continue;

				targets.push_back(std::make_pair(const_cast<RE::TESBoundObject*>(bound), targetSize.second));
				notifications.push_back(!isSilent);
#if _DEBUG
				_DMESSAGE("get %s from container %s/0x%08x", itemEx.m_form->GetName(),
					refrEx.m_ref->GetName(), refrEx.m_ref->formID);
#endif
				++count;
			}

			if (count > 0)
			{
				bool animate(false);
				if (m_targetType == INIFile::SecondaryType::containers)
				{
					int disableContainerAnimation = static_cast<int>(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "ContainerAnimation"));
					animate = disableContainerAnimation != 1 && refrEx.GetTimeController();
				}
				TriggerContainerLootMany(targets, notifications, animate);
			}
		}
	}
}

VMClassRegistry* SearchTask::m_registry = nullptr;
concurrency::critical_section SearchTask::m_searchLock;
bool SearchTask::m_threadStarted = false;
bool SearchTask::m_searchAllowed = false;
bool SearchTask::m_sneaking = false;
RE::TESObjectCELL* SearchTask::m_playerCell = nullptr;
RE::BGSLocation* SearchTask::m_playerLocation = nullptr;
std::unordered_set<const RE::BGSLocation*> SearchTask::m_locationCheckedForPlayerHouse;
RE::BGSKeyword* SearchTask::m_playerHouseKeyword(nullptr);
bool SearchTask::m_carryAdjustedForCombat = false;
bool SearchTask::m_carryAdjustedForPlayerHome = false;

void SearchTask::SetPlayerHouseKeyword(RE::BGSKeyword* keyword)
{
	m_playerHouseKeyword = keyword;
}

void SearchTask::Start()
{
	std::thread([]()
	{
#if _DEBUG
		_DMESSAGE("starting thread");
#endif
		while (true)
		{
			double delay(INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::autoharvest,
				INIFile::SecondaryType::config, "IntervalSeconds"));
			StartSearch(static_cast<SInt32>(INIFile::PrimaryType::autoharvest));
#if _DEBUG
			_DMESSAGE("wait for %d milliseconds", static_cast<long long>(delay * 1000.0));
#endif
			auto nextRunTime = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(static_cast<long long>(delay * 1000.0));
			std::this_thread::sleep_until(nextRunTime);
		}
	}).detach();
}

float InfiniteWeight = 100000.0;

SInt32 SearchTask::StartSearch(SInt32 type1)
{
	WindowsUtils::ScopedTimer elapsed("SearchTask::StartSearch");
	if (!GoodToGo())
	{
#if _DEBUG
		_DMESSAGE("Prerequisites not in place yet");
#endif
		return 0;
	}

	// disable auto-looting if we are inside player house - player 'current location' may be validly empty
	RE::PlayerCharacter* player(RE::PlayerCharacter::GetSingleton());
	if (!player)
	{
#if _DEBUG
		_DMESSAGE("PlayerCharacter not available");
#endif
		return 0;
	}

	RE::BGSLocation* playerLocation(RE::PlayerCharacter::GetSingleton()->currentLocation);
	if (IsPossiblePlayerHouse(playerLocation))
	{
#if _DEBUG
		_MESSAGE("Player location %s pending user check for player-house exclusion", playerLocation ? playerLocation->GetName() : "unnamed");
#endif
		return 0;
	}
	if (playerLocation != m_playerLocation)
	{
#if _DEBUG
		_MESSAGE("Player entered new location %s", playerLocation ? playerLocation->GetName() : "unnamed");
#endif
		m_playerLocation = playerLocation;
		// Player changed location - check if it is a player house
		if (m_playerLocation && m_locationCheckedForPlayerHouse.count(m_playerLocation) == 0)
		{
			m_locationCheckedForPlayerHouse.insert(m_playerLocation);
			if (m_playerLocation->HasKeyword(m_playerHouseKeyword))
			{
#if _DEBUG
				_MESSAGE("Player House %s requires user confirmation before looting", m_playerLocation->GetName());
#endif
				// lock this location until dialog has been handled
				LockPossiblePlayerHouse(m_playerLocation);
				PlayerHouseCheckEventFunctor argHouseCheck(reinterpret_cast<TESForm*>(m_playerLocation));
				m_registry->QueueEvent(m_aliasHandle, &playerHouseCheckEvent, &argHouseCheck);
				return 0;
			}
		}
	}

	if (!IsAllowed())
	{
#if _DEBUG
		_DMESSAGE("search disallowed");
#endif
		return 0;
	}

	if (IsLocationExcluded())
	{
#if _DEBUG
		_DMESSAGE("Player location excluded");
#endif
		return 0;
	}

	if (!RE::PlayerControls::GetSingleton() || !RE::PlayerControls::GetSingleton()->IsActivateControlsEnabled())
	{
#if _DEBUG
		_DMESSAGE("player controls disabled");
#endif
		return 0;
	}

	// By inspection, the stack has steady state size of 1. Opening application/inventory adds 1 each,
	// opening console adds 2. So this appars to be a catch-all for those conditions.
	if (!RE::UI::GetSingleton())
	{
#if _DEBUG
		_DMESSAGE("UI inaccessible");
		return 0;
#endif
	}
	size_t count(RE::UI::GetSingleton()->menuStack.size());
    if (count > 1)
	{
#if _DEBUG
		_DMESSAGE("console and/or menu(s) active, delta to menu-stack size = %d", count);
#endif
		return 0;
	}

#if 0
	// Spell for harvesting, WIP
	elseif (Game.GetPlayer().HasMagicEffectWithKeyword(CastKwd)); for spell(wip)
		s_updateDelay = true
#endif

	INIFile* ini = INIFile::GetInstance();
	if (!ini)
		return 0;

	// Respect encumbrance quality of life settings
	bool playerInOwnHouse(m_playerLocation && m_playerLocation->HasKeyword(m_playerHouseKeyword));
	if (ini->GetSetting(INIFile::PrimaryType::common, INIFile::config, "UnencumberedInPlayerHome") != 0.0)
	{
		// when location changes to/from player house, adjust carry weight accordingly
		if (playerInOwnHouse != m_carryAdjustedForPlayerHome)
		{
			RE::PlayerCharacter::GetSingleton()->ModActorValue(RE::ActorValue::kCarryWeight, playerInOwnHouse ? InfiniteWeight : -InfiniteWeight);
			m_carryAdjustedForPlayerHome = playerInOwnHouse;
#if _DEBUG
			_MESSAGE("Carry weight after in-player-home adjustment %f", RE::PlayerCharacter::GetSingleton()->GetActorValue(RE::ActorValue::kCarryWeight));
#endif
		}
	}
	bool playerInCombat(RE::PlayerCharacter::GetSingleton()->IsInCombat());
	if (ini->GetSetting(INIFile::PrimaryType::common, INIFile::config, "UnencumberedInCombat") != 0.0)
	{
		// when state changes in/out of combat, adjust carry weight accordingly
		if (playerInCombat != m_carryAdjustedForCombat)
		{
			RE::PlayerCharacter::GetSingleton()->ModActorValue(RE::ActorValue::kCarryWeight, playerInCombat ? InfiniteWeight : -InfiniteWeight);
			m_carryAdjustedForCombat = playerInCombat;
#if _DEBUG
			_MESSAGE("Carry weight after in-combat adjustment %f", RE::PlayerCharacter::GetSingleton()->GetActorValue(RE::ActorValue::kCarryWeight));
#endif
		}
	}

	UInt32 result = 0;

	DataCase* data = DataCase::GetInstance();
	if (!data)
		return 0;

	INIFile::PrimaryType first = static_cast<INIFile::PrimaryType>(type1);
	if (!ini->IsType(first))
		return 0;

	RE::TES* tes(RE::TES::GetSingleton());
	GridCellArray* cells(nullptr);
	GridCellArrayHelper grid(cells, ini->GetRadius(first));

	const int disableInCombat = static_cast<int>(ini->GetSetting(first, INIFile::config, "disableInCombat"));
	if (disableInCombat == 1 && playerInCombat)
	{
#if _DEBUG
		_MESSAGE("disableInCombat %d", disableInCombat);
#endif
		return 0;
	}

	const int disableDrawingWeapon = static_cast<int>(ini->GetSetting(first, INIFile::config, "disableDrawingWeapon"));
	if (disableDrawingWeapon == 1 && RE::PlayerCharacter::GetSingleton()->IsWeaponDrawn())
	{
#if _DEBUG
		_MESSAGE("disableDrawingWeapon %d", disableDrawingWeapon);
#endif
		return 0;
	}

	bool sneaking = RE::PlayerCharacter::GetSingleton()->IsSneaking();
	bool unblockAll(false);
	// Reset blocked lists if sneak state or player cell has changed
	if (m_sneaking != sneaking)
	{
		m_sneaking = sneaking;
		unblockAll = true;
	}
	// Player cell should never be empty
	RE::TESObjectCELL* playerCell(RE::PlayerCharacter::GetSingleton()->parentCell);
	if (!playerCell)
	{
#if _DEBUG
		_MESSAGE("Player location not yet set up");
#endif
		return 0;
	}
	if (playerCell != m_playerCell)
	{
		unblockAll = true;
		m_playerCell = playerCell;
	}
	if (unblockAll)
	{
		papyrus::UnblockEverything(nullptr);
	}

	if (m_playerLocation && !m_playerLocation->IsLoaded())
	{
#if _DEBUG
		_MESSAGE("Location %s not loaded", m_playerLocation->GetName());
#endif
		return 0;
	}

	int crimeCheck = static_cast<int>(ini->GetSetting(first, INIFile::config, (sneaking) ? "crimeCheckSneaking" : "crimeCheckNotSneaking"));
	int belongingsCheck = static_cast<int>(ini->GetSetting(first, INIFile::config, "playerBelongingsLoot"));

	std::vector<RE::TESObjectREFR*> refs;

	result = grid.GetReferences(&refs);
	if (result == 0)
		return 0;

	std::vector<std::pair<RE::TESObjectREFR*, INIFile::SecondaryType>> candidates;
	candidates.reserve(refs.size());
	for (RE::TESObjectREFR* refr : refs)
	{
		if (!refr)
			continue;
		INIFile::SecondaryType second = INIFile::SecondaryType::itemObjects;
		RE::TESObjectREFR* ashPileRefr(nullptr);
		if (refr->data.objectReference->As<RE::TESContainer>())
		{
			if (ini->GetSetting(INIFile::PrimaryType::common, INIFile::config, "EnableLootContainer") == 0.0)
				continue;
			second = INIFile::SecondaryType::containers;
		}
		else if (refr->data.objectReference->As<RE::Actor>())
		{
			if (ini->GetSetting(INIFile::PrimaryType::common, INIFile::config, "enableLootDeadbody") == 0.0 || !refr->IsDead(true))
				continue;

			ActorHelper actorEx(refr->data.objectReference->As<RE::Actor>());
			if (actorEx.IsPlayerFollower() || actorEx.IsEssential() || !actorEx.IsSummonable())
			{
				DataCase::GetInstance()->BlockReference(refr);
				continue;
			}

			second = INIFile::SecondaryType::containers;
		}
		else if (refr->data.objectReference->As<RE::TESObjectACTI>() && (ashPileRefr = GetAshPile(refr)))
		{
			if (ini->GetSetting(INIFile::PrimaryType::common, INIFile::config, "enableLootDeadbody") == 0.0)
				continue;
			second = INIFile::SecondaryType::containers;
		}
		else if (ini->GetSetting(INIFile::PrimaryType::common, INIFile::config, "enableAutoHarvest") == 0.0)
			continue;

		bool ownership = false;
		if (crimeCheck == 1.0)
		{
			ownership = refr->IsOffLimits();

#if _DEBUG
			const char* str = ownership ? "true" : "false";
			_MESSAGE("***1 crimeCheck: %d ownership: %s", crimeCheck, str);
#endif

		}
		else if (crimeCheck == 2.0)
		{
			ownership = refr->IsOffLimits();

#if _DEBUG
			const char* str = ownership ? "true" : "false";
			_MESSAGE("***2 crimeCheck: %d ownership: %s", crimeCheck, str);
#endif


			if (!ownership)
			{
				TESObjectREFRHelper refrEx(refr);
				ownership = refrEx.IsPlayerOwned();
#if _DEBUG
				const char* str = ownership ? "true" : "false";
				_MESSAGE("***2-1 crimeCheck: %d ownership: %s", crimeCheck, str);
#endif
			}

			if (!ownership)
			{
				TESObjectCELLHelper cellHelper(RE::PlayerCharacter::GetSingleton()->parentCell);
				if (cellHelper.m_cell)
				{
					ownership = cellHelper.IsPlayerOwned();

#if _DEBUG
					const char* str = ownership ? "true" : "false";
					_MESSAGE("***2-2 crimeCheck: %d ownership: %s", crimeCheck, str);
#endif
				}
			}
		}

		if (!ownership && belongingsCheck == 0)
		{
			TESObjectREFRHelper refrEx(refr);
			ownership = refrEx.IsPlayerOwned();

#if _DEBUG
			const char* str = ownership ? "true" : "false";
			_MESSAGE("***3 crimeCheck: %d ownership: %s", crimeCheck, str);
#endif
		}

		if (ownership)
		{
			DataCase::GetInstance()->BlockReference(refr);
			continue;
		}

		if (ashPileRefr)
		{
			refr = ashPileRefr;
		}

		// Acquire token to allow search to be kicked off here. If a task is already pending do not
		// create another one.
		if (SearchTask::LockREFR(refr))
		{
			candidates.push_back(std::make_pair(refr, second));
		}
	}
	SKSE::GetTaskInterface()->AddTask(new SearchTask(candidates));
	return result;
}

void SearchTask::Allow()
{
	concurrency::critical_section::scoped_lock guard(m_searchLock);
	m_searchAllowed = true;
	if (!m_threadStarted)
	{
		// Start the thread when we are first allowed to search
		m_threadStarted = true;
		SearchTask::Start();
	}
}
void SearchTask::Disallow()
{
	concurrency::critical_section::scoped_lock guard(m_searchLock);
	m_searchAllowed = false;
}
bool SearchTask::IsAllowed()
{
	concurrency::critical_section::scoped_lock guard(m_searchLock);
	return m_searchAllowed;
}

void SearchTask::TriggerGetCritterIngredient()
{
	CritterIngredientEventFunctor args(reinterpret_cast<TESObjectREFR*>(m_refr));
	m_registry->QueueEvent(m_aliasHandle, &critterIngredientEvent, &args);
}

std::unordered_set<const RE::TESObjectREFR*> SearchTask::m_autoHarvestLock;

void SearchTask::TriggerAutoHarvest(const ObjectType objType, int itemCount, bool isSilent)
{
    // Event handler in Papyrus script unlocks the task - do not issue multiple concurrent events on the same REFR
	if (!LockAutoHarvest(m_refr))
		return;
	LootEventFunctor args(reinterpret_cast<TESObjectREFR*>(m_refr), static_cast<SInt32>(objType), itemCount, isSilent);
	m_registry->QueueEvent(m_aliasHandle, &autoHarvestEvent, &args);
}

bool SearchTask::LockAutoHarvest(const RE::TESObjectREFR* refr)
{
	concurrency::critical_section::scoped_lock guard(m_lock);
	if (!refr)
		return false;
	return (m_autoHarvestLock.insert(refr)).second;
}

bool SearchTask::UnlockAutoHarvest(const RE::TESObjectREFR* refr)
{
	concurrency::critical_section::scoped_lock guard(m_lock);
	if (!refr)
		return false;
	return m_autoHarvestLock.erase(refr) > 0;
}

bool SearchTask::IsLockedForAutoHarvest(const RE::TESObjectREFR* refr)
{
	concurrency::critical_section::scoped_lock guard(m_lock);
	return (refr && m_autoHarvestLock.count(refr));
}

size_t SearchTask::PendingAutoHarvest()
{
	concurrency::critical_section::scoped_lock guard(m_lock);
	return m_autoHarvestLock.size();
}

std::unordered_set<const RE::BGSLocation*> SearchTask::m_possiblePlayerHouse;
bool SearchTask::LockPossiblePlayerHouse(const RE::BGSLocation* location)
{
	concurrency::critical_section::scoped_lock guard(m_lock);
	if (!location)
		return false;
	return (m_possiblePlayerHouse.insert(location)).second;
}

bool SearchTask::UnlockPossiblePlayerHouse(const RE::BGSLocation* location)
{
	concurrency::critical_section::scoped_lock guard(m_lock);
	if (!location)
		return false;
	return m_possiblePlayerHouse.erase(location) > 0;
}

bool SearchTask::IsPossiblePlayerHouse(const RE::BGSLocation* location)
{
	concurrency::critical_section::scoped_lock guard(m_lock);
	return (location && m_possiblePlayerHouse.count(location));
}

std::unordered_set<const RE::TESForm*> SearchTask::m_excludeLocations;

void SearchTask::AddLocationToExcludeList(const RE::TESForm* location)
{
	m_excludeLocations.insert(location);
}

void SearchTask::DropLocationFromExcludeList(const RE::TESForm* location)
{
	m_excludeLocations.insert(location);
}

bool SearchTask::IsLocationExcluded()
{
	const RE::TESForm* currentLocation(RE::PlayerCharacter::GetSingleton()->currentLocation);
	return m_excludeLocations.count(currentLocation);
}

void SearchTask::TriggerContainerLootMany(const std::vector<std::pair<RE::TESBoundObject*, int>>& targets, const std::vector<bool>& notifications, const bool animate)
{
	if (!m_refr)
		return;
	auto target(targets.cbegin());
	auto notification(notifications.cbegin());
	std::vector<RE::TESForm*> forms;
	forms.reserve(targets.size());
	for (; target != targets.cend(); ++target, ++notification)
	{
		RE::ExtraDataList* xList(nullptr);
		auto first(m_refr->extraList.cbegin());
		if (first != m_refr->extraList.cend())
		{
			xList = &m_refr->extraList;
		}
		RE::ObjectRefHandle handle(m_refr->RemoveItem(target->first, target->second, RE::ITEM_REMOVE_REASON::kRemove, xList, RE::PlayerCharacter::GetSingleton()));
		RE::PlayerCharacter::GetSingleton()->PlayPickUpSound(target->first, true, false);
		if (animate)
		{
			m_refr->PlayAnimation("Close", "Open");
		}

		if (*notification)
		{
			std::string notificationText;
			if (target->second > 1)
			{
				static RE::BSFixedString multiActivate(papyrus::GetTranslation(nullptr, RE::BSFixedString("$AHSE_ACTIVATE(COUNT)_MSG")));
				if (!multiActivate.empty())
				{
					notificationText = multiActivate;
					Replace(notificationText, "{ITEMNAME}", target->first->GetName());
					std::ostringstream intStr;
					intStr << target->second;
					Replace(notificationText, "{COUNT}", intStr.str());
				}
			}
			else
			{
				static RE::BSFixedString singleActivate(papyrus::GetTranslation(nullptr, RE::BSFixedString("$AHSE_ACTIVATE_MSG")));
				if (!singleActivate.empty())
				{
					notificationText = singleActivate;
					Replace(notificationText, "{ITEMNAME}", target->first->GetName());
				}
			}
			if (!notificationText.empty())
			{
				RE::DebugNotification(notificationText.c_str());
			}
		}
	}
}

const unsigned int ObjectGlowDuration = 15;

void SearchTask::TriggerObjectGlow()
{
	// TODO get this to compile
	// vm->SendEvent(m_aliasHandle, objectGlowEvent, RE::MakeFunctionArguments(m_refr, subEvent_glowObject));

	// only send the glow event once per N seconds. This will retrigger on every pass but once we are out of
	// range no more glowing will be triggered. The item remains in the list until we change cell but there should
	// never be so many in a cell that this is a problem.
	concurrency::critical_section::scoped_lock guard(m_lock);
	const auto existingGlow(m_glowExpiration.find(m_refr));
	auto currentTime(std::chrono::high_resolution_clock::now());
	if (existingGlow != m_glowExpiration.cend() && existingGlow->second > currentTime)
		return;
	auto expiry = currentTime + std::chrono::milliseconds(static_cast<long long>(ObjectGlowDuration * 1000.0));
	m_glowExpiration[m_refr] = expiry;

	ObjectGlowEventFunctor argGlow(reinterpret_cast<TESObjectREFR*>(m_refr), ObjectGlowDuration);
	m_registry->QueueEvent(m_aliasHandle, &objectGlowEvent, &argGlow);
}

void SearchTask::StopObjectGlow(const RE::TESObjectREFR* refr)
{
	// TODO get this to compile
	// vm->SendEvent(m_aliasHandle, objectGlowEvent, RE::MakeFunctionArguments(m_refr, subEvent_glowObject));
	ObjectGlowEventFunctor argGlow(reinterpret_cast<TESObjectREFR*>(const_cast<RE::TESObjectREFR*>(refr)), 0);
	m_registry->QueueEvent(m_aliasHandle, &objectGlowStopEvent, &argGlow);

	concurrency::critical_section::scoped_lock guard(m_lock);
	m_glowExpiration.erase(refr);
}

void SearchTask::UnlockAll()
{
#if _DEBUG
	_MESSAGE("Unlock task-pending REFRs");
#endif
	concurrency::critical_section::scoped_lock guard(m_lock);
	// unblock all blocked auto-harvest objects
	m_autoHarvestLock.clear();
	// unblock possible player house checks
	m_possiblePlayerHouse.clear();
}

// Stop objects glowing and clean up the list
void SearchTask::UnglowAll()
{
	concurrency::critical_section::scoped_lock guard(m_lock);
	for (auto glowingObject : m_glowExpiration)
	{
		StopObjectGlow(glowingObject.first);
	}
	m_glowExpiration.clear();
}

// returns true if an entry was added
bool SearchTask::LockREFR(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return false;
	concurrency::critical_section::scoped_lock guard(m_lock);
	bool added(m_pendingSearch.insert(refr).second);
#if _DEBUG
	_DMESSAGE("LockREFR %d for 0x%08x", added, refr->formID);
#endif
	return added;
}

// returns true if an entry was removed, clearing the input
bool SearchTask::UnlockREFR()
{
	if (!m_refr)
		return false;
	concurrency::critical_section::scoped_lock guard(m_lock);
	bool removed(m_pendingSearch.erase(m_refr) > 0);
#if _DEBUG
	_DMESSAGE("UnlockREFR %d for 0x%08x", removed, m_refr->formID);
#endif
	m_refr = nullptr;
	return removed;
}

void tasks::Init()
{
	WindowsUtils::ScopedTimer elapsed("tasks::Init");
	papyrus::UnblockEverything(nullptr);
	DataCase::GetInstance()->BuildList();
}
