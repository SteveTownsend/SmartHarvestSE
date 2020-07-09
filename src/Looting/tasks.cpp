#include "PrecompiledHeaders.h"
#include "Utilities/versiondb.h"

#include "Data/dataCase.h"
#include "Data/LoadOrder.h"
#include "Looting/tasks.h"
#include "Utilities/debugs.h"
#include "Utilities/utils.h"
#include "WorldState/ActorTracker.h"
#include "WorldState/LocationTracker.h"
#include "Looting/ManagedLists.h"
#include "Looting/objects.h"
#include "Looting/LootableREFR.h"
#include "WorldState/PopulationCenters.h"
#include "FormHelpers/FormHelper.h"
#include "Looting/ReferenceFilter.h"
#include "WorldState/PlayerHouses.h"
#include "WorldState/PlayerState.h"
#include "Looting/ProducerLootables.h"
#include "Utilities/LogStackWalker.h"
#include "Collections/CollectionManager.h"
#include "VM/EventPublisher.h"
#include "VM/papyrus.h"

#include <chrono>
#include <thread>

namespace shse
{

INIFile* SearchTask::m_ini = nullptr;

RecursiveLock SearchTask::m_lock;
std::unordered_map<const RE::TESObjectREFR*, std::chrono::time_point<std::chrono::high_resolution_clock>> SearchTask::m_glowExpiration;

SearchTask::SearchTask(RE::TESObjectREFR* target, INIFile::SecondaryType targetType)
	: m_candidate(target), m_targetType(targetType), m_glowReason(GlowReason::None)
{
}

const int HarvestSpamLimit = 10;

bool SearchTask::IsLootingForbidden(const INIFile::SecondaryType targetType)
{
	bool isForbidden(false);
	// Perform crime checks - this is done after checks for quest object glowing, as many quest-related objects are owned.
	// Ownership expires with the target, e.g. Francis the Horse from Interesting NPCs was killed by a wolf in Solitude
	// and becomes lootable thereafter.
	// For non-dead targets, check law-abiding settings vs criminality of target and player-ownership settings vs ownership
	if (targetType != INIFile::SecondaryType::deadbodies)
	{
		// check up to three ownership conditions depending on config
		bool playerOwned(IsPlayerOwned(m_candidate));
		// Fired arrows are marked as player owned but we don't want to prevent pickup, ever
		bool firedArrow(m_candidate->formType == RE::FormType::ProjectileArrow);
		bool lootingIsCrime(m_candidate->IsOffLimits());
		if (!lootingIsCrime && playerOwned && !firedArrow)
		{
			// can configure to not loot my own belongings even though it's always legal
			if (!IsSpecialObjectLootable(m_belongingsCheck))
			{
				DBG_VMESSAGE("Player-owned %s, looting belongings disallowed: %s/0x%08x",
					playerOwned ? "true" : "false",	m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
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
			else if (m_crimeCheck == 2 && !playerOwned && !firedArrow && m_candidate->GetOwner() != nullptr)
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
	const auto looted(m_lootedDynamicContainers.find(refr));
	return looted != m_lootedDynamicContainers.cend() ? looted->second : InvalidForm;
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
}

void SearchTask::RegisterActorTimeOfDeath(RE::TESObjectREFR* refr)
{
	shse::ActorTracker::Instance().RecordTimeOfDeath(refr);
	RecursiveLockGuard guard(m_lock);
	// record looting so we don't rescan
	MarkContainerLooted(refr);
}

void SearchTask::Run()
{
	DataCase* data = DataCase::GetInstance();

	if (m_targetType == INIFile::SecondaryType::itemObjects)
	{
		LootableREFR refrEx(m_candidate, m_targetType);
		ObjectType objType = refrEx.GetObjectType();
		std::string typeName = refrEx.GetTypeName();
		// Various form types contain an ingredient or FormList that is the final lootable item - resolve here
		if (objType == ObjectType::critter)
		{
			RE::TESForm* lootable(ProducerLootables::Instance().GetLootableForProducer(m_candidate->GetBaseObject()));
			if (lootable)
			{
				DBG_VMESSAGE("producer %s/0x%08x has lootable %s/0x%08x", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID,
					lootable->GetName(), lootable->formID);
				refrEx.SetLootable(lootable);
			}
			else
			{
				// trigger critter -> ingredient resolution and skip until it's resolved - pending resolve recorded using nullptr,
				// only trigger if not already pending
				DBG_VMESSAGE("resolve critter %s/0x%08x to ingredient", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
				if (ProducerLootables::Instance().SetLootableForProducer(m_candidate->GetBaseObject(), nullptr))
				{
					EventPublisher::Instance().TriggerGetProducerLootable(m_candidate);
				}
				return;
			}
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

		if (ManagedList::BlackList().Contains(m_candidate->GetBaseObject()))
		{
			DBG_VMESSAGE("Block BlackListed REFR base form 0x%08x", m_candidate->GetBaseObject()->GetFormID());
			data->BlockForm(m_candidate->GetBaseObject());
			return;
		}
#if _DEBUG
		DumpReference(refrEx, typeName.c_str(), m_targetType);
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

		const auto collectible(refrEx.TreatAsCollectible());
		SpecialObjectHandling collectibleAction(collectible.second);
		if (collectible.first)
		{
			DBG_VMESSAGE("Collectible Item 0x%08x", m_candidate->GetBaseObject()->formID);
			if (collectibleAction == SpecialObjectHandling::GlowTarget)
			{
				DBG_VMESSAGE("glow collectible object %s/0x%08x", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
				UpdateGlowReason(GlowReason::Collectible);
			}

			skipLooting = skipLooting || !IsSpecialObjectLootable(collectibleAction);
		}

		SpecialObjectHandling valuableLoot =
			SpecialObjectHandlingFromIniSetting(m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "ValuableItemLoot"));
		if (refrEx.IsValuable())
		{
			DBG_VMESSAGE("Valuable Item 0x%08x", m_candidate->GetBaseObject()->formID);
			if (valuableLoot == SpecialObjectHandling::GlowTarget)
			{
				DBG_VMESSAGE("glow valuable object %s/0x%08x", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID);
				UpdateGlowReason(GlowReason::Valuable);
			}

			skipLooting = skipLooting || !IsSpecialObjectLootable(valuableLoot);
		}

		if (objType == ObjectType::ammo)
		{
			skipLooting = skipLooting || data->SkipAmmoLooting(m_candidate);
		}

		// order is important to ensure we glow correctly even if blocked
		bool forbidden(IsLootingForbidden(m_targetType));
		skipLooting = forbidden || skipLooting;

		if (m_glowReason != GlowReason::None)
		{
			GlowObject(m_candidate, ObjectGlowDurationSpecialSeconds, m_glowReason);
		}

		// Harvesting and mining is allowed in settlements. We really just want to not auto-loot entire
		// buildings of friendly factions, and the like. Mines and farms mostly self-identify as Settlements.
		if (!LocationTracker::Instance().IsPlayerInWhitelistedPlace(LocationTracker::Instance().PlayerCell()) &&
			LocationTracker::Instance().IsPlayerInRestrictedLootSettlement(LocationTracker::Instance().PlayerCell()) &&
			!IsItemLootableInPopulationCenter(m_candidate->GetBaseObject(), objType))
		{
			DBG_VMESSAGE("Player location is excluded as restricted population center for this item");
			skipLooting = true;
		}

		LootingType lootingType(LootingType::LeaveBehind);
		if (collectible.first)
		{
			// ** if configured as permitted ** collectible objects are always looted silently
			DBG_VMESSAGE("check REFR to collectible 0x%08x", m_candidate->GetBaseObject()->formID);
			skipLooting = forbidden || collectibleAction != SpecialObjectHandling::DoLoot;
			lootingType = collectibleAction == SpecialObjectHandling::DoLoot ? LootingType::LootAlwaysSilent : LootingType::LeaveBehind;
			if (lootingType == LootingType::LeaveBehind)
			{
				// this is a blacklist collection
				DBG_VMESSAGE("block blacklist collection member 0x%08x", m_candidate->GetBaseObject()->formID);
				data->BlockForm(m_candidate->GetBaseObject());
			}
		}
		else if (ManagedList::WhiteList().Contains(m_candidate->GetBaseObject()))
		{
			// ** if configured as permitted ** whitelisted objects are always looted silently
			DBG_VMESSAGE("check REFR to whitelisted 0x%08x", m_candidate->GetBaseObject()->formID);
			skipLooting = forbidden;
			lootingType = LootingType::LootAlwaysSilent;
		}
		else if (ManagedList::BlackList().Contains(m_candidate->GetBaseObject()))
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
			lootingType = LootingTypeFromIniSetting(
				m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::itemObjects, typeName.c_str()));
			if (lootingType == LootingType::LeaveBehind)
			{
				DBG_VMESSAGE("Block REFR : LeaveBehind for 0x%08x", m_candidate->GetBaseObject()->formID);
				data->BlockReference(m_candidate);
				skipLooting = true;
			}
			else if (LootingDependsOnValueWeight(lootingType, objType))
			{
				TESFormHelper helper(m_candidate->GetBaseObject(), m_targetType);
				if (helper.ValueWeightTooLowToLoot())
				{
					DBG_VMESSAGE("block - v/w excludes harvest for 0x%08x", m_candidate->GetBaseObject()->formID);
					data->BlockForm(m_candidate->GetBaseObject());
					skipLooting = true;
				}
				DBG_VMESSAGE("%s/0x%08x value:%d", m_candidate->GetBaseObject()->GetName(), m_candidate->GetBaseObject()->formID, int(helper.GetWorth()));
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
			DBG_VMESSAGE("SmartHarvest %s/0x%08x for REFR 0x%08x", m_candidate->GetBaseObject()->GetName(),
				m_candidate->GetBaseObject()->GetFormID(), m_candidate->GetFormID());
			// don't let the backlog of messages get too large, it's about 1 per second
			// Event handler in Papyrus script unlocks the task - do not issue multiple concurrent events on the same REFR
			if (!LockHarvest(m_candidate, isSilent))
				return;
			EventPublisher::Instance().TriggerHarvest(m_candidate, objType, refrEx.GetItemCount(),
				isSilent || PendingHarvestNotifications() > HarvestSpamLimit, manualLootNotify, collectible.first);
		}
	}
	else if (m_targetType == INIFile::SecondaryType::containers || m_targetType == INIFile::SecondaryType::deadbodies)
	{
		DBG_MESSAGE("scanning container/body %s/0x%08x", m_candidate->GetName(), m_candidate->formID);
#if _DEBUG
		DumpContainer(LootableREFR(m_candidate, m_targetType));
#endif
		bool requireQuestItemAsTarget = m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "questObjectScope") != 0;
		bool skipLooting(false);
		// INI defaults exclude nudity by not looting armor from dead bodies
		bool excludeArmor(m_targetType == INIFile::SecondaryType::deadbodies &&
			DeadBodyLootingFromIniSetting(m_ini->GetSetting(
				INIFile::PrimaryType::common, INIFile::SecondaryType::config, "EnableLootDeadbody")) == DeadBodyLooting::LootExcludingArmor);
		ContainerLister lister(m_targetType, m_candidate, requireQuestItemAsTarget);
		LootableItems lootableItems(lister.GetOrCheckContainerForms());
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

		if (lister.HasQuestItem())
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

		if (lister.HasEnchantedItem())
		{
			SInt32 enchantItemGlow = static_cast<int>(m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "enchantItemGlow"));
			if (enchantItemGlow == 1)
			{
				DBG_VMESSAGE("glow container with enchanted object %s/0x%08x", m_candidate->GetName(), m_candidate->formID);
				UpdateGlowReason(GlowReason::EnchantedItem);
			}
		}

		if (lister.HasValuableItem())
		{
			SpecialObjectHandling valuableLoot =
				SpecialObjectHandlingFromIniSetting(m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "ValuableItemLoot"));
			if (valuableLoot == SpecialObjectHandling::GlowTarget)
			{
				DBG_VMESSAGE("glow container with valuable object %s/0x%08x", m_candidate->GetName(), m_candidate->formID);
				UpdateGlowReason(GlowReason::Valuable);
			}

			skipLooting = skipLooting || !IsSpecialObjectLootable(valuableLoot);
		}

		if (lister.HasCollectibleItem())
		{
			if (lister.CollectibleAction() == SpecialObjectHandling::GlowTarget)
			{
				DBG_VMESSAGE("glow container with collectible object %s/0x%08x", m_candidate->GetName(), m_candidate->formID);
				UpdateGlowReason(GlowReason::Collectible);
			}

			skipLooting = skipLooting || !IsSpecialObjectLootable(lister.CollectibleAction());
		}
		// Order is important to ensure we glow correctly even if blocked
		// Check here is on the container, skip all contents if not looting permitted
		skipLooting = IsLootingForbidden(m_targetType) || skipLooting;

		// Always allow auto-looting of dead bodies, e.g. Solitude Hall of the Dead in LCTN Solitude has skeletons that we
		// should be able to murder/plunder. And don't forget Margret in Markarth.
		if (m_targetType != INIFile::SecondaryType::deadbodies &&
			!LocationTracker::Instance().IsPlayerInWhitelistedPlace(LocationTracker::Instance().PlayerCell()) &&
			LocationTracker::Instance().IsPlayerInRestrictedLootSettlement(LocationTracker::Instance().PlayerCell()))
		{
			DBG_VMESSAGE("Player location is excluded as restricted population center for this target type");
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
		// Build list of lootable targets with notification and collectibility flag for each
		std::vector<std::tuple<InventoryItem, bool, bool>> targets;
		targets.reserve(lootableItems.size());
		for (auto& targetItemInfo : lootableItems)
		{
			RE::TESBoundObject* target(targetItemInfo.BoundObject());
			if (!target)
				continue;

			// crime-check this REFR from the container as individual object
			if (IsLootingForbidden(INIFile::SecondaryType::itemObjects))
			{
				continue;
			}

			if (ManagedList::BlackList().Contains(target))
			{
				DBG_VMESSAGE("block 0x%08x due to BlackList", target->formID);
				data->BlockForm(target);
				continue;
			}

			ObjectType objType = targetItemInfo.LootObjectType();
			if (excludeArmor && (objType == ObjectType::armor || objType == ObjectType::enchantedArmor))
			{
				// obey SFW setting, for this REFR on this pass - state resets on game reload/cell re-entry/MCM update
				DBG_VMESSAGE("block looting of armor from dead body %s/0x%08x", target->GetName(), target->GetFormID());
				continue;
			}

			if (objType == ObjectType::weapon || objType == ObjectType::armor || objType == ObjectType::jewelry)
			{
				bool hasEnchantment = GetEnchantmentFromExtraLists(targetItemInfo.GetExtraDataLists()) != nullptr;
				if (hasEnchantment) {
					DBG_VMESSAGE("%s/0x%08x has player-created enchantment",
						targetItemInfo.BoundObject()->GetName(), targetItemInfo.BoundObject()->GetFormID());
					switch (objType)
					{
					case ObjectType::weapon:
						objType = ObjectType::enchantedWeapon;
						break;
					case ObjectType::armor:
						objType = ObjectType::enchantedArmor;
						break;
					case ObjectType::jewelry:
						objType = ObjectType::enchantedJewelry;
						break;
					default:
						break;
					}
				}
			}

			LootingType lootingType(LootingType::LeaveBehind);
			const auto collectible(shse::CollectionManager::Instance().TreatAsCollectible(
				shse::ConditionMatcher(targetItemInfo.BoundObject(), m_targetType)));
			if (collectible.first)
			{
				if (IsSpecialObjectLootable(collectible.second))
				{
					DBG_VMESSAGE("Collectible Item 0x%08x", targetItemInfo.BoundObject()->formID);
					lootingType = LootingType::LootAlwaysSilent;
				}
				else
				{
					// blacklisted or 'glow'
					DBG_VMESSAGE("Collectible Item 0x%08x skipped", targetItemInfo.BoundObject()->formID);
					continue;
				}
			}
			else if (ManagedList::WhiteList().Contains(target))
			{
				// whitelisted objects are always looted silently
				DBG_VMESSAGE("transfer whitelisted 0x%08x", target->formID);
				lootingType = LootingType::LootAlwaysSilent;
			}
			else if (ManagedList::BlackList().Contains(target))
			{
				// blacklisted objects are never looted
				DBG_VMESSAGE("block blacklisted target %s/0x%08x", target->GetName(), target->GetFormID());
				data->BlockForm(target);
				continue;
			}
			else
			{
				std::string typeName = GetObjectTypeName(objType);
				lootingType = LootingTypeFromIniSetting(
					m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::itemObjects, typeName.c_str()));

				if (lootingType == LootingType::LeaveBehind)
				{
					DBG_VMESSAGE("block - typename %s excluded for 0x%08x", typeName.c_str(), target->formID);
					data->BlockForm(target);
					continue;
				}
				else if (LootingDependsOnValueWeight(lootingType, objType) &&
					TESFormHelper(target, m_targetType).ValueWeightTooLowToLoot())
				{
					DBG_VMESSAGE("block - v/w excludes for 0x%08x", target->formID);
					data->BlockForm(target);
					continue;
				}
			}

			targets.push_back({targetItemInfo, LootingRequiresNotification(lootingType), collectible.first });
			DBG_MESSAGE("get %s (%d) from container %s/0x%08x", target->GetName(), targetItemInfo.Count(),
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
					if (!GetTimeController(m_candidate))
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

void SearchTask::GetLootFromContainer(std::vector<std::tuple<InventoryItem, bool, bool>>& targets, const int animationType)
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
		InventoryItem& itemInfo(std::get<0>(target));
		bool notify(std::get<1>(target));
		bool collectible(std::get<2>(target));
		RE::PlayerCharacter::GetSingleton()->PlayPickUpSound(itemInfo.BoundObject(), true, false);
		std::string name(itemInfo.BoundObject()->GetName());
		int count(itemInfo.TakeAll(m_candidate, RE::PlayerCharacter::GetSingleton(), collectible));
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
	if (m_calibrating)
	{
		// use hard-coded delay to make UX comprehensible
		delay = double(CalibrationDelay);
	}

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

		{
			// narrowly-scoped lock, do not need if this check passes. Go no further if game load is in progress.
			RecursiveLockGuard guard(m_lock);
			if (!m_pluginSynced)
			{
				REL_MESSAGE("Plugin sync still pending");
				continue;
			}
		}

		if (!EventPublisher::Instance().GoodToGo())
		{
			REL_MESSAGE("Event publisher not ready yet");
			continue;
		}

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

		// Player location checked for Cell/Location change on every loop, provided UI ready for status updates
		if (!LocationTracker::Instance().Refresh())
		{
			REL_VMESSAGE("Location or cell not stable yet");
			continue;
		}

		shse::PlayerState::Instance().Refresh();

		// process any queued added items since last time
		shse::CollectionManager::Instance().ProcessAddedItems();

		// Skip loot-OK checks if calibrating
		if (!m_calibrating)
		{
			// Limited looting is possible on a per-item basis, so proceed with scan if this is the only reason to skip
			static const bool allowIfRestricted(true);
			if (!LocationTracker::Instance().IsPlayerInLootablePlace(LocationTracker::Instance().PlayerCell(), allowIfRestricted))
			{
				DBG_MESSAGE("Location cannot be looted");
				continue;
			}
			if (!shse::PlayerState::Instance().CanLoot())
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
			shse::PlayerState::Instance().CheckPerks(false);
		}

		DoPeriodicSearch();

		// request added items to be pushed to us while we are sleeping
		shse::CollectionManager::Instance().Refresh();
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
		PlayerHouses::Instance().Clear();
		// reset Actor data
		shse::ActorTracker::Instance().Reset();
		// clear list of looted containers
		ResetLootedContainers();
		// Reset Collections State and reapply the saved-game data
		shse::CollectionManager::Instance().OnGameReload();
	}
	// clean up the list of glowing objects, don't futz with EffectShader since cannot run scripts at this time
	m_glowExpiration.clear();
}

bool SearchTask::m_pluginOK(false);

void SearchTask::OnGoodToGo()
{
	REL_MESSAGE("UI/controls now good-to-go");
	// reset state that might be invalidated by MCM setting updates
	shse::PlayerState::Instance().CheckPerks(true);
	// reset carry weight - will reinstate correct value if/when scan resumes. Not a game reload.
	static const bool reloaded(false);
	shse::PlayerState::Instance().ResetCarryWeight(reloaded);

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
	if (m_calibrating)
	{
		// send the message first, it's super-slow compared to scan
		if (m_glowDemo)
		{
			m_nextGlow = CycleGlow(m_nextGlow);
			std::ostringstream glowText;
			glowText << "Glow demo: " << GlowName(m_nextGlow) << ", hold Pause key for 3 seconds to terminate";
			RE::DebugNotification(glowText.str().c_str());
		}
		else
		{
			static RE::BSFixedString rangeText(papyrus::GetTranslation(nullptr, RE::BSFixedString("$SHSE_DISTANCE")));
			if (!rangeText.empty())
			{
				std::string notificationText("Range: ");
				notificationText.append(rangeText);
				StringUtils::Replace(notificationText, "{0}", std::to_string(m_calibrateRadius));
				notificationText.append(", hold Pause key for 3 seconds to terminate");
				if (!notificationText.empty())
				{
					RE::DebugNotification(notificationText.c_str());
				}
			}
		}

		// brain-dead item scan and brief glow - ignores doors for simplicity
		BracketedRange rangeCheck(RE::PlayerCharacter::GetSingleton(),
			(double(m_calibrateRadius) - double(m_calibrateDelta)) / DistanceUnitInFeet, m_calibrateDelta / DistanceUnitInFeet);
		DistanceToTarget targets;
		ReferenceFilter(targets, rangeCheck, false, MaxREFRSPerPass).FindAllCandidates();
		for (auto target : targets)
		{
			DBG_VMESSAGE("Trigger glow for %s/0x%08x at distance %.2f units", target.second->GetName(), target.second->formID, target.first);
			EventPublisher::Instance().TriggerObjectGlow(target.second, ObjectGlowDurationCalibrationSeconds,
				m_glowDemo ? m_nextGlow : GlowReason::SimpleTarget);
		}

		// glow demo runs forever at the same radius, range calibration stops after the outer limit
		if (!m_glowDemo)
		{
			m_calibrateRadius += m_calibrateDelta;
			if (m_calibrateRadius > MaxCalibrationRange)
			{
				REL_MESSAGE("Loot range calibration complete");
				SearchTask::ToggleCalibration(false);
			}
		}
		return;
	}

	// Retrieve these settings only once
	m_crimeCheck = static_cast<int>(m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config,
		shse::PlayerState::Instance().IsSneaking() ? "crimeCheckSneaking" : "crimeCheckNotSneaking"));
	m_belongingsCheck = SpecialObjectHandlingFromIniSetting(m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "playerBelongingsLoot"));

	// Stress tested using Jorrvaskr with personal property looting turned on. It's more important to loot in an orderly fashion than to get it all into inventory on
	// one pass.
	// Process any queued dead body that is dead long enough to have played kill animation. We do this first to avoid being queued up behind new info for ever
	DistanceToTarget targets;
	shse::ActorTracker::Instance().ReleaseIfReliablyDead(targets);
	double radius(LocationTracker::Instance().IsPlayerIndoors() ?
		m_ini->GetIndoorsRadius(INIFile::PrimaryType::harvest) : m_ini->GetRadius(INIFile::PrimaryType::harvest));
	AbsoluteRange rangeCheck(RE::PlayerCharacter::GetSingleton(), radius, m_ini->GetVerticalFactor());
	bool respectDoors(m_ini->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "DoorsPreventLooting") != 0.);
	ReferenceFilter filter(targets, rangeCheck, respectDoors, MaxREFRSPerPass);
	// this adds eligible REFRs ordered by distance from player
	filter.FindLootableReferences();

	// Prevent double dipping of ash pile creatures: we may loot the dying creature and then its ash pile on the same pass.
	// This seems no harm apart but offends my aesthetic sensibilities, so prevent it.
	std::vector<RE::TESObjectREFR*> possibleDupes;
	for (auto target : targets)
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
		RE::TESObjectREFR* refr(target.second);
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
			if (refr->GetFormType() == RE::FormType::ActorCharacter)
			{
				if (!refr->IsDead(true) ||
					DeadBodyLootingFromIniSetting(m_ini->GetSetting(
						INIFile::PrimaryType::common, INIFile::SecondaryType::config, "EnableLootDeadbody")) == DeadBodyLooting::DoNotLoot)
					continue;

				RE::Actor* actor(refr->As<RE::Actor>());
				if (actor)
				{
					ActorHelper actorEx(actor);
					if (actorEx.IsPlayerAlly() || actorEx.IsEssential() || actorEx.IsSummoned())
					{
						DBG_VMESSAGE("Block ineligible Actor 0x%08x, base = %s/0x%08x", refr->GetFormID(),
							refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID());
						data->BlockReference(refr);
						continue;
					}
				}

				lootTargetType = INIFile::SecondaryType::deadbodies;
				// Delay looting exactly once. We only return here after required time since death has expired.
				// Only delay if the REFR represents an entity seen alive in this cell visit. The long-dead are fair game.
				if (shse::ActorTracker::Instance().SeenAlive(refr) && !HasDynamicData(refr) && !IsLootedContainer(refr))
				{
					// Use async looting to allow game to settle actor state and animate their untimely demise
					RegisterActorTimeOfDeath(refr);
					continue;
				}
				// avoid double dipping for immediate-loot case
				if (std::find(possibleDupes.cbegin(), possibleDupes.cend(), refr) != possibleDupes.cend())
				{
					DBG_MESSAGE("Skip immediate-loot deadbody, already looted on this pass 0x%08x, base = %s/0x%08x", refr->GetFormID(),
						refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID());
					continue;
				}
				possibleDupes.push_back(refr);
			}
			else if (refr->GetBaseObject()->As<RE::TESContainer>())
			{
				if (m_ini->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "EnableLootContainer") == 0.0)
					continue;
				lootTargetType = INIFile::SecondaryType::containers;
			}
			else if (refr->GetBaseObject()->As<RE::TESObjectACTI>() && HasAshPile(refr))
			{
				DeadBodyLooting lootBodies(DeadBodyLootingFromIniSetting(
					m_ini->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "EnableLootDeadbody")));
				if (lootBodies == DeadBodyLooting::DoNotLoot)
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
				refr = GetAshPile(refr);
				DBG_MESSAGE("Got ash-pile REFR 0x%08x from REFR 0x%08x", refr->GetFormID(), target.second->GetFormID());

				// avoid double dipping for immediate-loot case
				if (std::find(possibleDupes.cbegin(), possibleDupes.cend(), refr) != possibleDupes.cend())
				{
					DBG_MESSAGE("Skip ash-pile, already looted on this pass 0x%08x, base = %s/0x%08x", refr->GetFormID(),
						refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID());
					continue;
				}
				possibleDupes.push_back(refr);
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
	UIState::Instance().Reset();

	// Do not scan again until we are in sync with the scripts
	m_pluginSynced = false;
}

void SearchTask::AfterReload()
{
	// force recheck Perks and reset carry weight
	static const bool force(true);
	shse::PlayerState::Instance().CheckPerks(force);

	// reset carry weight and menu-active state
	static const bool reloaded(true);
	shse::PlayerState::Instance().ResetCarryWeight(reloaded);
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
	return m_HarvestLock.contains(refr);
}

size_t SearchTask::PendingHarvestNotifications()
{
	RecursiveLockGuard guard(m_lock);
	return m_pendingNotifies;
}

bool SearchTask::m_pluginSynced(false);

// this is the last function called by the scripts when re-syncing state
void SearchTask::SyncDone(const bool reload)
{
	RecursiveLockGuard guard(m_lock);

	// reset blocked lists to allow recheck vs current state
	ResetRestrictions(reload);
	REL_MESSAGE("Restrictions reset, new/loaded game = %s", reload ? "true" : "false");

	// need to wait for the scripts to sync up before performing player house checks
	m_pluginSynced = true;
}

// this triggers/stops loot range calibration cycle
bool SearchTask::m_calibrating(false);
int SearchTask::m_calibrateRadius(SearchTask::CalibrationRangeDelta);
int SearchTask::m_calibrateDelta(SearchTask::CalibrationRangeDelta);
bool SearchTask::m_glowDemo(false);
GlowReason SearchTask::m_nextGlow(GlowReason::SimpleTarget);

void SearchTask::ToggleCalibration(const bool glowDemo)
{
	RecursiveLockGuard guard(m_lock);
	m_calibrating = !m_calibrating;
	REL_MESSAGE("Calibration of Looting range %s, test shaders %s",	m_calibrating ? "started" : "stopped", m_glowDemo ? "true" : "false");
	if (m_calibrating)
	{
		m_glowDemo = glowDemo;
		m_calibrateDelta = m_glowDemo ? GlowDemoRange : CalibrationRangeDelta;
		m_calibrateRadius = m_glowDemo ? GlowDemoRange : CalibrationRangeDelta;
		m_nextGlow = GlowReason::SimpleTarget;
	}
	else
	{
		if (m_glowDemo)
		{
			std::string glowText("Glow demo stopped");
			RE::DebugNotification(glowText.c_str());
		}
		else
		{
			std::string rangeText("Range Calibration stopped");
			RE::DebugNotification(rangeText.c_str());
		}
		m_glowDemo = false;
	}
}

bool SearchTask::Load()
{
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Startup: Load Game Data");
#endif
#if _DEBUG
	VersionDb db;

	// Try to load database of version 1.5.97.0 regardless of running executable version.
	if (!db.Load(1, 5, 97, 0))
	{
		DBG_FATALERROR("Failed to load database for 1.5.97.0!");
		return false;
	}

	// Write out a file called offsets-1.5.97.0.txt where each line is the ID and offset.
	db.Dump("offsets-1.5.97.0.txt");
	DBG_MESSAGE("Dumped offsets for 1.5.97.0");
#endif
	if (!shse::LoadOrder::Instance().Analyze())
	{
		REL_FATALERROR("Load Order unsupportable");
		return false;
	}
	DataCase::GetInstance()->CategorizeLootables();
	PopulationCenters::Instance().Categorize();

	// Collections are layered on top of categorized objects
	REL_MESSAGE("*** LOAD *** Build Collections");
	shse::CollectionManager::Instance().ProcessDefinitions();

	m_pluginOK = true;
	REL_MESSAGE("Plugin now in sync - Game Data load complete!");
	return true;
}

bool SearchTask::Init()
{
    if (!m_pluginOK)
	{
		__try
		{
			// Use structured exception handling during game data load
			REL_MESSAGE("Plugin not synced up - Game Data load executing");
			if (!Load())
				return false;
			}
		__except (LogStackWalker::LogStack(GetExceptionInformation()))
		{
			REL_FATALERROR("Fatal Exception during Game Data load");
			return false;
		}
	}
	return true;
}

}