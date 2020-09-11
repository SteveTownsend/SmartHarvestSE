/*************************************************************************
SmartHarvest SE
Copyright (c) Steve Townsend 2020

>>> SOURCE LICENSE >>>
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation (www.fsf.org); either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License is available at
http://www.fsf.org/licensing/licenses
>>> END OF LICENSE >>>
*************************************************************************/
#include "PrecompiledHeaders.h"

#include "Data/dataCase.h"
#include "Data/LoadOrder.h"
#include "Looting/TryLootREFR.h"
#include "Looting/ScanGovernor.h"
#include "Utilities/debugs.h"
#include "Utilities/utils.h"
#include "WorldState/ActorTracker.h"
#include "WorldState/LocationTracker.h"
#include "Looting/ManagedLists.h"
#include "Looting/objects.h"
#include "Looting/LootableREFR.h"
#include "WorldState/PopulationCenters.h"
#include "FormHelpers/FormHelper.h"
#include "Looting/NPCFilter.h"
#include "Looting/ReferenceFilter.h"
#include "WorldState/PlayerHouses.h"
#include "WorldState/PlayerState.h"
#include "Looting/ProducerLootables.h"
#include "Looting/TheftCoordinator.h"
#include "Utilities/LogStackWalker.h"
#include "Collections/CollectionManager.h"
#include "VM/EventPublisher.h"
#include "VM/papyrus.h"

#include <chrono>
#include <thread>

namespace shse
{

std::unique_ptr<ScanGovernor> ScanGovernor::m_instance;

ScanGovernor& ScanGovernor::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<ScanGovernor>();
	}
	return *m_instance;
}

ScanGovernor::ScanGovernor() : m_searchAllowed(false), m_pendingNotifies(0), m_calibrating(false), m_calibrateRadius(CalibrationRangeDelta),
	m_calibrateDelta(ScanGovernor::CalibrationRangeDelta), m_glowDemo(false), m_nextGlow(GlowReason::SimpleTarget),
	m_targetType(INIFile::SecondaryType::NONE2), m_spergInProgress(0)
{
}

// Dynamic REFR looting is not delayed - the visuals may be less appealing, but delaying risks CTD as REFRs can
// be recycled very quickly.
bool ScanGovernor::HasDynamicData(RE::TESObjectREFR* refr) const
{
	// do not reregister known REFR
	if (LootedDynamicREFRFormID(refr) != InvalidForm)
		return true;

	// risk exists if REFR or its concrete object is dynamic
	if (refr->IsDynamicForm() || refr->GetBaseObject()->IsDynamicForm())
	{
		DBG_VMESSAGE("dynamic REFR 0x{:08x} or base 0x{:08x} for {}", refr->GetFormID(),
			refr->GetBaseObject()->GetFormID(), refr->GetBaseObject()->GetName());
		// record looting so we don't rescan
		MarkDynamicREFRLooted(refr);
		return true;
	}
	return false;
}

void ScanGovernor::MarkDynamicREFRLooted(const RE::TESObjectREFR* refr) const
{
	RecursiveLockGuard guard(m_searchLock);
	// record looting so we don't rescan
	m_lootedDynamicREFRs.insert({ refr->GetFormID(), refr->GetBaseObject()->GetFormID()});
}

RE::FormID ScanGovernor::LootedDynamicREFRFormID(const RE::TESObjectREFR* refr) const
{
	if (!refr)
		return false;
	RecursiveLockGuard guard(m_searchLock);
	const auto looted(m_lootedDynamicREFRs.find({ refr->GetFormID(), refr->GetBaseObject()->GetFormID() }));
	return looted != m_lootedDynamicREFRs.cend() ? looted->first : InvalidForm;
}

// forget about dynamic containers we looted when cell changes. This is more aggressive than static container looting
// as this list contains recycled FormIDs, and hypothetically may grow unbounded.
void ScanGovernor::ResetLootedDynamicREFRs()
{
	RecursiveLockGuard guard(m_searchLock);
	m_lootedDynamicREFRs.clear();
}

