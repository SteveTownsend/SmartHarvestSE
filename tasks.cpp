#include "PrecompiledHeaders.h"

#include "tasks.h"
#include "TESFormHelper.h"
#include "objects.h"
#include "dataCase.h"
#include "iniSettings.h"
#include "basketfile.h"
#include "debugs.h"
#include "papyrus.h"
#include "PlayerCellHelper.h"

#include "RE/SkyrimScript/RemoveItemFunctor.h"

#include <chrono>
#include <thread>

INIFile* SearchTask::m_ini = nullptr;
RE::BGSRefAlias* SearchTask::m_eventTarget = nullptr;

RecursiveLock SearchTask::m_lock;
std::unordered_map<const RE::TESObjectREFR*, std::chrono::time_point<std::chrono::high_resolution_clock>> SearchTask::m_glowExpiration;

// special object glow - not too long, in case we loot or move away
const int ObjectGlowDurationSpecial = 5;
// looted container notification glow - relatively short
const int ObjectGlowDurationNotify = 2;

SKSE::RegistrationSet<RE::TESObjectREFR*> onGetCritterIngredient("OnGetCritterIngredient");
SKSE::RegistrationSet<int> onCarryWeightDelta("OnCarryWeightDelta");
SKSE::RegistrationSet<> onResetCarryWeight("OnResetCarryWeight");
SKSE::RegistrationSet<RE::TESObjectREFR*, int, int, bool, bool, bool> onAutoHarvest("OnAutoHarvest");
SKSE::RegistrationSet<RE::TESObjectREFR*, int> onObjectGlow("OnObjectGlow");

RE::BGSRefAlias* GetScriptTarget(const char* espName, UInt32 questID)
{
	static RE::TESQuest* quest = nullptr;
	static RE::BGSRefAlias* alias = nullptr;
#if _DEBUG
	static bool listed(false);
#endif
	if (!quest)
	{
		UInt32 formID = 0;
		std::optional<UInt8> idx = RE::TESDataHandler::GetSingleton()->GetLoadedModIndex(espName);
		if (idx.has_value())
		{
			formID = (idx.value() << 24) | questID;
#if _DEBUG
			_DMESSAGE("Got formID for questID %08.2x", questID);
#endif
		}
#if _DEBUG
		else if (!listed)
		{
			for (const auto& nextFile : RE::TESDataHandler::GetSingleton()->compiledFileCollection.files)
			{
				_DMESSAGE("Mod loaded %s", &nextFile->fileName);
			}
			listed = true;
		}
#endif
		if (formID != 0)
		{
			RE::TESForm* questForm = RE::TESForm::LookupByID(formID);
#if _DEBUG
			_DMESSAGE("Got Base Form %s", questForm ? questForm->GetFormEditorID() : "nullptr");
#endif
			quest = questForm ? questForm->As<RE::TESQuest>() : nullptr;
#if _DEBUG
			_DMESSAGE("Got Quest Form %s", questForm->As<RE::TESQuest>() ? questForm->GetFormEditorID() : "nullptr");
#endif
		}
	}
	if (quest && quest->IsRunning())
	{
#if _DEBUG
		_DMESSAGE("Quest %s is running", quest->GetFormEditorID());
#endif
		RE::BGSBaseAlias* baseAlias(quest->aliases[0]);
		if (!baseAlias)
		{
#if _DEBUG
			_DMESSAGE("Quest has no alias at index 0");
#endif
			return nullptr;
		}

		alias = static_cast<RE::BGSRefAlias*>(baseAlias);
		if (!alias)
		{
#if _DEBUG
			_DMESSAGE("Quest is not type BGSRefAlias");
#endif
			return nullptr;
		}
#if _DEBUG
		_MESSAGE("Got BGSRefAlias for Mod's Quest");
#endif
	}
	return alias;
}

