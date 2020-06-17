#include "PrecompiledHeaders.h"

#include "tasks.h"
#include "basketfile.h"
#include "debugs.h"
#include "LocationTracker.h"
#include "ExcludedLocations.h"
#include "PopulationCenters.h"
#include "PlayerCellHelper.h"
#include "PlayerHouses.h"
#include "PlayerState.h"
#include "LogStackWalker.h"

#include <chrono>
#include <thread>

INIFile* SearchTask::m_ini = nullptr;

RecursiveLock SearchTask::m_lock;
std::unordered_map<const RE::TESObjectREFR*, std::chrono::time_point<std::chrono::high_resolution_clock>> SearchTask::m_glowExpiration;

SearchTask::SearchTask(RE::TESObjectREFR* candidate, INIFile::SecondaryType targetType)
	: m_candidate(candidate), m_targetType(targetType), m_glowReason(GlowReason::None)
{
}

const int HarvestSpamLimit = 10;

bool SearchTask::IsLootingForbidden()
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
		if (!lootingIsCrime && (LocationTracker::Instance().IsCellSelfOwned() || playerOwned))
		{
			// can configure to not loot my own belongings even though it's always legal
			if (!IsSpecialObjectLootable(m_belongingsCheck))
			{
				DBG_VMESSAGE("Player home or player-owned, looting belongings disallowed: %s/0x%08x",
					m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
				isForbidden = true;
				// Glow if configured
				if (m_belongingsCheck == SpecialObjectHandling::GlowTarget)
					UpdateGlowReason(GlowReason::PlayerProperty);
			}
		}
		// if restricted to law-abiding citizenship, check if OK to loot
		else if (m_crimeCheck > 0)
		{
			if (lootingIsCrime)
			{
				// never commit a crime unless crimeCheck is 0
				DBG_VMESSAGE("Crime to loot REFR, cannot loot");
				isForbidden = true;
			}
			else if (m_crimeCheck == 2 && !playerOwned && m_candidate->GetOwner() != nullptr)
			{
				// owner is not player, disallow
				DBG_VMESSAGE("REFR is owned, cannot loot");
				isForbidden = true;
			}
		}

		if (isForbidden)
		{
			DBG_MESSAGE("Block owned/illegal-to-loot REFR: %s/0x%08x", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
			DataCase::GetInstance()->BlockReference(m_candidate);
		}
	}
	return isForbidden;
}