void ScanGovernor::MarkContainerLootedRepeatGlow(const RE::TESObjectREFR* refr, const int glowDuration)
{
	RecursiveLockGuard guard(m_searchLock);
	// record looting so we don't rescan - glow may prevent looting and require repeat processing after the glow wears off
	std::chrono::steady_clock::time_point expiry;
	if (glowDuration > 0)
	{
		auto currentTime(std::chrono::high_resolution_clock::now());
		expiry = currentTime + std::chrono::milliseconds(static_cast<long long>(glowDuration * 1000.0));
	}
	// overwrite existing to stop repeated glow if container no longer merits it
	m_lootedContainers[refr] = expiry;
	// this may be a locked container that we manually emptied, if so we should stop it glowing
	m_lockedContainers.erase(refr);
}

void ScanGovernor::MarkContainerLooted(const RE::TESObjectREFR* refr)
{
	MarkContainerLootedRepeatGlow(refr, 0);
}

bool ScanGovernor::IsLootedContainer(const RE::TESObjectREFR* refr) const
{
	if (!refr)
		return false;
	RecursiveLockGuard guard(m_searchLock);
	const auto looted(m_lootedContainers.find(refr));
	if (looted != m_lootedContainers.cend())
	{
		const auto glowExpiry(looted->second);
		return glowExpiry == std::chrono::steady_clock::time_point() || glowExpiry > std::chrono::high_resolution_clock::now();
	}
	else
	{
		return false;
	}
}

// forget about containers we looted to allow rescan after game load or config settings update
void ScanGovernor::ResetLootedContainers()
{
	RecursiveLockGuard guard(m_searchLock);
	m_lootedContainers.clear();
}

// Remember locked containers so we do not auto-loot after player unlock, if config forbids
bool ScanGovernor::IsReferenceLockedContainer(const RE::TESObjectREFR* refr) const
{
	if (!refr)
		return false;
	RecursiveLockGuard guard(m_searchLock);
	// check instantaneous locked/unlocked state of the container
	if (!IsLocked(refr))
	{
		// If container is not locked, but previously was stored as locked, continue to treat as unlocked until item count changes.
		// For locked container, we want the player to have the enjoyment of manually looting after unlocking. If they don't
		// want this, they should configure 'Loot locked container'.
		// Such a container will no longer glow locked after player unlocks and loots it.
		auto locked(m_lockedContainers.find(refr));
		if (locked != m_lockedContainers.end())
		{
			// if item count has changed, remove from locked container list: manually looted, we assume
			size_t items(ContainerLister(INIFile::SecondaryType::containers, refr).CountLootableItems(
				[=](RE::TESBoundObject* item) -> bool { return true; }));
			if (items != locked->second)
			{
				DBG_VMESSAGE("Forget REFR 0x{:08x} to locked container {}/0x{:08x} with {} items, was {}", refr->GetFormID(),
					refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID(), items, locked->second);
				m_lockedContainers.erase(locked);
				return false;
			}
			// item count unchanged - continue to glow
			DBG_VMESSAGE("REFR 0x{:08x} to previously locked container {}/0x{:08x} still has {} items", refr->GetFormID(),
				refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID(), items);
			return true;
		}
		// vanilla unlocked container
		DBG_VMESSAGE("REFR 0x{:08x} was never a locked container {}/0x{:08x}", refr->GetFormID(),
			refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID());
		return false;
	}
	// container is locked - save if not already known
	else if (!m_lockedContainers.contains(refr))
	{
		size_t items(ContainerLister(INIFile::SecondaryType::containers, refr).CountLootableItems(
			[=](RE::TESBoundObject* item) -> bool { return true; }));
		DBG_VMESSAGE("Remember REFR 0x{:08x} to locked container {}/0x{:08x} with {} items", refr->GetFormID(),
			refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID(), items);
		m_lockedContainers.insert({ refr, items });
	}
	return true;
}