bool SearchTask::GoodToGo()
{
	if (!m_eventTarget)
	{
		m_eventTarget = GetScriptTarget(MODNAME, QUEST_ID);
		// register the events
		if (m_eventTarget)
		{
			onGetCritterIngredient.Register(m_eventTarget);
			onCarryWeightDelta.Register(m_eventTarget);
			onResetCarryWeight.Register(m_eventTarget);
			onObjectGlow.Register(m_eventTarget);
			onAutoHarvest.Register(m_eventTarget);
		}
	}
	return m_eventTarget != nullptr;
}

SearchTask::SearchTask(RE::TESObjectREFR* candidate, INIFile::SecondaryType targetType)
	: m_candidate(candidate), m_targetType(targetType)
{
}

RE::TESForm* GetCellOwner(RE::TESObjectCELL* cell)
{
	for (RE::BSExtraData& extraData : cell->extraList)
	{
		if (extraData.GetType() == RE::ExtraDataType::kOwnership)
		{
#if _DEBUG
			_MESSAGE("GetCellOwner Hit %08x", reinterpret_cast<RE::ExtraOwnership&>(extraData).owner->formID);
#endif
			return reinterpret_cast<RE::ExtraOwnership&>(extraData).owner;
		}
	}
	return nullptr;
}

bool IsCellPlayerOwned(RE::TESObjectCELL* cell)
{
	if (!cell)
		return false;
	RE::TESForm* owner = GetCellOwner(cell);
	if (!owner)
		return false;
	if (owner->formType == RE::FormType::NPC)
	{
		const RE::TESNPC* npc = owner->As<RE::TESNPC>();
		RE::TESNPC* playerBase = RE::PlayerCharacter::GetSingleton()->GetActorBase();
		return (npc && npc == playerBase);
	}
	else if (owner->formType == RE::FormType::Faction)
	{
		RE::TESFaction* faction = owner->As<RE::TESFaction>();
		if (faction)
		{
			if (RE::PlayerCharacter::GetSingleton()->IsInFaction(faction))
				return true;

			return false;
		}
	}
	return false;
}

const int AutoHarvestSpamLimit = 10;

bool SearchTask::IsLootingForbidden() const
{
	bool isForbidden(false);
	// Perform crime checks - this is done after checks for quest object glowing, as many quest-related objects are owned.
	// Ownership expires with the target, e.g. Francis the Horse from Interesting NPCs was killed by a wolf in Solitude
	// and becomes lootable thereafter.
	// For non-dead targets, check law-abiding settings vs criminality of target and player-ownership settings vs ownership
	if (m_targetType != INIFile::SecondaryType::deadbodies)
	{
		// check up to three ownership conditions depending on config
		bool playerOwned(TESObjectREFRHelper(m_candidate).IsPlayerOwned());
		bool lootingIsCrime(m_candidate->IsOffLimits());
		if (m_playerCellSelfOwned || playerOwned)
		{
			// can configure to not loot my own belongings even though it's always legal
			if (m_belongingsCheck == 0)
			{
#if _DEBUG
				_DMESSAGE("Player home or player-owned, looting belongings disallowed");
#endif
				isForbidden = true;
			}
		}
		// if restricted to law-abiding citizenship, check if OK to loot
		else if (m_crimeCheck > 0)
		{
			if (m_candidate->IsOffLimits())
			{
				// never commit a crime unless crimeCheck is 0
#if _DEBUG
				_DMESSAGE("Crime to loot REFR, cannot loot");
#endif
				isForbidden = true;
			}
			else if (m_crimeCheck == 2 && m_candidate->GetOwner() != nullptr)
			{
				// owner cannot be player by construction, disallow
#if _DEBUG
				_DMESSAGE("REFR is owned, cannot loot");
#endif
				isForbidden = true;
			}
		}

		if (isForbidden)
		{
#if _DEBUG
			_MESSAGE("Skip owned/illegal-to-loot REFR: %s/0x%08x", m_candidate->data.objectReference->GetName(), m_candidate->data.objectReference->formID);
#endif
		}
	}
	return isForbidden;
}

