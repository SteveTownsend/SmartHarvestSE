#include "PrecompiledHeaders.h"

#include "tasks.h"
#include "TESFormHelper.h"
#include "objects.h"
#include "dataCase.h"
#include "iniSettings.h"
#include "containerLister.h"
#include "basketfile.h"
#include "debugs.h"
#include "papyrus.h"
#include "PlayerCellHelper.h"

#include "TESQuestHelper.h"
#include "RE/SkyrimScript/RemoveItemFunctor.h"

#include <chrono>
#include <thread>

INIFile* SearchTask::m_ini = nullptr;
UInt64 SearchTask::m_aliasHandle = 0;

RecursiveLock SearchTask::m_lock;
std::unordered_map<const RE::TESObjectREFR*, std::chrono::time_point<std::chrono::high_resolution_clock>> SearchTask::m_glowExpiration;

bool SearchTask::GoodToGo()
{
	if (!m_aliasHandle)
	{
		RE::TESQuest* quest = GetTargetQuest(MODNAME, QUEST_ID);
		m_aliasHandle = (quest) ? TESQuestHelper(quest).GetAliasHandle(0) : 0;
	}
	return m_aliasHandle;
}

SearchTask::SearchTask(const std::vector<std::pair<RE::TESObjectREFR*, INIFile::SecondaryType>>& candidates)
	: m_candidates(candidates), m_refr(nullptr)
{
}

const RE::BSFixedString SearchTask::critterIngredientEvent = "OnGetCritterIngredient";
const RE::BSFixedString SearchTask::carryWeightDeltaEvent = "OnCarryWeightDelta";
const RE::BSFixedString SearchTask::autoHarvestEvent = "OnAutoHarvest";
const RE::BSFixedString SearchTask::objectGlowEvent = "OnObjectGlow";
const RE::BSFixedString SearchTask::objectGlowStopEvent = "OnObjectGlowStop";

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
		if (m_targetType == INIFile::SecondaryType::itemObjects)
		{
			ObjectType objType = refrEx.GetObjectType();
			std::string typeName = refrEx.GetTypeName();
			// Various form types contain an ingredient that is the final lootable item - resolve here
			const RE::TESForm* lootable(DataCase::GetInstance()->GetLootableForProducer(m_refr->data.objectReference));
			if (lootable)
			{
#if _DEBUG
				_DMESSAGE("producer %s/0x%08x has lootable %s/0x%08x", m_refr->data.objectReference->GetName(), m_refr->data.objectReference->formID,
					lootable->GetName(), lootable->formID);
#endif
				refrEx.SetLootable(lootable);
			}
			else if (objType == ObjectType::critter)
			{
				// trigger critter -> ingredient resolution and skip until it's resolved - pending resolve recorded using nullptr,
				// only trigger if not already pending
#if _DEBUG
				_DMESSAGE("resolve critter %s/0x%08x to ingredient", m_refr->data.objectReference->GetName(), m_refr->data.objectReference->formID);
#endif
				if (DataCase::GetInstance()->SetLootableForProducer(m_refr->data.objectReference, nullptr))
				{
					TriggerGetCritterIngredient();
				}
				continue;
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
				data->BlockForm(m_refr->data.objectReference);
				continue;
			}

			if (objType == ObjectType::manualLoot &&
				m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "ManualLootTargetNotify") != 0)
			{
				// notify about these, just once
				std::string notificationText;
				static RE::BSFixedString manualLootText(papyrus::GetTranslation(nullptr, RE::BSFixedString("$AHSE_MANUAL_LOOT_MSG")));
				if (!manualLootText.empty())
				{
					notificationText = manualLootText;
					Replace(notificationText, "{ITEMNAME}", refrEx.m_ref->GetName());
					if (!notificationText.empty())
					{
						RE::DebugNotification(notificationText.c_str());
					}
				}
#if _DEBUG
				_DMESSAGE("notify, then block objType == ObjectType::manualLoot for 0x%08x", m_refr->data.objectReference->formID);
#endif
				data->BlockForm(m_refr->data.objectReference);
				continue;
			}

			if (BasketFile::GetSingleton()->IsinList(BasketFile::EXCLUDELIST, m_refr->data.objectReference))
			{
#if _DEBUG
				_DMESSAGE("block m_refr->data.objectReference >= 1 for 0x%08x", m_refr->data.objectReference->formID);
#endif
				data->BlockForm(m_refr->data.objectReference);
				continue;
			}

			bool needsGlow(false);
			bool skipLooting(false);

			bool needsFullQuestFlags(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "questObjectScope") != 0);
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
				skipLooting = skipLooting || data->CheckAmmoLootable(m_refr);
			}

			if (needsGlow)
			{
				TriggerObjectGlow(m_refr);
			}
			if (!skipLooting)
			{
				LootingType lootingType = LootingTypeFromIniSetting(m_ini->GetSetting(INIFile::autoharvest, m_targetType, typeName.c_str()));
				if (lootingType == LootingType::LeaveBehind)
				{
#if _DEBUG
					_MESSAGE("Block REFR : LeaveBehind for 0x%08x", m_refr->data.objectReference->formID);
#endif
					data->BlockReference(m_refr);
					skipLooting = true;
				}
				else if (LootingDependsOnValueWeight(lootingType, objType) && TESFormHelper(m_refr->data.objectReference).ValueWeightTooLowToLoot(m_ini))
				{
#if _DEBUG
					_DMESSAGE("block - v/w excludes harvest for 0x%08x", m_refr->data.objectReference->formID);
#endif
					data->BlockForm(m_refr->data.objectReference);
					skipLooting = true;
				}
			}
			if (skipLooting)
				continue;

			bool isSilent = !LootingRequiresNotification(
				LootingTypeFromIniSetting(m_ini->GetSetting(INIFile::autoharvest, m_targetType, typeName.c_str())));