void ScanGovernor::ForgetLockedContainers()
{
	DBG_MESSAGE("Clear locked containers blacklist");
	RecursiveLockGuard guard(m_searchLock);
	m_lockedContainers.clear();
}

void ScanGovernor::RegisterActorTimeOfDeath(RE::TESObjectREFR* refr)
{
	shse::ActorTracker::Instance().RecordTimeOfDeath(refr);
	// block REFR so we don't include in future scans
	DataCase::GetInstance()->BlockReference(refr, Lootability::DeadBodyDelayedLooting);
}

void ScanGovernor::ProgressGlowDemo()
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
		DBG_VMESSAGE("Trigger glow for {}/0x{:08x} at distance {:0.2f} units", target.second->GetName(), target.second->formID, target.first);
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
			ToggleCalibration(false);
		}
	}
}

// input may get updated for ashpile
Lootability ScanGovernor::ValidateTarget(RE::TESObjectREFR*& refr, std::vector<RE::TESObjectREFR*>& possibleDupes, const bool dryRun, const bool glowOnly)
{
	if (!refr)
		return Lootability::NullReference;
	if (refr->GetFormID() == InvalidForm)
	{
		if (!dryRun)
		{
			DBG_WARNING("REFR has invalid FormID");
			DataCase::GetInstance()->BlacklistReference(refr);
		}
		return Lootability::InvalidFormID;
	}
	else if (!refr->GetBaseObject())
	{
		if (!dryRun)
		{
			DBG_WARNING("REFR 0x{:08x} has no Base Object", refr->GetFormID());
			DataCase::GetInstance()->BlacklistReference(refr);
		}
		return Lootability::NoBaseObject;
	}
	else
	{
		m_targetType = INIFile::SecondaryType::itemObjects;
		DBG_VMESSAGE("Process REFR 0x{:08x} with base object {}/0x{:08x}", refr->GetFormID(),
			refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID());
		if (refr->GetFormType() == RE::FormType::ActorCharacter)
		{
			if (!refr->IsDead(true))
			{
				return Lootability::ReferenceIsLiveActor;
			}
			if (DeadBodyLootingFromIniSetting(INIFile::GetInstance()->GetSetting(
					INIFile::PrimaryType::common, INIFile::SecondaryType::config, "EnableLootDeadbody")) == DeadBodyLooting::DoNotLoot)
			{
				return Lootability::LootDeadBodyDisabled;
			}
			// REFR to Dead NPC may be blacklisted by user
			if (ManagedList::BlackList().Contains(refr))
			{
				return Lootability::DeadBodyBlacklistedByUser;
			}

			RE::Actor* actor(refr->As<RE::Actor>());
			if (actor)
			{
				Lootability exclusionType(Lootability::Lootable);
				PlayerAffinity playerAffinity(GetPlayerAffinity(actor));
				if (playerAffinity != PlayerAffinity::Unaffiliated && playerAffinity != PlayerAffinity::Player)
				{
					exclusionType = Lootability::DeadBodyIsPlayerAlly;
				}
				else if (actor->IsEssential())
				{
					exclusionType = Lootability::DeadBodyIsEssential;
				}
				else if (IsSummoned(actor))
				{
					exclusionType = Lootability::DeadBodyIsSummoned;
				}
				else if (IsQuestTargetNPC(actor))
				{
					exclusionType = Lootability::CannotLootQuestTarget;
				}
				else if (!NPCFilter::Instance().IsLootable(actor->GetActorBase()))
				{
					exclusionType = Lootability::NPCExcludedByDeadBodyFilter;
				}
				if (exclusionType != Lootability::Lootable)
				{
					if (!dryRun)
					{
						DBG_VMESSAGE("Block ineligible Actor 0x{:08x}, base = {}/0x{:08x}", refr->GetFormID(),
							refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID());
						DataCase::GetInstance()->BlockReference(refr, exclusionType);
					}
					return exclusionType;
				}
			}

			m_targetType = INIFile::SecondaryType::deadbodies;
			// Delay looting exactly once. We only return here after required time since death has expired.
			// Only delay if the REFR represents an entity seen alive in this cell visit. The long-dead are fair game.
			if (shse::ActorTracker::Instance().SeenAlive(refr) && !HasDynamicData(refr) &&
				DataCase::GetInstance()->IsReferenceBlocked(refr) == Lootability::Lootable)
			{
				if (!dryRun && !glowOnly)
				{
					// Use async looting to allow game to settle actor state and animate their untimely demise
					RegisterActorTimeOfDeath(refr);
				}
				return Lootability::DeadBodyDelayedLooting;
			}
			// avoid double dipping for immediate-loot case
			if (std::find(possibleDupes.cbegin(), possibleDupes.cend(), refr) != possibleDupes.cend())
			{
				DBG_MESSAGE("Skip immediate-loot deadbody, already looted on this pass 0x{:08x}, base = {}/0x{:08x}", refr->GetFormID(),
					refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID());
				return Lootability::DeadBodyPossibleDuplicate;
			}
			// record Killer of dynamic REFR that we will loot immediately
			ActorTracker::Instance().RecordIfKilledByParty(actor);
			possibleDupes.push_back(refr);
		}
		else if (refr->GetBaseObject()->As<RE::TESContainer>())
		{
			if (INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "EnableLootContainer") == 0.0)
			{
				return Lootability::LootContainersDisabled;
			}
			// REFR to container may be blacklisted by user
			if (ManagedList::BlackList().Contains(refr))
			{
				return Lootability::ContainerBlacklistedByUser;
			}
			m_targetType = INIFile::SecondaryType::containers;
		}
		else if (refr->GetBaseObject()->As<RE::TESObjectACTI>() && HasAshPile(refr))
		{
			DeadBodyLooting lootBodies(DeadBodyLootingFromIniSetting(
				INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "EnableLootDeadbody")));
			if (lootBodies == DeadBodyLooting::DoNotLoot)
			{
				return Lootability::LootDeadBodyDisabled;
			}
			m_targetType = INIFile::SecondaryType::deadbodies;
			// Delay looting exactly once. We only return here after required time since death has expired.
			if (!HasDynamicData(refr) && DataCase::GetInstance()->IsReferenceBlocked(refr) == Lootability::Lootable)
			{
				if (!dryRun && !glowOnly)
				{
					// Use async looting to allow game to settle actor state and animate their untimely demise
					RegisterActorTimeOfDeath(refr);
				}
				return Lootability::DeadBodyDelayedLooting;
			}
			// deferred looting of dead bodies - introspect ExtraDataList to get the REFR
			RE::TESObjectREFR* original(refr);
			refr = GetAshPile(refr);
			if (!refr)
			{
				return Lootability::CannotGetAshPile;
			}
			DBG_MESSAGE("Got ash-pile REFR 0x{:08x} from REFR 0x{:08x}", refr->GetFormID(), original->GetFormID());

			// avoid double dipping for immediate-loot case
			if (std::find(possibleDupes.cbegin(), possibleDupes.cend(), refr) != possibleDupes.cend())
			{
				DBG_MESSAGE("Skip ash-pile, already looted on this pass 0x{:08x}, base = {}/0x{:08x}", refr->GetFormID(),
					refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID());
				return Lootability::DeadBodyPossibleDuplicate;
			}
			possibleDupes.push_back(refr);
		}
		else if (INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "enableHarvest") == 0.0)
		{
			return Lootability::HarvestLooseItemDisabled;
		}
		return Lootability::Lootable;
	}
}

