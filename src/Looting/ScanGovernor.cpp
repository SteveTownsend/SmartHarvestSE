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

RecursiveLock ScanGovernor::m_searchLock;
std::unordered_map<const RE::TESObjectREFR*, std::chrono::time_point<std::chrono::high_resolution_clock>> ScanGovernor::m_glowExpiration;

// Dynamic REFR looting is not delayed - the visuals may be less appealing, but delaying risks CTD as REFRs can
// be recycled very quickly.
bool ScanGovernor::HasDynamicData(RE::TESObjectREFR* refr)
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

std::unordered_map<const RE::TESObjectREFR*, RE::FormID> ScanGovernor::m_lootedDynamicContainers;
void ScanGovernor::MarkDynamicContainerLooted(const RE::TESObjectREFR* refr)
{
	RecursiveLockGuard guard(m_searchLock);
	// record looting so we don't rescan
	m_lootedDynamicContainers.insert(std::make_pair(refr, refr->GetFormID()));
}

RE::FormID ScanGovernor::LootedDynamicContainerFormID(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return false;
	RecursiveLockGuard guard(m_searchLock);
	const auto looted(m_lootedDynamicContainers.find(refr));
	return looted != m_lootedDynamicContainers.cend() ? looted->second : InvalidForm;
}

// forget about dynamic containers we looted when cell changes. This is more aggressive than static container looting
// as this list contains recycled FormIDs, and hypothetically may grow unbounded.
void ScanGovernor::ResetLootedDynamicContainers()
{
	RecursiveLockGuard guard(m_searchLock);
	m_lootedDynamicContainers.clear();
}

std::unordered_set<const RE::TESObjectREFR*> ScanGovernor::m_lootedContainers;
void ScanGovernor::MarkContainerLooted(const RE::TESObjectREFR* refr)
{
	RecursiveLockGuard guard(m_searchLock);
	// record looting so we don't rescan
	m_lootedContainers.insert(refr);
}

bool ScanGovernor::IsLootedContainer(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return false;
	RecursiveLockGuard guard(m_searchLock);
	return m_lootedContainers.count(refr) > 0;
}

// forget about containers we looted to allow rescan after game load or config settings update
void ScanGovernor::ResetLootedContainers()
{
	RecursiveLockGuard guard(m_searchLock);
	m_lootedContainers.clear();
}

std::unordered_set<const RE::TESObjectREFR*> ScanGovernor::m_lockedContainers;

// Remember locked containers so we do not auto-loot after player unlock, if config forbids
bool ScanGovernor::IsReferenceLockedContainer(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return false;
	RecursiveLockGuard guard(m_searchLock);
	// check instantaneous locked/unlocked state of the container
	if (!IsLocked(refr))
	{
		// If container is not locked, but previously was stored as locked, continue to treat as unlocked until game reload.
		// For locked container, we want the player to have the enjoyment of manually looting after unlocking. If they don't
		// want this, they should configure 'Loot locked container'. Such a container will glow locked even after player
		// unlocks.
		DBG_VMESSAGE("Check REFR 0x%08x to not-locked container %s/0x%08x", refr->GetFormID(),
			refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID());
		return m_lockedContainers.find(refr) != m_lockedContainers.cend();
	}
	// container is locked - save if not already known
	else if (m_lockedContainers.insert(refr).second)
	{
		DBG_VMESSAGE("Remember REFR 0x%08x to locked container %s/0x%08x", refr->GetFormID(),
			refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID());
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
	RecursiveLockGuard guard(m_searchLock);
	// record looting so we don't rescan
	MarkContainerLooted(refr);
}

bool ScanGovernor::m_searchAllowed = false;

void ScanGovernor::DoPeriodicSearch()
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
				ScanGovernor::ToggleCalibration(false);
			}
		}
		return;
	}

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
					DeadBodyLootingFromIniSetting(INIFile::GetInstance()->GetSetting(
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
				if (INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "EnableLootContainer") == 0.0)
					continue;
				lootTargetType = INIFile::SecondaryType::containers;
			}
			else if (refr->GetBaseObject()->As<RE::TESObjectACTI>() && HasAshPile(refr))
			{
				DeadBodyLooting lootBodies(DeadBodyLootingFromIniSetting(
					INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "EnableLootDeadbody")));
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
			else if (INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "enableHarvest") == 0.0)
			{
				continue;
			}
		}
		static const bool stolen(false);
		TryLootREFR(refr, lootTargetType, stolen).Process();
	}
	// after checking all REFRs, trigger async undetected-theft
	TheftCoordinator::Instance().StealIfUndetected();
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
bool ScanGovernor::IsAllowed()
{
	RecursiveLockGuard guard(m_searchLock);
	return m_searchAllowed;
}

std::unordered_set<const RE::TESObjectREFR*> ScanGovernor::m_HarvestLock;
int ScanGovernor::m_pendingNotifies = 0;

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

void ScanGovernor::Clear(const bool gameReload)
{
	RecursiveLockGuard guard(m_searchLock);
	// unblock all blocked auto-harvest objects
	ClearPendingHarvestNotifications();
	// Dynamic containers that we looted reset on cell change
	ResetLootedDynamicContainers();
	// clean up the list of glowing objects, don't futz with EffectShader since cannot run scripts at this time
	ClearGlowExpiration();

	if (gameReload)
	{
		// clear lists of looted and locked containers
		ResetLootedContainers();
		ForgetLockedContainers();
	}
}

bool ScanGovernor::IsLockedForHarvest(const RE::TESObjectREFR* refr)
{
	RecursiveLockGuard guard(m_searchLock);
	return m_HarvestLock.contains(refr);
}

size_t ScanGovernor::PendingHarvestNotifications()
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
	return m_HarvestLock.clear();
}

// this triggers/stops loot range calibration cycle
bool ScanGovernor::m_calibrating(false);
int ScanGovernor::m_calibrateRadius(ScanGovernor::CalibrationRangeDelta);
int ScanGovernor::m_calibrateDelta(ScanGovernor::CalibrationRangeDelta);
bool ScanGovernor::m_glowDemo(false);
GlowReason ScanGovernor::m_nextGlow(GlowReason::SimpleTarget);

void ScanGovernor::ToggleCalibration(const bool glowDemo)
{
	RecursiveLockGuard guard(m_searchLock);
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
	auto expiry = currentTime + std::chrono::milliseconds(static_cast<long long>(duration * 1000.0));
	m_glowExpiration[refr] = expiry;
	DBG_VMESSAGE("Trigger glow for %s/0x%08x", refr->GetName(), refr->formID);
	EventPublisher::Instance().TriggerObjectGlow(refr, duration, glowReason);
}

}