#if _DEBUG
			_MESSAGE("Enqueue AutoHarvest event");
#endif
			// don't let the backlog of messages get too large, it's about 1 per second
			bool ignoreBlocking(m_ini->GetSetting(INIFile::common, INIFile::config, "LootBlockedActivators") != 0);
			TriggerAutoHarvest(objType, refrEx.GetItemCount(), isSilent || PendingAutoHarvest() > AutoHarvestSpamLimit, ignoreBlocking);
		}
		else if (m_targetType == INIFile::SecondaryType::containers || m_targetType == INIFile::SecondaryType::deadbodies)
		{
#if _DEBUG
			_DMESSAGE("scanning container/body %s/0x%08x", refrEx.m_ref->GetName(), refrEx.m_ref->formID);
			DumpContainer(refrEx);
#endif
			std::unordered_map<RE::TESForm*, int> lootableItems;

			bool requireQuestItemAsTarget = m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "questObjectScope") != 0;

			bool hasQuestObject = false;
			bool hasEnchantItem = false;
			ContainerLister fnOption(refrEx.m_ref, requireQuestItemAsTarget);

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

			if (hasQuestObject)
			{
				SInt32 questObjectGlow = static_cast<int>(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "questObjectGlow"));
				if (questObjectGlow == 1)
				{
					needsGlow = true;
				}

				skipLooting = skipLooting || (static_cast<int>(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "questObjectLoot")) == 0);
			}

			if (hasEnchantItem)
			{
				SInt32 enchantItemGlow = static_cast<int>(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "enchantItemGlow"));
				if (enchantItemGlow == 1)
				{
					needsGlow = true;
				}
			}
			if (needsGlow)
			{
				TriggerObjectGlow(m_refr);
			}
			if (skipLooting)
				continue;

			// when we get to the point where looting is confirmed, block the reference to
			// avoid re-looting without a player cell or config change
#if _DEBUG
			_DMESSAGE("block looted container %s/0x%08x", refrEx.m_ref->GetName(), refrEx.m_ref->formID);