void ScanGovernor::LootAllEligible()
{
	// Stress tested using Jorrvaskr with personal property looting turned on. It's more important to loot in an orderly fashion than to get it all into inventory on
	// one pass.
	// Process any queued dead body that is dead long enough to have played kill animation. We do this first to avoid being queued up behind new info for ever
	DistanceToTarget targets;
	shse::ActorTracker::Instance().ReleaseIfReliablyDead(targets);
	double radius(LocationTracker::Instance().IsPlayerIndoors() ?
		INIFile::GetInstance()->GetIndoorsRadius(INIFile::PrimaryType::harvest) : INIFile::GetInstance()->GetRadius(INIFile::PrimaryType::harvest));
	AbsoluteRange rangeCheck(RE::PlayerCharacter::GetSingleton(), radius, INIFile::GetInstance()->GetVerticalFactor());
	bool respectDoors(INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "DoorsPreventLooting") != 0.);
	ReferenceFilter filter(targets, rangeCheck, respectDoors, MaxREFRSPerPass);
	// this adds eligible REFRs ordered by distance from player
	filter.FindLootableReferences();

	// Prevent double dipping of ash pile creatures: we may loot the dying creature and then its ash pile on the same pass.
	// This seems no harm apart but offends my aesthetic sensibilities, so prevent it.
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Loot Eligible Targets");
#endif
	std::unordered_map<RE::TESForm*, Lootability> checkedTargets;
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
		static const bool dryRun(false);
		static const bool glowOnly(false);
		// Scan radius often includes repeated mundane objects e.g. loose septims, several plates. Optimize for that case here.
		Lootability lootability(Lootability::Lootable);
		const auto checkedTarget(checkedTargets.find(refr ? refr->GetBaseObject() : nullptr));
		if (checkedTarget != checkedTargets.cend())
		{
			m_targetType = INIFile::SecondaryType::itemObjects;
			lootability = checkedTarget->second;
			DBG_VMESSAGE("0x{:08x}, base {}/0x{:08x} already checked: {}", refr ? refr->GetFormID() : InvalidForm,
				(refr && refr->GetBaseObject() ? refr->GetBaseObject()->GetName() : ""),
				(refr && refr->GetBaseObject() ? refr->GetBaseObject()->GetFormID() : InvalidForm), LootabilityName(lootability));
		}
		else
		{
			lootability = ValidateTarget(refr, possibleDupes, dryRun, glowOnly);
			if (refr->GetFormType() != RE::FormType::ActorCharacter && !refr->GetContainer())
			{
				// different Actors and Chests have different loot
				checkedTargets.insert({ refr ? refr->GetBaseObject() : nullptr, lootability });
			}
		}
		if (lootability	!= Lootability::Lootable)
		{
			continue;
		}
		static const bool stolen(false);
		TryLootREFR(refr, m_targetType, stolen, glowOnly).Process(dryRun);
	}
}