bool SearchTask::IsBookGlowable() const
{
	RE::BGSKeywordForm* keywordForm(m_candidate->GetBaseObject()->As<RE::BGSKeywordForm>());
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

// Dynamic REFR looting is not delayed - the visuals may be less appealing, but delaying risks CTD as REFRs can
// be recycled very quickly.
bool SearchTask::HasDynamicData(RE::TESObjectREFR* refr)
{
	// do not reregister known REFR
	if (LootedDynamicContainerFormID(refr) != InvalidForm)
		return true;

	// risk exists if REFR or its concrete object is dynamic
	if (refr->IsDynamicForm() || refr->GetBaseObject()->IsDynamicForm())
	{
		DBG_VMESSAGE("dynamic REFR 0x%08x or base 0x%08x for %s", refr->GetFormID(),
			refr->GetBaseObject()->GetFormID(), refr->GetBaseObject()->GetName());
		// record looting so we don't rescan
		MarkDynamicContainerLooted(refr);
		return true;
	}
	return false;
}

std::unordered_map<const RE::TESObjectREFR*, RE::FormID> SearchTask::m_lootedDynamicContainers;
void SearchTask::MarkDynamicContainerLooted(const RE::TESObjectREFR* refr)
{
	RecursiveLockGuard guard(m_lock);
	// record looting so we don't rescan
	m_lootedDynamicContainers.insert(std::make_pair(refr, refr->GetFormID()));
}

RE::FormID SearchTask::LootedDynamicContainerFormID(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return false;
	RecursiveLockGuard guard(m_lock);
	if (m_lootedDynamicContainers.count(refr) > 0)
		return m_lootedDynamicContainers[refr];
	return InvalidForm;
}

// forget about dynamic containers we looted when cell changes. This is more aggressive than static container looting
// as this list contains recycled FormIDs, and hypothetically may grow unbounded.
void SearchTask::ResetLootedDynamicContainers()
{
	RecursiveLockGuard guard(m_lock);
	m_lootedDynamicContainers.clear();
}

std::unordered_set<const RE::TESObjectREFR*> SearchTask::m_lootedContainers;
void SearchTask::MarkContainerLooted(const RE::TESObjectREFR* refr)
{
	RecursiveLockGuard guard(m_lock);
	// record looting so we don't rescan
	m_lootedContainers.insert(refr);
}

bool SearchTask::IsLootedContainer(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return false;
	RecursiveLockGuard guard(m_lock);
	return m_lootedContainers.count(refr) > 0;
}

// forget about containers we looted to allow rescan after game load or config settings update
void SearchTask::ResetLootedContainers()
{
	RecursiveLockGuard guard(m_lock);
	m_lootedContainers.clear();
	m_actorApparentTimeOfDeath.clear();
}

// looting during combat is unstable, so if that option is enabled, we store the combat victims and loot them once combat ends, no sooner 
// than N seconds after their death
void SearchTask::RegisterActorTimeOfDeath(RE::TESObjectREFR* refr)
{
	RecursiveLockGuard guard(m_lock);
	m_actorApparentTimeOfDeath.emplace_back(std::make_pair(refr, std::chrono::high_resolution_clock::now()));
	// record looting so we don't rescan
	MarkContainerLooted(refr);
	DBG_MESSAGE("Enqueued dead body to loot later 0x%08x", refr->GetFormID());
}

std::deque<std::pair<RE::TESObjectREFR*, std::chrono::time_point<std::chrono::high_resolution_clock>>> SearchTask::m_actorApparentTimeOfDeath;

// return false iff output list is full
bool SearchTask::ReleaseReliablyDeadActors(BoundedList<RE::TESObjectREFR*>& refs)
{
	RecursiveLockGuard guard(m_lock);
	const int interval(PlayerState::Instance().PerksAddLeveledItemsOnDeath() ?
		ActorReallyDeadWaitIntervalSecondsLong : ActorReallyDeadWaitIntervalSeconds);
	const auto cutoffPoint(std::chrono::high_resolution_clock::now() - std::chrono::milliseconds(static_cast<long long>(interval * 1000.0)));
	while (!m_actorApparentTimeOfDeath.empty() && m_actorApparentTimeOfDeath.front().second <= cutoffPoint)
	{
		// this actor died long enough ago that we trust actor->GetContainer not to crash, provided the ID is still usable
		RE::TESObjectREFR* refr(m_actorApparentTimeOfDeath.front().first);
		if (!RE::TESForm::LookupByID<RE::TESObjectREFR>(refr->GetFormID()))
		{
			DBG_MESSAGE("Process enqueued dead body 0x%08x", refr->GetFormID());
		}
		else
		{
			DBG_MESSAGE("Suspect enqueued dead body ID 0x%08x", refr->GetFormID());
		}
		m_actorApparentTimeOfDeath.pop_front();
		if (!refs.Add(refr))
			return false;
	}
	return true;
}

void SearchTask::Run()
{
	DataCase* data = DataCase::GetInstance();
	TESObjectREFRHelper refrEx(m_candidate);

	if (m_targetType == INIFile::SecondaryType::itemObjects)
	{
		ObjectType objType = refrEx.GetObjectType();
		std::string typeName = refrEx.GetTypeName();
		// Various form types contain an ingredient that is the final lootable item - resolve here
		RE::TESForm* lootable(DataCase::GetInstance()->GetLootableForProducer(m_candidate->GetBaseObject()));
		if (lootable)
		{
			DBG_VMESSAGE("producer %s/0x%08x has lootable %s/0x%08x", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID,
				lootable->GetName(), lootable->formID);
			refrEx.SetLootable(lootable);
		}
		else if (objType == ObjectType::critter)
		{
			// trigger critter -> ingredient resolution and skip until it's resolved - pending resolve recorded using nullptr,
			// only trigger if not already pending
			DBG_VMESSAGE("resolve critter %s/0x%08x to ingredient", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
			if (DataCase::GetInstance()->SetLootableForProducer(m_candidate->GetBaseObject(), nullptr))
			{
				EventPublisher::Instance().TriggerGetCritterIngredient(m_candidate);
			}
			return;
		}

		if (objType == ObjectType::unknown)
		{
			DBG_VMESSAGE("blacklist objType == ObjectType::unknown for 0x%08x", m_candidate->GetFormID());
			data->BlacklistReference(m_candidate);
			return;
		}

		bool manualLootNotify(m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "ManualLootTargetNotify") != 0);
		if (objType == ObjectType::manualLoot && manualLootNotify)
		{
			// notify about these, just once
			std::string notificationText;
			static RE::BSFixedString manualLootText(papyrus::GetTranslation(nullptr, RE::BSFixedString("$SHSE_MANUAL_LOOT_MSG")));
			if (!manualLootText.empty())
			{
				notificationText = manualLootText;
				StringUtils::Replace(notificationText, "{ITEMNAME}", m_candidate->GetName());
				if (!notificationText.empty())
				{
					RE::DebugNotification(notificationText.c_str());
				}
			}
			DBG_VMESSAGE("notify, then block objType == ObjectType::manualLoot for 0x%08x", m_candidate->GetFormID());
			data->BlockReference(m_candidate);
			return;
		}

		if (BasketFile::GetSingleton()->IsinList(BasketFile::listnum::BLACKLIST, m_candidate->GetBaseObject()))
		{
			DBG_VMESSAGE("register REFR base form 0x%08x in BlackList", m_candidate->GetBaseObject()->GetFormID());
			data->BlockForm(m_candidate->GetBaseObject());
			return;
		}
#if _DEBUG
		DumpReference(refrEx, typeName.c_str());
#endif
		// initially no glow - use synthetic value with highest precedence
		m_glowReason = GlowReason::None;
		bool skipLooting(false);

		bool needsFullQuestFlags(m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "questObjectScope") != 0);
		SpecialObjectHandling questObjectLoot =
			SpecialObjectHandlingFromIniSetting(m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "questObjectLoot"));
		if (refrEx.IsQuestItem(needsFullQuestFlags))
		{
			DBG_VMESSAGE("Quest Item 0x%08x", m_candidate->GetBaseObject()->formID);
			if (questObjectLoot == SpecialObjectHandling::GlowTarget)
			{
				DBG_VMESSAGE("glow quest object %s/0x%08x", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
				UpdateGlowReason(GlowReason::QuestObject);
			}

			skipLooting = skipLooting || !IsSpecialObjectLootable(questObjectLoot);
		}
		// glow unread notes as they are often quest-related
		else if (questObjectLoot == SpecialObjectHandling::GlowTarget && objType == ObjectType::book && IsBookGlowable())
		{
			DBG_VMESSAGE("Glowable book 0x%08x", m_candidate->GetBaseObject()->formID);
			UpdateGlowReason(GlowReason::SimpleTarget);
		}

		if (objType == ObjectType::ammo)
		{
			skipLooting = skipLooting || data->SkipAmmoLooting(m_candidate);
		}

		// order is important to ensure we glow correctly even if blocked
		skipLooting = IsLootingForbidden() || skipLooting;

		if (m_glowReason != GlowReason::None)
		{
			GlowObject(m_candidate, ObjectGlowDurationSpecialSeconds, m_glowReason);
		}

		if (LocationTracker::Instance().IsPlayerInBlacklistedPlace())
		{
			DBG_VMESSAGE("Player location is excluded");
			skipLooting = true;
		}

		// Harvesting and mining is allowed in settlements. We really just want to not auto-loot entire
		// buildings of friendly factions, and the like. Mines and farms mostly self-identify as Settlements.
		if (LocationTracker::Instance().IsPlayerInRestrictedLootSettlement() && 
			!IsItemLootableInPopulationCenter(m_candidate->GetBaseObject(), objType))
		{
			DBG_VMESSAGE("Player location is excluded as unpermitted population center");
			skipLooting = true;
		}

		LootingType lootingType(LootingTypeFromIniSetting(m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::itemObjects, typeName.c_str())));
		if (objType == ObjectType::whitelist)
		{
			// whitelisted objects are always looted silently
			DBG_VMESSAGE("pick up REFR to whitelisted 0x%08x", m_candidate->GetBaseObject()->formID);
			skipLooting = false;
			lootingType = LootingType::LootAlwaysSilent;
		}
		else if (objType == ObjectType::blacklist)
		{
			// blacklisted objects are never looted
			DBG_VMESSAGE("block blacklisted base %s/0x%08x for REFR 0x%08x",
				m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->GetFormID(), m_candidate->GetFormID());
			data->BlockForm(m_candidate->GetBaseObject());
			skipLooting = true;
			lootingType = LootingType::LeaveBehind;
		}
		else if (!skipLooting)
		{
			if (lootingType == LootingType::LeaveBehind)
			{
				DBG_VMESSAGE("Block REFR : LeaveBehind for 0x%08x", m_candidate->GetBaseObject()->formID);
				data->BlockReference(m_candidate);
				skipLooting = true;
			}
			else if (LootingDependsOnValueWeight(lootingType, objType) && TESFormHelper(m_candidate->GetBaseObject()).ValueWeightTooLowToLoot(m_ini))
			{
				DBG_VMESSAGE("block - v/w excludes harvest for 0x%08x", m_candidate->GetBaseObject()->formID);
				data->BlockForm(m_candidate->GetBaseObject());
				skipLooting = true;
			}
		}

		if (skipLooting)
			return;

		// don't try to re-harvest excluded, depleted or malformed ore vein again until we revisit the cell
		if (objType == ObjectType::oreVein)
		{
			DBG_VMESSAGE("do not process oreVein more than once per cell visit: 0x%08x", m_candidate->formID);
			data->BlockReference(m_candidate);
			EventPublisher::Instance().TriggerMining(m_candidate, data->OreVeinResourceType(m_candidate->GetBaseObject()->As<RE::TESObjectACTI>()), manualLootNotify);
		}
		else
		{
			bool isSilent = !LootingRequiresNotification(lootingType);
			DBG_VMESSAGE("Enqueue SmartHarvest event");
			// don't let the backlog of messages get too large, it's about 1 per second
			// Event handler in Papyrus script unlocks the task - do not issue multiple concurrent events on the same REFR
			if (!LockHarvest(m_candidate, isSilent))
				return;
			ObjectType effectiveType(objType);
			if (effectiveType == ObjectType::whitelist)
			{
				// find lootable type if whitelist were not a factor, for Harvest script
				effectiveType = GetREFRObjectType(m_candidate, true);
			}
			EventPublisher::Instance().TriggerHarvest(m_candidate, effectiveType, refrEx.GetItemCount(),
				isSilent || PendingHarvestNotifications() > HarvestSpamLimit, manualLootNotify);
		}
	}
	else if (m_targetType == INIFile::SecondaryType::containers || m_targetType == INIFile::SecondaryType::deadbodies)
	{
		DBG_MESSAGE("scanning container/body %s/0x%08x", m_candidate->GetName(), m_candidate->formID);
#if _DEBUG
		DumpContainer(refrEx);
#endif
		bool requireQuestItemAsTarget = m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "questObjectScope") != 0;
		bool hasQuestObject(false);
		bool hasEnchantItem(false);
		bool skipLooting(false);
		LootableItems lootableItems(
			ContainerLister(m_targetType, m_candidate, requireQuestItemAsTarget).GetOrCheckContainerForms(hasQuestObject, hasEnchantItem));
		if (lootableItems.empty())
		{
			// Nothing lootable here
			DBG_MESSAGE("container %s/0x%08x is empty", m_candidate->GetName(), m_candidate->formID);
			// record looting so we don't rescan
			MarkContainerLooted(m_candidate);
			return;
		}

		// initially no glow - flag using synthetic value with highest precedence
		m_glowReason = GlowReason::None;
		if (m_targetType == INIFile::SecondaryType::containers)
		{
			if (data->IsReferenceLockedContainer(m_candidate))
			{
				SpecialObjectHandling lockedChestLoot =
					SpecialObjectHandlingFromIniSetting(m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "lockedChestLoot"));
				if (lockedChestLoot == SpecialObjectHandling::GlowTarget)
				{
					DBG_VMESSAGE("glow locked container %s/0x%08x", m_candidate->GetName(), m_candidate->formID);
					UpdateGlowReason(GlowReason::LockedContainer);
				}

				skipLooting = skipLooting || !IsSpecialObjectLootable(lockedChestLoot);
			}

			if (IsBossContainer(m_candidate))
			{
				SpecialObjectHandling bossChestLoot = 
					SpecialObjectHandlingFromIniSetting(m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "bossChestLoot"));
				if (bossChestLoot == SpecialObjectHandling::GlowTarget)
				{
					DBG_VMESSAGE("glow boss container %s/0x%08x", m_candidate->GetName(), m_candidate->formID);
					UpdateGlowReason(GlowReason::BossContainer);
				}

				skipLooting = skipLooting || !IsSpecialObjectLootable(bossChestLoot);
			}
		}

		if (hasQuestObject)
		{
			SpecialObjectHandling questObjectLoot =
				SpecialObjectHandlingFromIniSetting(m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "questObjectLoot"));
			if (questObjectLoot == SpecialObjectHandling::GlowTarget)
			{
				DBG_VMESSAGE("glow container with quest object %s/0x%08x", m_candidate->GetName(), m_candidate->formID);
				UpdateGlowReason(GlowReason::QuestObject);
			}

			skipLooting = skipLooting || !IsSpecialObjectLootable(questObjectLoot);
		}

		if (hasEnchantItem)
		{
			SInt32 enchantItemGlow = static_cast<int>(m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "enchantItemGlow"));
			if (enchantItemGlow == 1)
			{
				DBG_VMESSAGE("glow container with enchanted object %s/0x%08x", m_candidate->GetName(), m_candidate->formID);
				UpdateGlowReason(GlowReason::EnchantedItem);
			}
		}
		// order is important to ensure we glow correctly even if blocked
		skipLooting = IsLootingForbidden() || skipLooting;

		if (LocationTracker::Instance().IsPlayerInBlacklistedPlace())
		{
			DBG_VMESSAGE("Player location is excluded");
			skipLooting = true;
		}

		// Always allow auto-looting of dead bodies, e.g. Solitude Hall of the Dead in LCTN Solitude has skeletons that we
		// should be able to murder/plunder. And don't forget Margret in Markarth.
		if (m_targetType != INIFile::SecondaryType::deadbodies &&
			LocationTracker::Instance().IsPlayerInRestrictedLootSettlement())
		{
			DBG_VMESSAGE("Player location is excluded as unpermitted population center");
			skipLooting = true;
		}

		if (m_glowReason != GlowReason::None)
		{
			GlowObject(m_candidate, ObjectGlowDurationSpecialSeconds, m_glowReason);
		}

		// TODO if it contains whitelisted items we will nonetheless skip, due to checks at the container level
		if (skipLooting)
			return;

		// when we get to the point where looting is confirmed, block the reference to
		// avoid re-looting without a player cell or config change
		DBG_MESSAGE("block looted container %s/0x%08x", m_candidate->GetName(), m_candidate->formID);
		data->BlockReference(m_candidate);
		// Build list of lootable targets with count and notification flag for each
		std::vector<std::pair<InventoryItem, bool>> targets;
		targets.reserve(lootableItems.size());
		for (auto& targetItemInfo : lootableItems)
		{
			RE::TESBoundObject* target(targetItemInfo.BoundObject());
			if (!target)
				continue;

			TESFormHelper itemEx(target);

			if (BasketFile::GetSingleton()->IsinList(BasketFile::listnum::BLACKLIST, target))
			{
				DBG_VMESSAGE("block due to BasketFile exclude-list for 0x%08x", target->formID);
				data->BlockForm(target);
				continue;
			}

			ObjectType objType = targetItemInfo.LootObjectType();
			std::string typeName = GetObjectTypeName(objType);

			LootingType lootingType = LootingTypeFromIniSetting(m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::itemObjects, typeName.c_str()));
			if (objType == ObjectType::whitelist)
			{
				// whitelisted objects are always looted silently
				DBG_VMESSAGE("transfer whitelisted 0x%08x", target->formID);
				lootingType = LootingType::LootAlwaysSilent;
			}
			else if (objType == ObjectType::blacklist)
			{
				// blacklisted objects are never looted
				DBG_VMESSAGE("block blacklisted target %s/0x%08x", target->GetName(), target->GetFormID());
				data->BlockForm(target);
				continue;
			}
			else if (lootingType == LootingType::LeaveBehind)
			{
				DBG_VMESSAGE("block - typename %s excluded for 0x%08x", typeName.c_str(), target->formID);
				data->BlockForm(target);
				continue;
			}
			else if (LootingDependsOnValueWeight(lootingType, objType) && itemEx.ValueWeightTooLowToLoot(m_ini))
			{
				DBG_VMESSAGE("block - v/w excludes for 0x%08x", target->formID);
				data->BlockForm(target);
				continue;
			}

			targets.push_back({targetItemInfo, LootingRequiresNotification(lootingType)});
			DBG_MESSAGE("get %s (%d) from container %s/0x%08x", itemEx.m_form->GetName(), targetItemInfo.Count(),
				m_candidate->GetName(), m_candidate->formID);
		}

		if (!targets.empty())
		{
			// check highlighting for dead NPC or container
			int playContainerAnimation(static_cast<int>(m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "PlayContainerAnimation")));
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

			GetLootFromContainer(targets, playContainerAnimation);
		}
	}
}

