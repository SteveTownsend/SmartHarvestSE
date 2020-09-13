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
#include "Looting/TheftCoordinator.h"
#include "Looting/TryLootREFR.h"
#include "VM/EventPublisher.h"
#include "WorldState/ActorTracker.h"
#include "Utilities/utils.h"

namespace shse
{

std::unique_ptr<TheftCoordinator> TheftCoordinator::m_instance;

TheftCoordinator& TheftCoordinator::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<TheftCoordinator>();
	}
	return *m_instance;
}

void TheftCoordinator::DelayStealableItem(RE::TESObjectREFR* target, INIFile::SecondaryType targetType)
{
	RecursiveLockGuard guard(m_theftLock);
	// speculative until we make the check for already-in-progress
	m_refrsToSteal.push_back({ target, targetType });
}

// called at end of periodic scan
void TheftCoordinator::StealIfUndetected(void)
{
	RecursiveLockGuard guard(m_theftLock);
	// if we are still waiting for last batch to process, do not trigger
	if (!m_refrsToSteal.empty() && !m_stealInProgress)
	{
		m_refrsStealInProgress.swap(m_refrsToSteal);
		m_stealInProgress = true;
		m_detectingActors = ActorTracker::Instance().GetDetectives();
		DBG_VMESSAGE("Steal {} items/containers under the nose of {} Actors", m_refrsStealInProgress.size(), m_detectingActors.size());
		// start timer before issuing event in case result comes back really quickly (however unlikely)
		m_stealTimer = WindowsUtils::ScopedTimerFactory::Instance().StartTimer("Steal async");
		static const bool dryRun(false);
		EventPublisher::Instance().TriggerStealIfUndetected(m_detectingActors.size(), dryRun);
	}
	else
	{
		// clear the REFR list - no steal triggered this time. Actor list left in place for possible REFR dry run.
		m_refrsToSteal.clear();
	}
}

const RE::Actor* TheftCoordinator::ActorByIndex(const size_t actorIndex) const
{
	RecursiveLockGuard guard(m_theftLock);
	if (actorIndex < m_detectingActors.size())
		return m_detectingActors[actorIndex];
	return nullptr;
}

// after checking Player detection state with respect to eligible Actors, script will report status via this API
void TheftCoordinator::StealOrForgetItems(const bool detected)
{
	WindowsUtils::ScopedTimerFactory::Instance().StopTimer(m_stealTimer);
	decltype(m_refrsStealInProgress) items;
	{
		//hold lock only while using shared state. Item stealing should be thread-safe
		RecursiveLockGuard guard(m_theftLock);
		WindowsUtils::ScopedTimerFactory::Instance().StopTimer(m_stealTimer);
		m_stealTimer = -1;
		items.swap(m_refrsStealInProgress);
		m_stealInProgress = false;
	}
	DBG_VMESSAGE("Detected = {} for stealing of {} items/containers", detected ? "true" : "false", items.size());
	if (!detected)
	{
		static const bool stolen(true);
		static const bool dryRun(false);
		static const bool glowOnly(false);
		for (const auto& item : items)
		{
			TryLootREFR(item.first, item.second, stolen, glowOnly).Process(dryRun);
		}
	}
}

bool TheftCoordinator::StealingItems() const
{
	RecursiveLockGuard guard(m_theftLock);
	return m_stealInProgress;
}

}