bool SearchTask::IsBookGlowable() const
{
	RE::BGSKeywordForm* keywordForm(m_candidate->data.objectReference->As<RE::BGSKeywordForm>());
	if (!keywordForm)
		return false;
	for (UInt32 index = 0; index < keywordForm->GetNumKeywords(); ++index)
	{
		std::optional<RE::BGSKeyword*> keyword(keywordForm->GetKeywordAt(index));
		if (!keyword || !keyword.has_value())
			continue;
		if (DataCase::GetInstance()->IsBookGlowableKeyword(keyword.value()))
			return true;
	}
	return false;
}

void SearchTask::Run()
{
	WindowsUtils::ScopedTimer elapsed("SearchTask::Run");
	DataCase* data = DataCase::GetInstance();
	RE::BSScript::Internal::VirtualMachine* vm(RE::BSScript::Internal::VirtualMachine::GetSingleton());

	TESObjectREFRHelper refrEx(m_candidate);
	if (m_targetType == INIFile::SecondaryType::itemObjects)
	{
		ObjectType objType = refrEx.GetObjectType();
		std::string typeName = refrEx.GetTypeName();
		// Various form types contain an ingredient that is the final lootable item - resolve here
		RE::TESForm* lootable(DataCase::GetInstance()->GetLootableForProducer(m_candidate->data.objectReference));
		if (lootable)
		{
#if _DEBUG
			_DMESSAGE("producer %s/0x%08x has lootable %s/0x%08x", m_candidate->data.objectReference->GetName(), m_candidate->data.objectReference->formID,
				lootable->GetName(), lootable->formID);
#endif
			refrEx.SetLootable(lootable);
		}
		else if (objType == ObjectType::critter)
		{
			// trigger critter -> ingredient resolution and skip until it's resolved - pending resolve recorded using nullptr,
			// only trigger if not already pending
#if _DEBUG
			_DMESSAGE("resolve critter %s/0x%08x to ingredient", m_candidate->data.objectReference->GetName(), m_candidate->data.objectReference->formID);
#endif
			if (DataCase::GetInstance()->SetLootableForProducer(m_candidate->data.objectReference, nullptr))
			{
				TriggerGetCritterIngredient();
			}
			return;
		}

#if _DEBUG
		_MESSAGE("typeName  %s", typeName.c_str());
		DumpReference(refrEx, typeName.c_str());
#endif

		if (objType == ObjectType::unknown)
		{
#if _DEBUG
			_DMESSAGE("block objType == ObjectType::unknown for 0x%08x", m_candidate->data.objectReference->formID);
#endif
			data->BlockForm(m_candidate->data.objectReference);
			return;
		}

		bool manualLootNotify(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "ManualLootTargetNotify") != 0);
		if (objType == ObjectType::manualLoot && manualLootNotify)
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
			_DMESSAGE("notify, then block objType == ObjectType::manualLoot for 0x%08x", m_candidate->data.objectReference->formID);
#endif
			data->BlockForm(m_candidate->data.objectReference);
			return;
		}

		if (BasketFile::GetSingleton()->IsinList(BasketFile::EXCLUDELIST, m_candidate->data.objectReference))
		{
#if _DEBUG
			_DMESSAGE("block m_refr->data.objectReference >= 1 for 0x%08x", m_candidate->data.objectReference->formID);
#endif
			data->BlockForm(m_candidate->data.objectReference);
			return;
		}

		bool needsGlow(false);
		bool skipLooting(false);

		bool needsFullQuestFlags(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "questObjectScope") != 0);
		SInt32 questObjectGlow = static_cast<int>(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "questObjectGlow"));
		if (refrEx.IsQuestItem(needsFullQuestFlags))
		{
#if _DEBUG
			_DMESSAGE("Quest Item 0x%08x", m_candidate->data.objectReference->formID);
#endif
			if (questObjectGlow != 0)
			{
				needsGlow = true;
			}

			SInt32 questObjectLoot = static_cast<int>(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "questObjectLoot"));
			if (questObjectLoot == 0)
				skipLooting = true;
		}
		// glow unread notes as they are often quest-related
		else if (questObjectGlow != 0 && objType == ObjectType::books && IsBookGlowable())
		{
#if _DEBUG
			_DMESSAGE("Glowable book 0x%08x", m_candidate->data.objectReference->formID);
#endif
			needsGlow = true;
		}

		if (needsGlow)
		{
			TriggerObjectGlow(m_candidate, ObjectGlowDurationSpecial);
		}

		if (objType == ObjectType::ammo)
		{
			skipLooting = skipLooting || data->SkipAmmoLooting(m_candidate);
		}

		skipLooting = skipLooting || IsLootingForbidden();

		if (!skipLooting)
		{
			LootingType lootingType = LootingTypeFromIniSetting(m_ini->GetSetting(INIFile::autoharvest, m_targetType, typeName.c_str()));
			if (lootingType == LootingType::LeaveBehind)
			{
#if _DEBUG
				_MESSAGE("Block REFR : LeaveBehind for 0x%08x", m_candidate->data.objectReference->formID);
#endif
				data->BlockReference(m_candidate);
				skipLooting = true;
			}
			else if (LootingDependsOnValueWeight(lootingType, objType) && TESFormHelper(m_candidate->data.objectReference).ValueWeightTooLowToLoot(m_ini))
			{
#if _DEBUG
				_DMESSAGE("block - v/w excludes harvest for 0x%08x", m_candidate->data.objectReference->formID);
#endif
				data->BlockForm(m_candidate->data.objectReference);
				skipLooting = true;
			}
		}
		if (skipLooting)
			return;

		// don't try to re-harvest excluded, depleted or malformed ore vein again until we revisit the cell
		if (objType == ObjectType::oreVein)
		{
#if _DEBUG
			_DMESSAGE("do not process oreVein more than once per cell visit: 0x%08x", m_candidate->formID);
#endif
			data->BlockReference(m_candidate);
		}

		bool isSilent = !LootingRequiresNotification(
			LootingTypeFromIniSetting(m_ini->GetSetting(INIFile::autoharvest, m_targetType, typeName.c_str())));
