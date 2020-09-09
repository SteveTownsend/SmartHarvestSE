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

#include "WorldState/PlacedObjects.h"
#include "Collections/Collection.h"
#include "Data/dataCase.h"
#include "Looting/ManagedLists.h"
#include "Utilities/utils.h"

namespace shse
{

std::unique_ptr<PlacedObjects> PlacedObjects::m_instance;

PlacedObjects& PlacedObjects::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<PlacedObjects>();
	}
	return *m_instance;
}

PlacedObjects::PlacedObjects()
{
}

// record all the Placed instances, so we can validate Collections and send Player to find items later
void PlacedObjects::RecordPlacedItem(const RE::TESForm* item, const RE::TESObjectREFR* refr)
{
	m_placedItems.insert(item);
	if (m_placedObjects[item].insert(refr).second)
	{
		REL_VMESSAGE("REFR 0x{:08x} to item {}/0x{:08x} is a Placed Object", refr->GetFormID(), item->GetName(), item->GetFormID());
	}
}

void PlacedObjects::SaveREFRIfPlaced(const RE::TESObjectREFR* refr)
{
	// skip if empty REFR
	if (!refr)
	{
		DBG_VMESSAGE("REFR invalid");
		return;
	}
	// skip if BaseObject not concrete
	if (!FormUtils::IsConcrete(refr->GetBaseObject()))
	{
		DBG_VMESSAGE("REFR 0x{:08x} Base 0x{:08x} is missing, non-playable or unnamed",
			refr->GetFormID(), refr->GetBaseObject() ? refr->GetBaseObject()->GetFormID() : InvalidForm);
		return;
	}

	// skip if not a valid BaseObject for Collections, or a placed Container or Corpse that we need to introspect
	if (!SignatureCondition::IsValidFormType(refr->GetBaseObject()->GetFormType()) &&
		refr->GetBaseObject()->GetFormType() != RE::FormType::Container &&
		refr->GetFormType() != RE::FormType::ActorCharacter)
	{
		DBG_VMESSAGE("REFR 0x{:08x} Base {}/0x{:08x} invalid FormType {}", refr->GetFormID(), refr->GetBaseObject()->GetName(),
			refr->GetBaseObject()->GetFormID(), refr->GetBaseObject()->GetFormType());
		return;
	}
	// skip if not enabled at start of game - different checks for Actor and REFR
	if (refr->GetFormType() == RE::FormType::ActorCharacter &&
		(refr->As<RE::Actor>()->formFlags & RE::Actor::RecordFlags::kStartsDead) != RE::Actor::RecordFlags::kStartsDead)
	{
		DBG_VMESSAGE("Actor 0x{:08x} Base {}/0x{:08x} does not Start Dead", refr->GetFormID(), refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID());
		return;
	}
	if ((refr->formFlags & RE::TESObjectREFR::RecordFlags::kInitiallyDisabled) == RE::TESObjectREFR::RecordFlags::kInitiallyDisabled)
	{
		DBG_VMESSAGE("REFR 0x{:08x} Base {}/0x{:08x} initially disabled", refr->GetFormID(), refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID());
		return;
	}
	if (refr->GetBaseObject()->GetFormType() == RE::FormType::Container || refr->GetFormType() == RE::FormType::ActorCharacter)
	{
		if (DataCase::GetInstance()->IsOffLimitsContainer(refr))
		{
			DBG_VMESSAGE("Container REFR {}/0x{:08x} is off-limits", refr->GetName(), refr->GetFormID());
			return;
		}
		if (DataCase::GetInstance()->ReferencesBlacklistedContainer(refr))
		{
			DBG_VMESSAGE("Container REFR {}/0x{:08x} is blacklisted", refr->GetName(), refr->GetFormID());
			return;
		}
		const RE::TESContainer* container(const_cast<RE::TESObjectREFR*>(refr)->GetContainer());
		container->ForEachContainerObject([&](RE::ContainerObject& entry) -> bool {
			auto entryContents(entry.obj);
			if (!SignatureCondition::IsValidFormType(entryContents->GetFormType()))
			{
				DBG_VMESSAGE("Container/NPC {}/0x{:08x} item {}/0x{:08x} FormType {} invalid", refr->GetName(), refr->GetFormID(), entryContents->GetName(),
					entryContents->GetFormID(), entryContents->GetFormType());
			}
			else
			{
				RecordPlacedItem(entryContents, refr);
			}
			// continue the scan
			return true;
		});
	}
	else
	{
		RecordPlacedItem(refr->GetBaseObject(), refr);
	}
}