#endif
			data->BlockReference(refrEx.m_ref);
			// for dead bodies, the REFR is not iterable after a reload, so save those until cell exited
			if (m_targetType == INIFile::SecondaryType::deadbodies)
				DataCase::GetInstance()->RememberDeadBody(refrEx.m_ref->GetFormID());

			// Build list of lootable targets with count and notification flag for each
			std::vector<std::tuple<RE::TESBoundObject*, int, bool>> targets;
			targets.reserve(lootableItems.size());
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
					data->BlockForm(target);
					continue;
				}

				ObjectType objType = ClassifyType(itemEx.m_form);
				std::string typeName = GetObjectTypeName(objType);

				LootingType lootingType = LootingTypeFromIniSetting(m_ini->GetSetting(INIFile::autoharvest, m_targetType, typeName.c_str()));
				if (lootingType == LootingType::LeaveBehind)
				{
#if _DEBUG
					_DMESSAGE("block - typename %s excluded for 0x%08x", typeName.c_str(), target->formID);
#endif
					data->BlockForm(target);
					continue;
				}
				if (LootingDependsOnValueWeight(lootingType, objType) && itemEx.ValueWeightTooLowToLoot(m_ini))
				{
#if _DEBUG
					_DMESSAGE("block - v/w excludes for 0x%08x", target->formID);
#endif
					data->BlockForm(target);
					continue;
				}

				const RE::TESBoundObject* bound(target->As<RE::TESBoundObject>());
				if (!bound)
					continue;

				targets.push_back({const_cast<RE::TESBoundObject*>(bound), targetSize.second, LootingRequiresNotification(lootingType)});
#if _DEBUG
				_DMESSAGE("get %s (%d) from container %s/0x%08x", itemEx.m_form->GetName(), targetSize.second,
					refrEx.m_ref->GetName(), refrEx.m_ref->formID);
#endif
				++count;
			}

			if (count > 0)
			{
				bool animate(false);
				if (m_targetType == INIFile::SecondaryType::containers)
				{
					int playContainerAnimation = static_cast<int>(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "PlayContainerAnimation"));
					animate = playContainerAnimation != 0 && refrEx.GetTimeController();
				}
				TriggerContainerLootMany(targets, animate);
			}
		}
	}
}

RecursiveLock SearchTask::m_searchLock;
bool SearchTask::m_threadStarted = false;
bool SearchTask::m_searchAllowed = false;
bool SearchTask::m_sneaking = false;
RE::TESObjectCELL* SearchTask::m_playerCell = nullptr;
RE::BGSLocation* SearchTask::m_playerLocation = nullptr;
RE::BGSKeyword* SearchTask::m_playerHouseKeyword(nullptr);
bool SearchTask::m_carryAdjustedForCombat = false;
bool SearchTask::m_carryAdjustedForPlayerHome = false;
int SearchTask::m_currentCarryWeightChange = 0;

void SearchTask::SetPlayerHouseKeyword(RE::BGSKeyword* keyword)
{
	m_playerHouseKeyword = keyword;
}

double MinDelay = 0.1;

void SearchTask::Start()
{
	std::thread([]()
	{
#if _DEBUG
		_DMESSAGE("starting thread");
#endif
		m_ini = INIFile::GetInstance();
		while (true)
		{
			double delay(m_ini->GetSetting(INIFile::PrimaryType::autoharvest,
				INIFile::SecondaryType::config, "IntervalSeconds"));
			delay = std::max(MinDelay, delay);
			if (!IsAllowed())
			{
#if _DEBUG
				_DMESSAGE("search disallowed, game loading or menus open");
#endif
			}
			else
			{
				StartSearch(static_cast<SInt32>(INIFile::PrimaryType::autoharvest));
			}
#if _DEBUG
			_DMESSAGE("wait for %d milliseconds", static_cast<long long>(delay * 1000.0));
#endif
			auto nextRunTime = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(static_cast<long long>(delay * 1000.0));
			std::this_thread::sleep_until(nextRunTime);
		}
	}).detach();
}

int InfiniteWeight = 100000;

RE::TESForm* GetCellOwner(RE::TESObjectCELL* cell)
{
	for (RE::BSExtraData& extraData : cell->extraList)
	{
		if (extraData.GetType() == RE::ExtraDataType::kOwnership)
		{
#if _DEBUG
			_MESSAGE("TESObjectCELLEx::GetOwner Hit %08x", reinterpret_cast<RE::ExtraOwnership&>(extraData).owner->formID);
#endif
			return reinterpret_cast<RE::ExtraOwnership&>(extraData).owner;
		}
	}
	return nullptr;
}