void SearchTask::GetLootFromContainer(std::vector<std::pair<InventoryItem, bool>>& targets, const int animationType)
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
		// glow looted object briefly after looting
		GlowObject(m_candidate, ObjectGlowDurationLootedSeconds, GlowReason::SimpleTarget);
	}

	for (auto& target : targets)
	{
		// Play sound first as this uses InventoryItemData on the source container
		InventoryItem& itemInfo(target.first);
		bool notify(target.second);
		RE::PlayerCharacter::GetSingleton()->PlayPickUpSound(itemInfo.BoundObject(), true, false);
		std::string name(itemInfo.BoundObject()->GetName());
		int count(itemInfo.TakeAll(m_candidate, RE::PlayerCharacter::GetSingleton()));
		if (notify)
		{
			std::string notificationText;
			if (count > 1)
			{
				static RE::BSFixedString multiActivate(papyrus::GetTranslation(nullptr, RE::BSFixedString("$SHSE_ACTIVATE(COUNT)_MSG")));
				if (!multiActivate.empty())
				{
					notificationText = multiActivate;
					StringUtils::Replace(notificationText, "{ITEMNAME}", name.c_str());
					std::ostringstream intStr;
					intStr << count;
					StringUtils::Replace(notificationText, "{COUNT}", intStr.str());
				}
			}
			else
			{
				static RE::BSFixedString singleActivate(papyrus::GetTranslation(nullptr, RE::BSFixedString("$SHSE_ACTIVATE_MSG")));
				if (!singleActivate.empty())
				{
					notificationText = singleActivate;
					StringUtils::Replace(notificationText, "{ITEMNAME}", name.c_str());
				}
			}
			if (!notificationText.empty())
			{
				RE::DebugNotification(notificationText.c_str());
			}
		}
	}
}