void ScanGovernor::TrackActors()
{
	DistanceToTarget targets;
	AlwaysInRange rangeCheck;
	ReferenceFilter(targets, rangeCheck, false, MaxREFRSPerPass).FindActors();
}

const RE::Actor* ScanGovernor::ActorByIndex(const int actorIndex) const
{
	RecursiveLockGuard guard(m_searchLock);
	if (actorIndex < m_detectiveWannabes.size())
		return m_detectiveWannabes[actorIndex];
	return nullptr;
}

void ScanGovernor::DoPeriodicSearch(const ReferenceScanType scanType)
{
	bool sneaking(false);
	if (scanType == ReferenceScanType::Calibration)
	{
		ProgressGlowDemo();
	}
	else if (scanType == ReferenceScanType::Loot)
	{
		LootAllEligible();

		// after checking all REFRs, trigger async undetected-theft
		TheftCoordinator::Instance().StealIfUndetected();
	}
	else
	{
		// if not looting, run a more limited scan
		TrackActors();
	}

	// Refresh player party of followers
	PartyMembers::Instance().AdjustParty(ActorTracker::Instance().GetFollowers(), PlayerState::Instance().CurrentGameTime());
	// request added items to be pushed to us while we are sleeping - including items not auto-looted
	CollectionManager::Instance().Refresh();
}