bool IsCellPlayerOwned(RE::TESObjectCELL* cell)
{
	const RE::TESForm* owner = GetCellOwner(cell);
	if (cell && owner)
	{
		if (owner->formType == RE::FormType::NPC)
		{
			const RE::TESNPC* npc = owner->As<RE::TESNPC>();
			RE::TESNPC* playerBase = RE::PlayerCharacter::GetSingleton()->GetActorBase();
			return (npc && npc == playerBase);
		}
		else if (owner->formType == RE::FormType::Faction)
		{
			RE::TESFaction* faction = skyrim_cast<RE::TESFaction*, RE::TESForm>(owner);
			if (faction)
			{
				if (RE::PlayerCharacter::GetSingleton()->IsInFaction(faction))
					return true;

				return false;
			}
		}
	}
	return false;
}

void SearchTask::ResetRestrictions(const bool gameReload)
{
	DataCase::GetInstance()->ListsClear(gameReload);
#if _DEBUG
	_MESSAGE("Unlock task-pending REFRs");
#endif
	RecursiveLockGuard guard(m_lock);
	// unblock all blocked auto-harvest objects
	m_autoHarvestLock.clear();
	// unblock possible player house checks after game reload
	if (gameReload)
		m_playerHouses.clear();
	// Stop objects glowing and clean up the list
	for (auto glowingObject : m_glowExpiration)
	{
		StopObjectGlow(glowingObject.first);
	}
	m_glowExpiration.clear();
}