void SearchTask::GlowObject(RE::TESObjectREFR* refr, const int duration, const GlowReason glowReason)
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
	DBG_VMESSAGE("Trigger glow for %s/0x%08x", refr->GetName(), refr->formID);
	EventPublisher::Instance().TriggerObjectGlow(m_candidate, duration, glowReason);
}

RecursiveLock SearchTask::m_searchLock;
bool SearchTask::m_threadStarted = false;
bool SearchTask::m_searchAllowed = false;

int SearchTask::m_crimeCheck = 0;
SpecialObjectHandling SearchTask::m_belongingsCheck = SpecialObjectHandling::GlowTarget;

void SearchTask::TakeNap()
{
	double delay(m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config,
		LocationTracker::Instance().IsPlayerIndoors() ? "IndoorsIntervalSeconds" : "IntervalSeconds"));
	delay = std::max(MinDelay, delay);

	DBG_MESSAGE("wait for %d milliseconds", static_cast<long long>(delay * 1000.0));
	auto nextRunTime = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(static_cast<long long>(delay * 1000.0));
	std::this_thread::sleep_until(nextRunTime);
}

void SearchTask::ScanThread()
{
	REL_MESSAGE("Starting SHSE Worker Thread");
	// record a message periodically if mod remains idle
	constexpr std::chrono::milliseconds TellUserIAmIdle(60000LL);
	m_ini = INIFile::GetInstance();
	std::chrono::time_point<std::chrono::steady_clock> lastScanEndTime(std::chrono::high_resolution_clock::now());
	std::chrono::time_point<std::chrono::steady_clock> lastIdleLogTime(lastScanEndTime);
	while (true)
	{
		// Delay the scan for each loop
		TakeNap();

		if (!EventPublisher::Instance().GoodToGo())
		{
			REL_MESSAGE("Event publisher not ready yet");
			continue;
		}

		// process any queued added items since last time
		CollectionManager::Instance().ProcessAddedItems();

		// Player location checked for Cell/Location change on every loop
		LocationTracker::Instance().Refresh();
		PlayerState::Instance().Refresh();

		if (!UIState::Instance().OKForSearch())
		{
			DBG_MESSAGE("UI state not good to loot");
			const auto timeNow(std::chrono::high_resolution_clock::now());
			const auto timeSinceLastIdleLog(timeNow - lastIdleLogTime);
			const auto timeSinceLastScanEnd(timeNow - lastScanEndTime);
			if (timeSinceLastIdleLog > TellUserIAmIdle && timeSinceLastScanEnd > TellUserIAmIdle)
			{
				REL_MESSAGE("No loot scan in the past %lld seconds", std::chrono::duration_cast<std::chrono::seconds>(timeSinceLastScanEnd).count());
				lastIdleLogTime = timeNow;
			}
			continue;
		}
		if (!LocationTracker::Instance().IsPlayerInLootablePlace())
		{
			DBG_MESSAGE("Location cannot be looted");
			continue;
		}
		if (!PlayerState::Instance().CanLoot())
		{
			DBG_MESSAGE("Player State prevents looting");
			continue;
		}
		if (!IsAllowed())
		{
			DBG_MESSAGE("search disallowed");
			const auto timeNow(std::chrono::high_resolution_clock::now());
			const auto timeSinceLastIdleLog(timeNow - lastIdleLogTime);
			const auto timeSinceLastScanEnd(timeNow - lastScanEndTime);
			if (timeSinceLastIdleLog > TellUserIAmIdle && timeSinceLastScanEnd > TellUserIAmIdle)
			{
				REL_MESSAGE("No loot scan in the past %lld seconds", std::chrono::duration_cast<std::chrono::seconds>(timeSinceLastScanEnd).count());
				lastIdleLogTime = timeNow;
			}
			continue;
		}
		// re-evaluate perks if timer has popped - no force, and execute scan
		PlayerState::Instance().CheckPerks(false);
		DoPeriodicSearch();

		// request added items to be pushed to us while we are sleeping
		EventPublisher::Instance().TriggerFlushAddedItems();
		lastScanEndTime = std::chrono::high_resolution_clock::now();
	}
}