// Glow-only, for Immersion enthusiasts
void ScanGovernor::InvokeLootSense(void)
{
	// Stress tested using Jorrvaskr with personal property looting turned on. It's more important to glow in an orderly fashion than to do it all on one pass.
	DistanceToTarget targets;
	double radius(LocationTracker::Instance().IsPlayerIndoors() ?
		INIFile::GetInstance()->GetIndoorsRadius(INIFile::PrimaryType::harvest) : INIFile::GetInstance()->GetRadius(INIFile::PrimaryType::harvest));
	AbsoluteRange rangeCheck(RE::PlayerCharacter::GetSingleton(), radius, INIFile::GetInstance()->GetVerticalFactor());
	bool respectDoors(INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "DoorsPreventLooting") != 0.);
	ReferenceFilter filter(targets, rangeCheck, respectDoors, MaxREFRSPerPass);
	// this adds eligible REFRs ordered by distance from player
	filter.FindLootableReferences();

	// Prevent double dipping of ash pile creatures: we may loot the dying creature and then its ash pile on the same pass.
	// This seems no harm apart but offends my aesthetic sensibilities, so prevent it.
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Glow Eligible Targets");
#endif
	std::unordered_map<RE::TESForm*, Lootability> checkedTargets;
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
		static const bool dryRun(false);
		static const bool glowOnly(true);
		// Scan radius often includes repeated mundane objects e.g. loose septims, several plates. Optimize for that case here.
		Lootability lootability(Lootability::Lootable);
		const auto checkedTarget(checkedTargets.find(refr ? refr->GetBaseObject() : nullptr));
		if (checkedTarget != checkedTargets.cend())
		{
			m_targetType = INIFile::SecondaryType::itemObjects;
			lootability = checkedTarget->second;
			DBG_VMESSAGE("0x{:08x}, base {}/0x{:08x} already checked: {}", refr ? refr->GetFormID() : InvalidForm,
				(refr && refr->GetBaseObject() ? refr->GetBaseObject()->GetName() : ""),
				(refr && refr->GetBaseObject() ? refr->GetBaseObject()->GetFormID() : InvalidForm), LootabilityName(lootability));
		}
		else
		{
			lootability = ValidateTarget(refr, possibleDupes, dryRun, glowOnly);
			if (refr->GetFormType() != RE::FormType::ActorCharacter && !refr->GetContainer())
			{
				// different Actors and Chests have different loot
				checkedTargets.insert({ refr ? refr->GetBaseObject() : nullptr, lootability });
			}
		}
		if (lootability != Lootability::Lootable)
		{
			continue;
		}
		static const bool stolen(false);
		TryLootREFR(refr, m_targetType, stolen, glowOnly).Process(dryRun);
	}
}

void ScanGovernor::DisplayLootability(RE::TESObjectREFR* refr)
{
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Check Lootability", refr);
#endif
	Lootability result(ReferenceFilter::CheckLootable(refr));
	static const bool dryRun(true);
	static const bool glowOnly(false);
	std::string typeName;
	if (result == Lootability::Lootable)
	{
		std::vector<RE::TESObjectREFR*> possibleDupes;
		result = ValidateTarget(refr, possibleDupes, dryRun, glowOnly);
	}
	if (result == Lootability::Lootable)
	{
		// flag to prevent mutation of state when just checking the rules
		TryLootREFR runner(refr, m_targetType, false, glowOnly);
		result = runner.Process(dryRun);
		typeName = runner.ObjectTypeName();
	}

	// check player detection state if relevant
	if (PlayerState::Instance().EffectiveOwnershipRule() == OwnershipRule::AllowCrimeIfUndetected)
	{
		m_detectiveWannabes = ActorTracker::Instance().GetDetectives();
		DBG_VMESSAGE("Detection check to steal under the nose of {} Actors", m_detectiveWannabes.size());
		static const bool dryRun(true);
		EventPublisher::Instance().TriggerStealIfUndetected(m_detectiveWannabes.size(), dryRun);
	}

	std::ostringstream resultStr;
	resultStr << "REFR 0x" << StringUtils::FromFormID(refr ? refr->GetFormID() : InvalidForm);
	const auto baseObject(refr ? refr->GetBaseObject() : nullptr);
	if (baseObject)
	{
		resultStr << " -> " << baseObject->GetName() << "/0x" << StringUtils::FromFormID(baseObject->GetFormID());
	}
	if (!typeName.empty())
	{
		resultStr << " type=" << typeName;
	}
	std::string message(resultStr.str());
	RE::DebugNotification(message.c_str());
	REL_MESSAGE("Lootability checked for {}", message.c_str());
	resultStr.str("");

	resultStr << LootabilityName(result) << ' ' << LocationTracker::Instance().PlayerExactLocation();
	message = resultStr.str();
	RE::DebugNotification(message.c_str());
	REL_MESSAGE("Lootability result: {}", message.c_str());
}