#if _DEBUG
		_MESSAGE("Enqueue AutoHarvest event");
#endif
		// don't let the backlog of messages get too large, it's about 1 per second
		bool ignoreBlocking(m_ini->GetSetting(INIFile::common, INIFile::config, "LootBlockedActivators") != 0);
		TriggerAutoHarvest(objType, refrEx.GetItemCount(), isSilent || PendingAutoHarvest() > AutoHarvestSpamLimit, ignoreBlocking, manualLootNotify);
	}
	else if (m_targetType == INIFile::SecondaryType::containers || m_targetType == INIFile::SecondaryType::deadbodies)
	{
#if _DEBUG
		_DMESSAGE("scanning container/body %s/0x%08x", refrEx.m_ref->GetName(), refrEx.m_ref->formID);
		DumpContainer(refrEx);
#endif
		bool requireQuestItemAsTarget = m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "questObjectScope") != 0;
		bool hasQuestObject(false);
		bool hasEnchantItem(false);
		bool skipLooting(false);
		LootableItems lootableItems;
		if (!ContainerLister(refrEx.m_ref, requireQuestItemAsTarget).GetOrCheckContainerForms(
			lootableItems, hasQuestObject, hasEnchantItem))
		{
			skipLooting = true;
		}

		bool needsGlow(false);
		if (m_targetType == INIFile::SecondaryType::containers)
		{
			if (data->IsReferenceLockedContainer(m_candidate))
			{
				SInt32 lockedChestGlow = static_cast<int>(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "lockedChestGlow"));
				if (lockedChestGlow == 1)
				{
					needsGlow = true;
				}

				if (static_cast<int>(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "lockedChestLoot")) == 0)
				{
					// Remember locked container, to avoid indeterminate disallowed auto-loot if player unlocks it
					// via the Unlock UI or some other means.
					// We don't want to block the reference, or we will only glow this once. This is a way of persisting the
					// initial locked state, making Locked Container flow of control more like Boss Container.
					if (data->RememberLockedContainer(refrEx.m_ref->GetFormID()))
					{
#if _DEBUG
						_DMESSAGE("recorded locked container %s/0x%08x", refrEx.m_ref->GetName(), refrEx.m_ref->formID);
#endif
					}
					skipLooting = true;
				}
			}

			if (IsBossContainer(m_candidate))
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
			TriggerObjectGlow(m_candidate, ObjectGlowDurationSpecial);
		}
		if (skipLooting || IsLootingForbidden())
			return;

		// when we get to the point where looting is confirmed, block the reference to
		// avoid re-looting without a player cell or config change