void SearchTask::Start()
{
	// do not start the thread if we failed to initialize
	if (!m_pluginOK)
		return;
	std::thread([]()
	{
		// use structured exception handling to get stack walk on windows exceptions
		__try
		{
			ScanThread();
		}
		__except (LogStackWalker::LogStack(GetExceptionInformation()))
		{
		}
	}).detach();
}

void SearchTask::ResetRestrictions(const bool gameReload)
{
	DataCase::GetInstance()->ListsClear(gameReload);

	DBG_MESSAGE("Unlock task-pending REFRs");
	RecursiveLockGuard guard(m_lock);
	// unblock all blocked auto-harvest objects
	m_HarvestLock.clear();

	// Dynamic containers that we looted reset on cell change
	ResetLootedDynamicContainers();

	if (gameReload)
	{
		// unblock possible player house checks after game reload
		// TODO is this correct
		PlayerHouses::Instance().Clear();
		// clear list of dead bodies pending looting - blocked reference cleanup allows redo if still viable
		ResetLootedContainers();
		// reset excluded locations
		// TODO blacklist has to play nice with this
		ExcludedLocations::Instance().Reset();
		// TODO reset Collections State from the saved-game data
	}
	// clean up the list of glowing objects, don't futz with EffectShader since cannot run scripts at this time
	m_glowExpiration.clear();
}