void ScanGovernor::Allow()
{
	RecursiveLockGuard guard(m_searchLock);
	m_searchAllowed = true;
}

void ScanGovernor::Disallow()
{
	RecursiveLockGuard guard(m_searchLock);
	m_searchAllowed = false;
}
bool ScanGovernor::IsAllowed() const
{
	RecursiveLockGuard guard(m_searchLock);
	return m_searchAllowed;
}

bool ScanGovernor::LockHarvest(const RE::TESObjectREFR* refr, const bool isSilent)
{
	RecursiveLockGuard guard(m_searchLock);
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

bool ScanGovernor::UnlockHarvest(const RE::TESObjectREFR* refr, const bool isSilent)
{
	RecursiveLockGuard guard(m_searchLock);
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

void ScanGovernor::Clear()
{
	RecursiveLockGuard guard(m_searchLock);
	// unblock all blocked auto-harvest objects
	ClearPendingHarvestNotifications();
	// Dynamic containers that we looted reset on cell change
	ResetLootedDynamicREFRs();
	// clean up the list of glowing objects, don't futz with EffectShader since cannot run scripts at this time
	ClearGlowExpiration();

	// clear lists of looted and locked containers
	ResetLootedContainers();
	ForgetLockedContainers();
}

bool ScanGovernor::IsLockedForHarvest(const RE::TESObjectREFR* refr) const
{
	RecursiveLockGuard guard(m_searchLock);
	return m_HarvestLock.contains(refr);
}

size_t ScanGovernor::PendingHarvestNotifications() const
{
	RecursiveLockGuard guard(m_searchLock);
	return m_pendingNotifies;
}

void ScanGovernor::ClearPendingHarvestNotifications()
{
	RecursiveLockGuard guard(m_searchLock);
	return m_HarvestLock.clear();
}

void ScanGovernor::ClearGlowExpiration()
{
	RecursiveLockGuard guard(m_searchLock);
	return m_glowExpiration.clear();
}

// SPERG doubles mined item amounts based on KYWD values. Store those items beforehand and recheck afterwards, adjusting counts for Player.
void ScanGovernor::SetSPERGKeyword(const RE::BGSKeyword* keyword)
{
	m_spergKeywords.push_back(keyword);
}

// This is called from script but does not mutate anything in the game data.
// Saves inventory items applicable for SPERG for later reconciliation.
void ScanGovernor::SPERGMiningStart(void)
{
	RE::PlayerCharacter* player(RE::PlayerCharacter::GetSingleton());
	if (!player)
		return;

	RecursiveLockGuard guard(m_searchLock);
	++m_spergInProgress;
	if (m_spergInventory)
	{
		REL_MESSAGE("Pre-SPERG inventory snapshot already captured, {} in progress", m_spergInProgress);
		return;
	}
	m_spergInventory = std::make_unique<ContainerLister>(INIFile::SecondaryType::deadbodies, player);
	m_spergInventory->FilterLootableItems([&](RE::TESBoundObject* item) -> bool
	{
		const RE::BGSKeywordForm* keywordHolder(item->As<RE::BGSKeywordForm>());
		if (keywordHolder)
		{
			for (const auto keyword : m_spergKeywords)
			{
				if (keywordHolder->HasKeyword(keyword))
					return true;
			}
		}
		return false;
	});
	DBG_DMESSAGE("SPERG KYWD matching items {}", m_spergInventory->GetLootableItems().size());
}

void ScanGovernor::SPERGMiningEnd(void)
{
	RecursiveLockGuard guard(m_searchLock);
	if (m_spergInProgress > 0)
	{
		--m_spergInProgress;
		if (m_spergInProgress > 0)
		{
			REL_MESSAGE("SPERG completion, {} still in progress", m_spergInProgress);
		}
	}
	else
	{
		REL_WARNING("SPERG completion, no operations in progress");
	}
}

// This runs in scan thread whenever there are no mining operations in progress, to avoid problems with player inventory contention
void ScanGovernor::ReconcileSPERGMined(void)
{
	RecursiveLockGuard guard(m_searchLock);
	if (m_spergInProgress > 0)
	{
		REL_MESSAGE("Skip SPERG reconciliation, {} in progress", m_spergInProgress);
		return;
	}
	if (!m_spergInventory)
	{
		DBG_DMESSAGE("No SPERG mining operations since last pass");
		return;
	}
	RE::PlayerCharacter* player(RE::PlayerCharacter::GetSingleton());
	if (!player)
		return;

	// get current inventory and duplicate any SPERG items added since snapshot was taken
	ContainerLister lister(INIFile::SecondaryType::deadbodies, player);
	lister.FilterLootableItems([&](RE::TESBoundObject* item) -> bool
	{
		const RE::BGSKeywordForm* keywordHolder(item->As<RE::BGSKeywordForm>());
		if (keywordHolder)
		{
			for (const auto keyword : m_spergKeywords)
			{
				if (keywordHolder->HasKeyword(keyword))
					return true;
			}
		}
		return false;
	});
	const LootableItems& newInventory(lister.GetLootableItems());
	for (const auto& newItem : newInventory)
	{
		int32_t delta(0);
		if (std::find_if(m_spergInventory->GetLootableItems().cbegin(), m_spergInventory->GetLootableItems().cend(),
			[&](const InventoryItem& item) -> bool {
			if (item.BoundObject() == newItem.BoundObject())
			{
				delta = static_cast<int32_t>(std::max(int(newItem.Count()) - int(item.Count()), 0));
				return true;
			}
			return false;
		}) == m_spergInventory->GetLootableItems().cend())
		{
			delta = static_cast<int32_t>(newItem.Count());
		}
		if (delta > 0)
		{
			DBG_MESSAGE("SPERG mined {} extra of {}/0x{:08x}", delta, newItem.BoundObject()->GetName(), newItem.BoundObject()->GetFormID());
			player->AddObjectToContainer(newItem.BoundObject(), nullptr, delta, nullptr);
		}
	}
	// resets the cycle
	m_spergInventory.reset();
}

// this triggers/stops loot range calibration cycle
void ScanGovernor::ToggleCalibration(const bool glowDemo)
{
	RecursiveLockGuard guard(m_searchLock);
	m_calibrating = !m_calibrating;
	REL_MESSAGE("Calibration of Looting range {}, test shaders {}",	m_calibrating ? "started" : "stopped", m_glowDemo ? "true" : "false");
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

void ScanGovernor::GlowObject(RE::TESObjectREFR* refr, const int duration, const GlowReason glowReason)
{
	// only send the glow event once per N seconds. This will retrigger on later passes, but once we are out of
	// range no more glowing will be triggered. The item remains in the list until we change cell but there should
	// never be so many in a cell that this is a problem.
	RecursiveLockGuard guard(m_searchLock);
	const auto existingGlow(m_glowExpiration.find(refr));
	auto currentTime(std::chrono::high_resolution_clock::now());
	if (existingGlow != m_glowExpiration.cend() && existingGlow->second > currentTime)
		return;
	// lower this by 500ms so that it expires before container recheck timer
	auto expiry = currentTime + std::chrono::milliseconds(static_cast<long long>(duration * 1000.0) - 500LL);
	m_glowExpiration[refr] = expiry;
	DBG_VMESSAGE("Trigger glow for {}/0x{:08x}", refr->GetName(), refr->formID);
	EventPublisher::Instance().TriggerObjectGlow(refr, duration, glowReason);
}

}