SInt32 SearchTask::StartSearch(SInt32 type1)
{
	std::vector<std::pair<RE::TESObjectREFR*, INIFile::SecondaryType>> candidates;
	UInt32 result(0);
	{
		WindowsUtils::ScopedTimer elapsed("SearchTask::StartSearch");
		if (!IsAllowed())
		{
#if _DEBUG
			_DMESSAGE("search disallowed");
#endif
			return 0;
		}

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

		{
			RecursiveLockGuard guard(m_lock);
			if (!m_pluginSynced)
			{
	#if _DEBUG
				_DMESSAGE("Plugin sync still pending");
	#endif
				return 0;
			}
		}

		// handle player death. Obviously we are not looting on their behalf until a game reload or other resurrection event.
		// Assumes player non-essential: if player is in God mode a little extra carry weight or post-0death looting is not
		// breaking immersion.
		RE::BGSLocation* playerLocation(player->currentLocation);
		const bool RIPPlayer(player->IsDead(true));
		if (RIPPlayer)
		{
			// Fire location change logic
			m_playerLocation = nullptr;
			m_playerCell = nullptr;
		}

		if (playerLocation != m_playerLocation)
		{
#if _DEBUG
			_MESSAGE("Player left old location, now at %s", playerLocation ? playerLocation->GetName() : "unnamed");
#endif
			m_playerLocation = playerLocation;
			// Player changed location - check if it is a player house
			if (m_playerLocation && !IsPlayerHouse(m_playerLocation))
			{
				if (m_playerLocation->HasKeyword(m_playerHouseKeyword))
				{
					// record as a player house and notify as it is a new one in this game load
					AddPlayerHouse(m_playerLocation);
#if _DEBUG
					_MESSAGE("Player House %s detected", m_playerLocation->GetName());
#endif
					static RE::BSFixedString playerHouseMsg(papyrus::GetTranslation(nullptr, RE::BSFixedString("$AHSE_HOUSE_CHECK")));
					if (!playerHouseMsg.empty())
					{
						std::string notificationText(playerHouseMsg);
						Replace(notificationText, "{HOUSENAME}", m_playerLocation->GetName());
						RE::DebugNotification(notificationText.c_str());
					}
				}
			}
		}

		// Respect encumbrance quality of life settings
		bool playerInOwnHouse(IsPlayerHouse(m_playerLocation));
		int carryWeightChange(m_currentCarryWeightChange);
		if (m_ini->GetSetting(INIFile::PrimaryType::common, INIFile::config, "UnencumberedInPlayerHome") != 0.0)
		{
			// when location changes to/from player house, adjust carry weight accordingly
			if (playerInOwnHouse != m_carryAdjustedForPlayerHome)
			{
				carryWeightChange += playerInOwnHouse ? InfiniteWeight : -InfiniteWeight;
				m_carryAdjustedForPlayerHome = playerInOwnHouse;
#if _DEBUG
				_MESSAGE("Carry weight delta after in-player-home adjustment %d", carryWeightChange);
#endif
			}
		}
		bool playerInCombat(player->IsInCombat() && !player->IsDead(true));
		if (m_ini->GetSetting(INIFile::PrimaryType::common, INIFile::config, "UnencumberedInCombat") != 0.0)
		{
			// when state changes in/out of combat, adjust carry weight accordingly
			if (playerInCombat != m_carryAdjustedForCombat)
			{
				carryWeightChange += playerInCombat ? InfiniteWeight : -InfiniteWeight;
				m_carryAdjustedForCombat = playerInCombat;
#if _DEBUG
				_MESSAGE("Carry weight delta after in-combat adjustment %d", carryWeightChange);
#endif
			}
		}
		if (carryWeightChange != m_currentCarryWeightChange)
		{
			int requiredWeightDelta(carryWeightChange - m_currentCarryWeightChange);
			m_currentCarryWeightChange = carryWeightChange;
			// handle carry weight update via a script event
#if _DEBUG
			_MESSAGE("Adjust carry weight by delta %d", requiredWeightDelta);
#endif
			TriggerCarryWeightDelta(requiredWeightDelta);
		}

		if (RIPPlayer)
		{
#if _DEBUG
			_DMESSAGE("Player is dead");
#endif
			return 0;
		}

		if (IsLocationExcluded())
		{
#if _DEBUG
			_DMESSAGE("Player location is excluded");
#endif
			return 0;
		}

		if (playerInOwnHouse)
		{
#if _DEBUG
			_DMESSAGE("Player House, skip");
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

		DataCase* data = DataCase::GetInstance();
		if (!data)
			return 0;

		INIFile::PrimaryType first = static_cast<INIFile::PrimaryType>(type1);
		if (!m_ini->IsType(first))
			return 0;

		const int disableDuringCombat = static_cast<int>(m_ini->GetSetting(first, INIFile::config, "disableDuringCombat"));
		if (disableDuringCombat != 0 && playerInCombat)
		{
#if _DEBUG
			_MESSAGE("disableDuringCombat %d", disableDuringCombat);
#endif
			return 0;
		}

		const int disableWhileWeaponIsDrawn = static_cast<int>(m_ini->GetSetting(first, INIFile::config, "disableWhileWeaponIsDrawn"));
		if (disableWhileWeaponIsDrawn != 0 && player->IsWeaponDrawn())
		{
#if _DEBUG
			_MESSAGE("disableWhileWeaponIsDrawn %d", disableWhileWeaponIsDrawn);
#endif
			return 0;
		}

		bool sneaking = player->IsSneaking();
		bool unblockAll(false);
		// Reset blocked lists if sneak state or player cell has changed
		if (m_sneaking != sneaking)
		{
			m_sneaking = sneaking;
			unblockAll = true;
		}
		// Player cell should never be empty
		RE::TESObjectCELL* playerCell(player->parentCell);
		if (playerCell != m_playerCell)
		{
			unblockAll = true;
			m_playerCell = playerCell;
#if _DEBUG
			if (m_playerCell)
			{
				_MESSAGE("Player cell updated to 0x%08x", m_playerCell->GetFormID());
			}
			else
			{
				_MESSAGE("Player cell cleared");
			}
#endif
		}
		if (unblockAll)
		{
			static const bool gameReload(false);
			ResetRestrictions(gameReload);
		}
		if (!m_playerCell)
		{
#if _DEBUG
			_MESSAGE("Player cell not yet set up");
#endif
			return 0;
		}

		int crimeCheck = static_cast<int>(m_ini->GetSetting(first, INIFile::config, (sneaking) ? "crimeCheckSneaking" : "crimeCheckNotSneaking"));
		int belongingsCheck = static_cast<int>(m_ini->GetSetting(first, INIFile::config, "playerBelongingsLoot"));

		std::vector<RE::TESObjectREFR*> refs;
		PlayerCellHelper(m_playerCell, refs, m_ini->GetRadius(first)).GetReferences();
		result = static_cast<UInt32>(refs.size());
		if (result == 0)
			return 0;

		candidates.reserve(refs.size());
		for (RE::TESObjectREFR* refr : refs)
		{
			if (!refr)
				continue;
			INIFile::SecondaryType lootTargetType = INIFile::SecondaryType::itemObjects;
			RE::TESObjectREFR* ashPileRefr(nullptr);
			if (refr->data.objectReference->As<RE::TESContainer>())
			{
				if (m_ini->GetSetting(INIFile::PrimaryType::common, INIFile::config, "EnableLootContainer") == 0.0)
					continue;
				lootTargetType = INIFile::SecondaryType::containers;
			}
			else if (refr->data.objectReference->As<RE::Actor>())
			{
				if (m_ini->GetSetting(INIFile::PrimaryType::common, INIFile::config, "enableLootDeadbody") == 0.0 || !refr->IsDead(true))
					continue;

				ActorHelper actorEx(refr->data.objectReference->As<RE::Actor>());
				bool allied(actorEx.IsPlayerAlly());
				if (allied || actorEx.IsEssential() || !actorEx.IsSummonable())
				{
					data->BlockReference(refr);
					continue;
				}

				lootTargetType = INIFile::SecondaryType::deadbodies;
			}
			else if (refr->data.objectReference->As<RE::TESObjectACTI>() && (ashPileRefr = GetAshPile(refr)))
			{
				if (m_ini->GetSetting(INIFile::PrimaryType::common, INIFile::config, "enableLootDeadbody") == 0.0)
					continue;
				lootTargetType = INIFile::SecondaryType::deadbodies;
			}
			else if (m_ini->GetSetting(INIFile::PrimaryType::common, INIFile::config, "enableAutoHarvest") == 0.0)
			{
				continue;
			}

			bool ownership = false;
			if (crimeCheck == 1)
			{
				ownership = refr->IsOffLimits();

#if _DEBUG
				const char* str = ownership ? "true" : "false";
				_MESSAGE("***1 crimeCheck: %d ownership: %s", crimeCheck, str);
#endif

			}
			else if (crimeCheck == 2)
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
					ownership = IsCellPlayerOwned(m_playerCell);
#if _DEBUG
					const char* str = ownership ? "true" : "false";
					_MESSAGE("***2-2 crimeCheck: %d ownership: %s", crimeCheck, str);
#endif
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
				data->BlockReference(refr);
				continue;
			}

			if (ashPileRefr)
			{
				refr = ashPileRefr;
			}
			candidates.push_back(std::make_pair(refr, lootTargetType));
		}
	}
	SearchTask(candidates).Run();
	return result;
}