bool SearchTask::m_pluginOK(false);

void SearchTask::OnGoodToGo()
{
	REL_MESSAGE("UI/controls now good-to-go");
	// reset state that might be invalidated by MCM setting updates
	PlayerState::Instance().CheckPerks(true);
	// reset carry weight - will reinstate correct value if/when scan resumes
	PlayerState::Instance().ResetCarryWeight();

	// update Locked Container last-accessed time
	DataCase::GetInstance()->UpdateLockedContainers();
	// Base Object Forms and REFRs handled for the case where we are not reloading game
	DataCase::GetInstance()->ResetBlockedForms();
	DataCase::GetInstance()->ClearBlockedReferences(false);
	// clear list of dead bodies pending looting - blocked reference cleanup allows redo if still viable
	ResetLootedContainers();
}

void SearchTask::DoPeriodicSearch()
{
	DataCase* data = DataCase::GetInstance();
	if (!data)
		return;
	bool sneaking(false);
	{
#ifdef _PROFILING
		WindowsUtils::ScopedTimer elapsed("Periodic Search pre-checks");
#endif
		{
			// narrowly-scoped lock, do not need if this check passes
			RecursiveLockGuard guard(m_lock);
			if (!m_pluginSynced)
			{
				DBG_MESSAGE("Plugin sync still pending");
				return;
			}
		}
	}

	// Retrieve these settings only once
	m_crimeCheck = static_cast<int>(m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config,
		PlayerState::Instance().IsSneaking() ? "crimeCheckSneaking" : "crimeCheckNotSneaking"));
	m_belongingsCheck = SpecialObjectHandlingFromIniSetting(m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "playerBelongingsLoot"));

	// This logic needs to reliably handle load spikes. We do not commit to process more than N references. The rest will get processed on future passes.
	// A spike of 200+ in a second makes the VM dump stacks, so pick N accordingly.
	// Stress tested using Jorrvaskr with personal property looting turned on. It's more important to loot in an orderly fashion than to get it all into inventory on
	// one pass.
	// Process any queued dead body that is dead long enough to have played kill animation. We do this first to avoid being queued up behind new info for ever
	BoundedList<RE::TESObjectREFR*> refrs(MaxREFRSPerPass);
	if (ReleaseReliablyDeadActors(refrs))
	{
	    // space remains to process loot after corpses checked
		AbsoluteRange rangeCheck(RE::PlayerCharacter::GetSingleton(), LocationTracker::Instance().IsPlayerIndoors() ?
			m_ini->GetIndoorsRadius(INIFile::PrimaryType::harvest) : m_ini->GetRadius(INIFile::PrimaryType::harvest));
		PlayerCellHelper(refrs, rangeCheck).FindLootableReferences();
	}

	for (RE::TESObjectREFR* refr : refrs.Data())
	{
		// Filter out borked REFRs. PROJ repro observed in logs as below:
		/*
			0x15f0 (2020-05-17 14:05:27.290) J:\GitHub\SmartHarvestSE\utils.cpp(211): [MESSAGE] TIME(Filter loot candidates in/near cell)=54419 micros
			0x15f0 (2020-05-17 14:05:27.290) J:\GitHub\SmartHarvestSE\tasks.cpp(1037): [MESSAGE] Process REFR 0x00000000 with base object Iron Arrow/0x0003be11
			0x15f0 (2020-05-17 14:05:27.290) J:\GitHub\SmartHarvestSE\utils.cpp(211): [MESSAGE] TIME(Process Auto-loot Candidate Iron Arrow/0x0003be11)=35 micros

			0x15f0 (2020-05-17 14:05:31.950) J:\GitHub\SmartHarvestSE\utils.cpp(211): [MESSAGE] TIME(Filter loot candidates in/near cell)=54195 micros
			0x15f0 (2020-05-17 14:05:31.950) J:\GitHub\SmartHarvestSE\tasks.cpp(1029): [MESSAGE] REFR 0x00000000 has no Base Object
		*/
		// Similar scenario seen when transitioning from indoors to outdoors (Blue Palace) - could this be any 'temp' REFRs being cleaned up, for various reasons?

		if (refr->GetFormID() == InvalidForm)
		{
			DBG_WARNING("REFR has invalid FormID");
			data->BlacklistReference(refr);
			continue;
		}
		else if (!refr->GetBaseObject())
		{
			DBG_WARNING("REFR 0x%08x has no Base Object", refr->GetFormID());
			data->BlacklistReference(refr);
			continue;
		}
		else
		{
			DBG_VMESSAGE("Process REFR 0x%08x with base object %s/0x%08x", refr->GetFormID(),
				refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID());
		}

		INIFile::SecondaryType lootTargetType = INIFile::SecondaryType::itemObjects;
		{
#ifdef _PROFILING
			WindowsUtils::ScopedTimer elapsed("Process Auto-loot Candidate", refr);
#endif
			if (!refr)
				continue;
			RE::Actor* actor(nullptr);
			if ((actor = refr->GetBaseObject()->As<RE::Actor>()) || refr->GetBaseObject()->As<RE::TESNPC>())
			{
				if (m_ini->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "enableLootDeadbody") == 0.0 || !refr->IsDead(true))
					continue;

				if (actor)
				{
					ActorHelper actorEx(actor);
					if (actorEx.IsPlayerAlly() || actorEx.IsEssential() || actorEx.IsSummoned())
					{
						data->BlockReference(refr);
						continue;
					}
				}

				lootTargetType = INIFile::SecondaryType::deadbodies;
				// Delay looting exactly once. We only return here after required time since death has expired.
				if (!HasDynamicData(refr) && !IsLootedContainer(refr))
				{
					// Use async looting to allow game to settle actor state and animate their untimely demise
					RegisterActorTimeOfDeath(refr);
					continue;
				}
			}
			else if (refr->GetBaseObject()->As<RE::TESContainer>())
			{
				if (m_ini->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "EnableLootContainer") == 0.0)
					continue;
				lootTargetType = INIFile::SecondaryType::containers;
			}
			else if (refr->GetBaseObject()->As<RE::TESObjectACTI>() && HasAshPile(refr))
			{
				if (m_ini->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "enableLootDeadbody") == 0.0)
					continue;
				lootTargetType = INIFile::SecondaryType::deadbodies;
				// Delay looting exactly once. We only return here after required time since death has expired.
				if (!HasDynamicData(refr) && !IsLootedContainer(refr))
				{
					// Use async looting to allow game to settle actor state and animate their untimely demise
					RegisterActorTimeOfDeath(refr);
					continue;
				}
				// deferred looting of dead bodies - introspect ExtraDataList to get the REFR