#if _DEBUG
		_DMESSAGE("block looted container %s/0x%08x", refrEx.m_ref->GetName(), refrEx.m_ref->formID);
#endif
		data->BlockReference(refrEx.m_ref);
		// Build list of lootable targets with count and notification flag for each
		std::vector<std::pair<InventoryItem, bool>> targets;
		targets.reserve(lootableItems.size());
		int count(0);
		for (auto& targetItemInfo : lootableItems)
		{
			RE::TESBoundObject* target(targetItemInfo.BoundObject());
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

			targets.push_back({targetItemInfo, LootingRequiresNotification(lootingType)});
#if _DEBUG
			_DMESSAGE("get %s (%d) from container %s/0x%08x", itemEx.m_form->GetName(), targetItemInfo.Count(),
				refrEx.m_ref->GetName(), refrEx.m_ref->formID);
#endif
			++count;
		}

		if (count > 0)
		{
			// check highlighting for dead NPC or container
			int playContainerAnimation(static_cast<int>(m_ini->GetSetting(INIFile::autoharvest, INIFile::config, "PlayContainerAnimation")));
			if (playContainerAnimation > 0)
			{
				if (m_targetType == INIFile::SecondaryType::containers)
				{
					if (!refrEx.GetTimeController())
					{
						// no container animation feasible, highlight it instead
						playContainerAnimation = 2;
					}
				}
				else
				{
					// Dead NPCs cannot be animated, but highlighting requested
					playContainerAnimation = 2;
				}
			}

			TriggerContainerLootMany(targets, playContainerAnimation);
		}
	}
}

RecursiveLock SearchTask::m_searchLock;
bool SearchTask::m_threadStarted = false;
bool SearchTask::m_searchAllowed = false;
bool SearchTask::m_sneaking = false;
RE::TESObjectCELL* SearchTask::m_playerCell = nullptr;
bool SearchTask::m_playerCellSelfOwned = false;
RE::BGSLocation* SearchTask::m_playerLocation = nullptr;
RE::BGSKeyword* SearchTask::m_playerHouseKeyword(nullptr);
bool SearchTask::m_carryAdjustedForCombat = false;
bool SearchTask::m_carryAdjustedForPlayerHome = false;
bool SearchTask::m_carryAdjustedForDrawnWeapon = false;
int SearchTask::m_currentCarryWeightChange = 0;
int SearchTask::m_crimeCheck = 0;
int SearchTask::m_belongingsCheck = 0;

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

void SearchTask::ResetRestrictions(const bool gameReload)
{
	DataCase::GetInstance()->ListsClear();
#if _DEBUG
	_MESSAGE("Unlock task-pending REFRs");
#endif
	RecursiveLockGuard guard(m_lock);
	// unblock all blocked auto-harvest objects
	m_autoHarvestLock.clear();
	// unblock possible player house checks after game reload
	if (gameReload)
	{
		m_playerHouses.clear();
	}
	// clean up the list of glowing objects, don't futz with EffectShader since cannot run scripts at this time
	m_glowExpiration.clear();
}