void SearchTask::PrepareForReload()
{
	// stop scanning
	Disallow();

#if _DEBUG
	_MESSAGE("Reset carry weight delta %d, in-player-home=%s, in-combat=%s", m_currentCarryWeightChange,
		m_carryAdjustedForPlayerHome ? "true" : "false", m_carryAdjustedForCombat ? "true" : "false");
#endif
	// reset carry weight adjustments - scripts will handle the Player Actor Value, our scan will reinstate as needed
	m_currentCarryWeightChange = 0;
	m_carryAdjustedForPlayerHome = false;
	m_carryAdjustedForCombat = false;
	// reset player location - reload may bring us back in a different place and even if not, we should start from scratch
	m_playerCell = nullptr;
	m_playerLocation = nullptr;

	// Do not scan again until we are in sync with the scripts
	m_pluginSynced = false;
}

void SearchTask::Allow()
{
	RecursiveLockGuard guard(m_searchLock);
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
	RecursiveLockGuard guard(m_searchLock);
	m_searchAllowed = false;
}
bool SearchTask::IsAllowed()
{
	RecursiveLockGuard guard(m_searchLock);
	return m_searchAllowed;
}

void SearchTask::TriggerGetCritterIngredient()
{
	RE::BSScript::Internal::VirtualMachine::GetSingleton()->SendEvent(
		m_aliasHandle, critterIngredientEvent, RE::MakeFunctionArguments(std::move(m_refr)));
}

void SearchTask::TriggerCarryWeightDelta(const int delta)
{
	RE::BSScript::Internal::VirtualMachine::GetSingleton()->SendEvent(
		m_aliasHandle, carryWeightDeltaEvent, RE::MakeFunctionArguments(std::move(delta)));
}