#if _DEBUG
				RE::TESObjectREFR* originalRefr(refr);
#endif
				refr = GetAshPile(refr);
#if _DEBUG
				DBG_MESSAGE("Got ash-pile REFR 0x%08x from REFR 0x%08x", refr->GetFormID(), originalRefr->GetFormID());
#endif
			}
			else if (m_ini->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "enableHarvest") == 0.0)
			{
				continue;
			}
		}
		SearchTask(refr, lootTargetType).Run();
	}
}

void SearchTask::PrepareForReload()
{
	// stop scanning
	Disallow();

	// force recheck Perks and reset carry weight
	static const bool force(true);
	PlayerState::Instance().CheckPerks(force);

	// reset carry weight and menu-active state
	PlayerState::Instance().ResetCarryWeight();
	UIState::Instance().Reset();

	// reset player location - reload may bring us back in a different place and even if not, we should start from scratch
	LocationTracker::Instance().Reset();

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

std::unordered_set<const RE::TESObjectREFR*> SearchTask::m_HarvestLock;
int SearchTask::m_pendingNotifies = 0;

bool SearchTask::LockHarvest(const RE::TESObjectREFR* refr, const bool isSilent)
{
	RecursiveLockGuard guard(m_lock);
	if (!refr)
		return false;
	if ((m_HarvestLock.insert(refr)).second)
	{
		if (!isSilent)
			++m_pendingNotifies;
		return true;
	}
	return false;
}

bool SearchTask::UnlockHarvest(const RE::TESObjectREFR* refr, const bool isSilent)
{
	RecursiveLockGuard guard(m_lock);
	if (!refr)
		return false;
	if (m_HarvestLock.erase(refr) > 0)
	{
		if (!isSilent)
			--m_pendingNotifies;
		return true;
	}
	return false;
}

bool SearchTask::IsLockedForHarvest(const RE::TESObjectREFR* refr)
{
	RecursiveLockGuard guard(m_lock);
	return (refr && m_HarvestLock.count(refr));
}

size_t SearchTask::PendingHarvestNotifications()
{
	RecursiveLockGuard guard(m_lock);
	return m_pendingNotifies;
}

bool SearchTask::m_pluginSynced(false);

// this is the last function called by the scripts when re-syncing state
void SearchTask::MergeBlackList()
{
	RecursiveLockGuard guard(m_lock);
	// Add loaded locations to the list of exclusions
	BasketFile::GetSingleton()->SyncList(BasketFile::listnum::BLACKLIST);
	for (const auto exclusion : BasketFile::GetSingleton()->GetList(BasketFile::listnum::BLACKLIST))
	{
		ExcludedLocations::Instance().Add(exclusion);
	}
	// reset blocked lists to allow recheck vs current state
	static const bool gameReload(true);
	ResetRestrictions(gameReload);

	// need to wait for the scripts to sync up before performing player house checks
	m_pluginSynced = true;
}

bool SearchTask::Init()
{
    if (!m_pluginOK)
	{
		// Use structured exception handling during game data load
		REL_MESSAGE("Plugin not synced up - Game Data load executing");
		__try
		{
#ifdef _PROFILING
			WindowsUtils::ScopedTimer elapsed("Categorize Lootables");
#endif
			if (!LoadOrder::Instance().Analyze())
			{
				REL_FATALERROR("Load Order unsupportable");
				return false;
			}
			DataCase::GetInstance()->CategorizeLootables();
			PopulationCenters::Instance().Categorize();
			m_pluginOK = true;
			REL_MESSAGE("Plugin now in sync - Game Data load complete!");
	}
		__except (LogStackWalker::LogStack(GetExceptionInformation()))
		{
			REL_FATALERROR("Fatal Exception during Game Data load");
			return false;
		}
	}
	static const bool gameReload(true);
	ResetRestrictions(gameReload);
	REL_MESSAGE("Restrictions reset for new/loaded game");
	return true;
}