SInt32 SearchTask::StartSearch(SInt32 type1)
{
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
		// Assumes player non-essential: if player is in God mode a little extra carry weight or post-death looting is not
		// breaking immersion.
		RE::BGSLocation* playerLocation(player->currentLocation);
		const bool RIPPlayer(player->IsDead(true));
		if (RIPPlayer)
		{
			// Fire location change logic
			m_playerLocation = nullptr;
			m_playerCell = nullptr;
			m_playerCellSelfOwned = false;
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
		bool isWeaponDrawn(player->IsWeaponDrawn());
		if (m_ini->GetSetting(INIFile::PrimaryType::common, INIFile::config, "UnencumberedIfWeaponDrawn") != 0.0)
		{
			// when state changes between drawn/sheathed, adjust carry weight accordingly
			if (isWeaponDrawn != m_carryAdjustedForDrawnWeapon)
			{
				carryWeightChange += isWeaponDrawn ? InfiniteWeight : -InfiniteWeight;
				m_carryAdjustedForDrawnWeapon = isWeaponDrawn;
#if _DEBUG
				_MESSAGE("Carry weight delta after drawn weapon adjustment %d", carryWeightChange);
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

		// By inspection, UI menu stack has steady state size of 1. Opening application and/or inventory adds 1 each,
		// opening console adds 2. So this appears to be a catch-all for those conditions.
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
			// reset carry weight - will reinstate correct value if/when scan resumes
			ResetCarryWeight();
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
			m_playerCellSelfOwned = IsCellPlayerOwned(m_playerCell);
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

		// Retrieve these settings only once
		m_crimeCheck = static_cast<int>(m_ini->GetSetting(first, INIFile::config, (sneaking) ? "crimeCheckSneaking" : "crimeCheckNotSneaking"));
		m_belongingsCheck = static_cast<int>(m_ini->GetSetting(first, INIFile::config, "playerBelongingsLoot"));

		std::vector<RE::TESObjectREFR*> refs;
		PlayerCellHelper::GetInstance().GetReferences(m_playerCell, &refs, m_ini->GetRadius(first));
		result = static_cast<UInt32>(refs.size());
		if (result == 0)
			return 0;

		for (RE::TESObjectREFR* refr : refs)
		{
			if (!refr)
				continue;
			INIFile::SecondaryType lootTargetType = INIFile::SecondaryType::itemObjects;
			RE::TESObjectREFR* ashPileRefr(nullptr);
			if (refr->data.objectReference->As<RE::Actor>() || refr->data.objectReference->As<RE::TESNPC>())
			{
				if (m_ini->GetSetting(INIFile::PrimaryType::common, INIFile::config, "enableLootDeadbody") == 0.0 || !refr->IsDead(true))
					continue;

				RE::Actor* actor(refr->data.objectReference->As<RE::Actor>());
				if (actor)
				{
					ActorHelper actorEx(actor);
					bool allied(actorEx.IsPlayerAlly());
					if (allied || actorEx.IsEssential() || !actorEx.IsSummonable())
					{
						data->BlockReference(refr);
						continue;
					}
				}

				lootTargetType = INIFile::SecondaryType::deadbodies;
			}
			else if (refr->data.objectReference->As<RE::TESContainer>())
			{
				if (m_ini->GetSetting(INIFile::PrimaryType::common, INIFile::config, "EnableLootContainer") == 0.0)
					continue;
				lootTargetType = INIFile::SecondaryType::containers;
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

			if (ashPileRefr)
			{
				refr = ashPileRefr;
			}
			SearchTask(refr, lootTargetType).Run();
		}
	}
	return result;
}

// reset carry weight adjustments - scripts will handle the Player Actor Value, scan will reinstate as needed when we resume
void SearchTask::ResetCarryWeight()
{
	if (m_currentCarryWeightChange != 0)
	{
#if _DEBUG
		_MESSAGE("Reset carry weight delta %d, in-player-home=%s, in-combat=%s, weapon-drawn=%s", m_currentCarryWeightChange,
			m_carryAdjustedForPlayerHome ? "true" : "false", m_carryAdjustedForCombat ? "true" : "false", m_carryAdjustedForDrawnWeapon ? "true" : "false");
#endif
		m_currentCarryWeightChange = 0;
		m_carryAdjustedForCombat = false;
		m_carryAdjustedForPlayerHome = false;
		m_carryAdjustedForDrawnWeapon = false;
		TriggerResetCarryWeight();
	}
}

void SearchTask::PrepareForReload()
{
	// stop scanning
	Disallow();
	ResetCarryWeight();
	// reset player location - reload may bring us back in a different place and even if not, we should start from scratch
	m_playerCell = nullptr;
	m_playerCellSelfOwned = false;
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
	onGetCritterIngredient.SendEvent(m_candidate);
}

void SearchTask::TriggerCarryWeightDelta(const int delta)
{
	onCarryWeightDelta.SendEvent(delta);
}

void SearchTask::TriggerResetCarryWeight()
{
	onResetCarryWeight.SendEvent();
}

std::unordered_set<const RE::TESObjectREFR*> SearchTask::m_autoHarvestLock;

void SearchTask::TriggerAutoHarvest(const ObjectType objType, int itemCount, const bool isSilent, const bool ignoreBlocking, const bool manualLootNotify)
{
	// Event handler in Papyrus script unlocks the task - do not issue multiple concurrent events on the same REFR
	if (!LockAutoHarvest(m_candidate))
		return;
	onAutoHarvest.SendEvent(m_candidate, static_cast<int>(objType), itemCount, isSilent, ignoreBlocking, manualLootNotify);
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

void SearchTask::TriggerContainerLootMany(std::vector<std::pair<InventoryItem, bool>>& targets, const int animationType)
{
	if (!m_candidate)
		return;

	// visual notification, if requested
	if (animationType == 1)
	{
		m_candidate->PlayAnimation("Close", "Open");
	}
	else if (animationType == 2)
	{
		// glow looted object for a few seconds after looting
		TriggerObjectGlow(m_candidate, ObjectGlowDurationNotify);
	}

	for (auto& target : targets)
	{
		InventoryItem& itemInfo(target.first);
		int count(itemInfo.TakeAll(m_candidate, RE::PlayerCharacter::GetSingleton()));
		bool notify(target.second);
		RE::PlayerCharacter::GetSingleton()->PlayPickUpSound(itemInfo.BoundObject(), true, false);
		if (notify)
		{
			std::string notificationText;
			if (count > 1)
			{
				static RE::BSFixedString multiActivate(papyrus::GetTranslation(nullptr, RE::BSFixedString("$AHSE_ACTIVATE(COUNT)_MSG")));
				if (!multiActivate.empty())
				{
					notificationText = multiActivate;
					Replace(notificationText, "{ITEMNAME}", itemInfo.BoundObject()->GetName());
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
					Replace(notificationText, "{ITEMNAME}", itemInfo.BoundObject()->GetName());
				}
			}
			if (!notificationText.empty())
			{
				RE::DebugNotification(notificationText.c_str());
			}
		}
	}
}

void SearchTask::TriggerObjectGlow(RE::TESObjectREFR* refr, const int duration)
{

	// only send the glow event once per N seconds. This will retrigger on later passes, but once we are out of
	// range no more glowing will be triggered. The item remains in the list until we change cell but there should
	// never be so many in a cell that this is a problem.
	RecursiveLockGuard guard(m_lock);
	const auto existingGlow(m_glowExpiration.find(refr));
	auto currentTime(std::chrono::high_resolution_clock::now());
	if (existingGlow != m_glowExpiration.cend() && existingGlow->second > currentTime)
		return;
	auto expiry = currentTime + std::chrono::milliseconds(static_cast<long long>(duration * 1000.0));
	m_glowExpiration[refr] = expiry;
#if _DEBUG
	_DMESSAGE("Trigger glow for %s/0x%08x", refr->GetName(), refr->formID);
#endif
	onObjectGlow.SendEvent(refr, duration);
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