std::unordered_set<const RE::TESObjectREFR*> SearchTask::m_autoHarvestLock;

void SearchTask::TriggerAutoHarvest(const ObjectType objType, int itemCount, const bool isSilent, const bool ignoreBlocking)
{
    // Event handler in Papyrus script unlocks the task - do not issue multiple concurrent events on the same REFR
	if (!LockAutoHarvest(m_refr))
		return;
	SInt32 intType(static_cast<SInt32>(objType));
#if 0
	// hack to avoid bool/uint32 ambiguity
	SInt32 intSilent(static_cast<SInt32>(isSilent));
	// hack to avoid bool/uint32 ambiguity
	SInt32 intIgnoreBlocking(static_cast<SInt32>(ignoreBlocking));
#endif
	RE::BSScript::Internal::VirtualMachine::GetSingleton()->SendEvent(
		m_aliasHandle, autoHarvestEvent, RE::MakeFunctionArguments(std::move(m_refr), std::move(intType),
			std::move(itemCount), std::move(isSilent), std::move(ignoreBlocking)));
}

bool SearchTask::LockAutoHarvest(const RE::TESObjectREFR* refr)
{
	RecursiveLockGuard guard(m_lock);
	if (!refr)
		return false;
	return (m_autoHarvestLock.insert(refr)).second;
}

bool SearchTask::UnlockAutoHarvest(const RE::TESObjectREFR* refr)
{
	RecursiveLockGuard guard(m_lock);
	if (!refr)
		return false;
	return m_autoHarvestLock.erase(refr) > 0;
}

bool SearchTask::IsLockedForAutoHarvest(const RE::TESObjectREFR* refr)
{
	RecursiveLockGuard guard(m_lock);
	return (refr && m_autoHarvestLock.count(refr));
}

size_t SearchTask::PendingAutoHarvest()
{
	RecursiveLockGuard guard(m_lock);
	return m_autoHarvestLock.size();
}

std::unordered_set<const RE::BGSLocation*> SearchTask::m_playerHouses;
bool SearchTask::AddPlayerHouse(const RE::BGSLocation* location)
{
	if (!location)
		return false;
	RecursiveLockGuard guard(m_lock);
	return (m_playerHouses.insert(location)).second;
}

bool SearchTask::RemovePlayerHouse(const RE::BGSLocation* location)
{
	if (!location)
		return false;
	RecursiveLockGuard guard(m_lock);
	return m_playerHouses.erase(location) > 0;
}

// Check indeterminate status of the location, because a requested UI check is pending
bool SearchTask::IsPlayerHouse(const RE::BGSLocation* location)
{
	RecursiveLockGuard guard(m_lock);
	return location && m_playerHouses.count(location);
}

std::unordered_set<const RE::TESForm*> SearchTask::m_excludeLocations;
bool SearchTask::m_pluginSynced(false);

// this is the last function called by the scripts when re-syncing state
void SearchTask::MergeExcludeList()
{
	RecursiveLockGuard guard(m_lock);
	// Add loaded locations to the list of exclusions
	BasketFile::GetSingleton()->SyncList(BasketFile::EXCLUDELIST);
	for (const auto exclusion : BasketFile::GetSingleton()->GetList(BasketFile::EXCLUDELIST))
	{
		SearchTask::AddLocationToExcludeList(exclusion);
	}
	// reset blocked lists to allow recheck vs current state
	static const bool gameReload(true);
	ResetRestrictions(gameReload);

	// need to wait for the scripts to sync up before performing player house checks
	m_pluginSynced = true;
}

void SearchTask::ResetExcludedLocations()
{
#if _DEBUG
	_DMESSAGE("Reset list of locations excluded from looting");
#endif
	RecursiveLockGuard guard(m_lock);
	m_excludeLocations.clear();
}

void SearchTask::AddLocationToExcludeList(const RE::TESForm* location)
{
#if _DEBUG
	_DMESSAGE("Location %s excluded from looting", location->GetName());
#endif
	RecursiveLockGuard guard(m_lock);
	m_excludeLocations.insert(location);
}