// This logic works at startup for non-Masters. If the REFR is from a master, temp REFRs are loaded on demand and we can
// check again then.
// TODO check if this does work on cell load, and that the REFRs are still valid after we change location
void PlacedObjects::RecordPlacedObjectsForCell(const RE::TESObjectCELL* cell)
{
	if (!m_checkedForPlacedObjects.insert(cell).second)
		return;

	if (ManagedList::BlackList().Contains(cell))
		return;

	if (DataCase::GetInstance()->IsOffLimitsLocation(cell))
		return;

	if (!IsCellLocatable(cell))
	{
		if (!cell)
			return;
		// no obvious way to locate - save possibly-reachable doors for the cell
		bool connected(false);
		for (const RE::TESObjectREFRPtr& refptr : cell->references)
		{
			const RE::TESObjectREFR* refr(refptr.get());
			if (refr->GetBaseObject() && refr->GetBaseObject()->GetFormType() == RE::FormType::Door)
			{
				// check Worldspace connection
				const auto linkage(m_linkingDoors.find(refr));
				if (linkage != m_linkingDoors.cend())
				{
					if (linkage->second->parentCell && IsCellLocatable(linkage->second->parentCell))
					{
						DBG_VMESSAGE("REFR 0x{:08x} in CELL {}/0x{:08x} can teleport via {}/0x{:08x}", refr->GetFormID(), FormUtils::SafeGetFormEditorID(cell).c_str(),
							cell->GetFormID(), FormUtils::SafeGetFormEditorID(linkage->second->parentCell).c_str(), linkage->second->parentCell->GetFormID());
						connected = true;
						break;
					}
					DBG_VMESSAGE("REFR 0x{:08x} in CELL {}/0x{:08x} cannot teleport via REFR {}/0x{:08x}", refr->GetFormID(), FormUtils::SafeGetFormEditorID(cell).c_str(),
						cell->GetFormID(), FormUtils::SafeGetFormEditorID(linkage->second).c_str(), linkage->second->GetFormID());
				}
			}
		}
		if (!connected)
			return;
	}

	size_t actors(std::count_if(cell->references.cbegin(), cell->references.cend(),
		[&](const auto refr) -> bool { return refr->GetFormType() == RE::FormType::ActorCharacter; }));
	DBG_MESSAGE("Process {} REFRs including {} actors in CELL {}/0x{:08x}", cell->references.size(), actors, FormUtils::SafeGetFormEditorID(cell).c_str(), cell->GetFormID());
	for (const RE::TESObjectREFRPtr& refptr : cell->references)
	{
		const RE::TESObjectREFR* refr(refptr.get());
		SaveREFRIfPlaced(refr);
	}
}

// skip if not in a valid worldspace or Location
// TODO might miss some stray CELLs
bool PlacedObjects::IsCellLocatable(const RE::TESObjectCELL* cell)
{
#if 1
	return true;
#else
	if (!cell)
		return false;
	const RE::ExtraLocation* extraLocation(cell->extraList.GetByType<RE::ExtraLocation>());
	if (extraLocation && extraLocation->location)
	{
		DBG_VMESSAGE("CELL {}/0x{:08x} is in Location {}/0x{:08x}", FormUtils::SafeGetFormEditorID(cell).c_str(), cell->GetFormID(),
			extraLocation->location->GetName(), extraLocation->location->GetFormID());
		return true;
	}
	else if (cell->worldSpace)
	{
		DBG_VMESSAGE("CELL {}/0x{:08x} is in WorldSpace {}", FormUtils::SafeGetFormEditorID(cell).c_str(), cell->GetFormID(), cell->worldSpace->GetName());
		return true;
	}
	DBG_VMESSAGE("CELL {}/0x{:08x} unlocatable", FormUtils::SafeGetFormEditorID(cell).c_str(), cell->GetFormID());
	return false;
#endif
}

void PlacedObjects::RecordPlacedObjects(void)
{
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Record Placed Objects");
#endif

	// list all placed objects of interest for Collections - don't quest for anything we cannot see
	for (const auto worldSpace : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESWorldSpace>())
	{
		if (!worldSpace->persistentCell)
			continue;
		// find all DOORs in the persistent CELL, these link other CELLs
		DBG_MESSAGE("Search for DOORs {} REFRs in persistent cell for WorldSpace {}/0x{:08x}", worldSpace->persistentCell->references.size(), worldSpace->GetName(), worldSpace->GetFormID());
		for (const auto& refPtr : worldSpace->persistentCell->references)
		{
			if (!refPtr)
				continue;
			const RE::TESObjectREFR* refr(refPtr.get());
			if (refr->GetBaseObject() && refr->GetBaseObject()->GetFormType() == RE::FormType::Door)
			{
				if (refr->extraList.HasType<RE::ExtraTeleport>())
				{
					// Store door link representing entry from either direction
					const auto teleport(refr->extraList.GetByType<RE::ExtraTeleport>());
					if (!teleport || !teleport->teleportData || !teleport->teleportData->linkedDoor)
						continue;
					const RE::TESObjectREFR* targetDoorRefr(teleport->teleportData->linkedDoor.get().get());
					m_linkingDoors.insert({ refr, targetDoorRefr });
					m_linkingDoors.insert({ targetDoorRefr, refr });
					DBG_VMESSAGE("Linking Doors 0x{:08x} and 0x{:08x}", refr->GetFormID(), targetDoorRefr->GetFormID());
				}
			}
		}
		// check direct CELL to CELL connection
		for (const auto cellEntry : worldSpace->cellMap)
		{
			const auto cell(cellEntry.second);
			for (const auto& refPtr : cell->references)
			{
				if (!refPtr)
					continue;
				const RE::TESObjectREFR* refr(refPtr.get());
				if (refr->GetBaseObject() && refr->GetBaseObject()->GetFormType() == RE::FormType::Door)
				{
					if (refr->extraList.HasType<RE::ExtraTeleport>())
					{
						const auto teleport(refr->extraList.GetByType<RE::ExtraTeleport>());
						if (!teleport || !teleport->teleportData || !teleport->teleportData->linkedDoor)
							continue;
						const RE::TESObjectREFR* target(teleport->teleportData->linkedDoor.get().get());
						if (!target)
						{
							DBG_VMESSAGE("REFR 0x{:08x} in CELL {}/0x{:08x} teleport unusable via RefHandle 0x{:08x}", refr->GetFormID(),
								FormUtils::SafeGetFormEditorID(cell), cell->GetFormID(), teleport->teleportData->linkedDoor.native_handle());
							continue;
						}
						if (!target->parentCell)
						{
							DBG_VMESSAGE("REFR 0x{:08x} in CELL {}/0x{:08x} teleport unusable via REFR 0x{:08x}", refr->GetFormID(), FormUtils::SafeGetFormEditorID(cell).c_str(),
								cell->GetFormID(), target->GetFormID());
							continue;
						}
						if (!IsCellLocatable(target->parentCell))
						{
							DBG_VMESSAGE("REFR 0x{:08x} in CELL {}/0x{:08x} teleport unusable via {}/0x{:08x}", refr->GetFormID(), FormUtils::SafeGetFormEditorID(cell),
								cell->GetFormID(), FormUtils::SafeGetFormEditorID(target->parentCell), target->parentCell->GetFormID());
							continue;
						}
						DBG_VMESSAGE("REFR 0x{:08x} in CELL {}/0x{:08x} teleport connects to CELL {}/0x{:08x}", refr->GetFormID(), FormUtils::SafeGetFormEditorID(cell),
							cell->GetFormID(), FormUtils::SafeGetFormEditorID(target->parentCell), target->parentCell->GetFormID());
						m_linkingDoors.insert({ refr, target });
						m_linkingDoors.insert({ target, refr });
					}
				}
			}
		}
	}

	// Process CELLs now linking DOORs are identified
	for (const auto worldSpace : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESWorldSpace>())
	{
		DBG_MESSAGE("Process {} CELLs in WorldSpace Map for {}/0x{:08x}", worldSpace->cellMap.size(), worldSpace->GetName(), worldSpace->GetFormID());
		for (const auto cellEntry : worldSpace->cellMap)
		{
			RecordPlacedObjectsForCell(cellEntry.second);
		}
	}
	DBG_MESSAGE("Process {} Interior CELLs", RE::TESDataHandler::GetSingleton()->interiorCells.size());
	for (const auto cell : RE::TESDataHandler::GetSingleton()->interiorCells)
	{
		RecordPlacedObjectsForCell(cell);
	}
	size_t placed(0);
	placed = std::accumulate(m_placedObjects.cbegin(), m_placedObjects.cend(), placed,
		[&] (const size_t& result, const auto& keyList) { return result + keyList.second.size(); });
	REL_MESSAGE("{} Placed Objects recorded for {} Items", placed, m_placedItems.size());
}

bool PlacedObjects::IsPlacedObject(const RE::TESForm* form) const
{
	RecursiveLockGuard guard(m_placedLock);
	return m_placedObjects.find(form) != m_placedObjects.cend();
}

size_t PlacedObjects::NumberOfInstances(const RE::TESForm* form) const
{
	RecursiveLockGuard guard(m_placedLock);
	const auto& matched(m_placedObjects.find(form));
	if (matched != m_placedObjects.cend())
	{
		return matched->second.size();
	}
	return 0;
}

}