void SearchTask::DropLocationFromExcludeList(const RE::TESForm* location)
{
#if _DEBUG
	_DMESSAGE("Location %s no longer excluded from looting", location->GetName());
#endif
	RecursiveLockGuard guard(m_lock);
	m_excludeLocations.erase(location);
}

bool SearchTask::IsLocationExcluded()
{
	if (!m_playerLocation)
		return false;
	RecursiveLockGuard guard(m_lock);
	return m_excludeLocations.count(m_playerLocation) > 0;
}

void SearchTask::TriggerContainerLootMany(const std::vector<std::tuple<RE::TESBoundObject*, int, bool>>& targets, const bool animate)
{
	if (!m_refr)
		return;
	for (const auto target : targets)
	{
		RE::ExtraDataList* xList(nullptr);
		auto first(m_refr->extraList.cbegin());
		if (first != m_refr->extraList.cend())
		{
			xList = &m_refr->extraList;
		}
		RE::TESBoundObject* boundObject(std::get<0>(target));
		int count(std::get<1>(target));
		bool notify(std::get<2>(target));
		RE::ObjectRefHandle handle(m_refr->RemoveItem(boundObject, count, RE::ITEM_REMOVE_REASON::kRemove, xList, RE::PlayerCharacter::GetSingleton()));
		RE::PlayerCharacter::GetSingleton()->PlayPickUpSound(boundObject, true, false);
		if (animate)
		{
			m_refr->PlayAnimation("Close", "Open");
		}

		if (notify)
		{
			std::string notificationText;
			if (count > 1)
			{
				static RE::BSFixedString multiActivate(papyrus::GetTranslation(nullptr, RE::BSFixedString("$AHSE_ACTIVATE(COUNT)_MSG")));
				if (!multiActivate.empty())
				{
					notificationText = multiActivate;
					Replace(notificationText, "{ITEMNAME}", boundObject->GetName());
					std::ostringstream intStr;
					intStr << count;
					Replace(notificationText, "{COUNT}", intStr.str());
				}
			}
			else
			{
				static RE::BSFixedString singleActivate(papyrus::GetTranslation(nullptr, RE::BSFixedString("$AHSE_ACTIVATE_MSG")));
				if (!singleActivate.empty())
				{
					notificationText = singleActivate;
					Replace(notificationText, "{ITEMNAME}", boundObject->GetName());
				}
			}
			if (!notificationText.empty())
			{
				RE::DebugNotification(notificationText.c_str());
			}
		}
	}
}

const int ObjectGlowDuration = 15;

void SearchTask::TriggerObjectGlow(RE::TESObjectREFR* refr)
{

	// only send the glow event once per N seconds. This will retrigger on every pass but once we are out of
	// range no more glowing will be triggered. The item remains in the list until we change cell but there should
	// never be so many in a cell that this is a problem.
	RecursiveLockGuard guard(m_lock);
	const auto existingGlow(m_glowExpiration.find(refr));
	auto currentTime(std::chrono::high_resolution_clock::now());
	if (existingGlow != m_glowExpiration.cend() && existingGlow->second > currentTime)
		return;
	auto expiry = currentTime + std::chrono::milliseconds(static_cast<long long>(ObjectGlowDuration * 1000.0));
	m_glowExpiration[refr] = expiry;
#if _DEBUG
	_DMESSAGE("Trigger glow for %s/0x%08x", refr->GetName(), refr->formID);
#endif
	RE::BSScript::Internal::VirtualMachine::GetSingleton()->SendEvent(
		m_aliasHandle, objectGlowEvent, RE::MakeFunctionArguments(std::move(refr), std::move(ObjectGlowDuration)));
}

void SearchTask::StopObjectGlow(const RE::TESObjectREFR* refr)
{
	RE::BSScript::Internal::VirtualMachine::GetSingleton()->SendEvent(
		m_aliasHandle, objectGlowStopEvent, RE::MakeFunctionArguments(std::move(refr)));
}

bool SearchTask::firstTime = true;

void SearchTask::Init()
{
	WindowsUtils::ScopedTimer elapsed("SearchTask::Init");
    if (firstTime)
	{
		DataCase::GetInstance()->CategorizeLootables();
		firstTime = false;
	}
	static const bool gameReload(true);
	ResetRestrictions(gameReload);
}

