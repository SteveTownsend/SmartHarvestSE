#include "PrecompiledHeaders.h"

#include "tasks.h"
#include "PlayerCellHelper.h"

PlayerCellHelper PlayerCellHelper::m_instance;
std::vector<RE::TESObjectCELL*> PlayerCellHelper::m_adjacentCells;

bool PlayerCellHelper::WithinLootingRange(const RE::TESObjectREFR* refr) const
{
	RE::FormID formID(refr->formID);
	double dx = fabs(refr->GetPositionX() - RE::PlayerCharacter::GetSingleton()->GetPositionX());
	double dy = fabs(refr->GetPositionY() - RE::PlayerCharacter::GetSingleton()->GetPositionY());
	double dz = fabs(refr->GetPositionZ() - RE::PlayerCharacter::GetSingleton()->GetPositionZ());

	// don't do Floating Point math if we can trivially see it's too far away
	if (dx > m_radius || dy > m_radius || dz > m_radius)
	{
		// very verbose
		DBG_DMESSAGE("REFR 0x%08x {%.2f,%.2f,%.2f} trivially too far from player {%.2f,%.2f,%.2f}",
			formID, refr->GetPositionX(), refr->GetPositionY(), refr->GetPositionZ(),
			RE::PlayerCharacter::GetSingleton()->GetPositionX(),
			RE::PlayerCharacter::GetSingleton()->GetPositionY(),
			RE::PlayerCharacter::GetSingleton()->GetPositionZ());
    	return false;
	}
	double distance(sqrt((dx*dx) + (dy*dy) + (dz*dz)));
	DBG_VMESSAGE("REFR 0x%08x is %.2f units away, loot range %.2f units", formID, distance, m_radius);
	return distance <= m_radius;
}

/*
Lootability can be subjective and/or time-sensitive. Dynamic forms (FormID 0xffnnnnnn) may be deleted from under our feet.
Current hypothesis is that this is safe so long as the base object is not itself dynamic.
Example from play-testing CTD logs where we crashed getting FormID for a pending-loot dead body.

Line 3732034: 0x8e70 (2020 - 05 - 23 07:24 : 16.910) J : \GitHub\SmartHarvestSE\tasks.cpp(1034) : [MESSAGE] Process REFR 0xff0024e9 with base object Ancient Nord Arrow / 0x0003be1a
Line 3735016 : 0x8e70 (2020 - 05 - 23 07:24 : 17.800) J : \GitHub\SmartHarvestSE\tasks.cpp(1034) : [MESSAGE] Process REFR 0xff0024e9 with base object Ancient Nord Arrow / 0x0003be1a
Line 3737998 : 0x8e70 (2020 - 05 - 23 07:24 : 18.700) J : \GitHub\SmartHarvestSE\tasks.cpp(1034) : [MESSAGE] Process REFR 0xff0024e9 with base object Ancient Nord Arrow / 0x0003be1a
Line 3785563 : 0x8e70 (2020 - 05 - 23 07:24 : 36.095) J : \GitHub\SmartHarvestSE\tasks.cpp(1034) : [MESSAGE] Process REFR 0xff0024eb with base object Restless Skeleton / 0xff0024ed
Line 3785564 : 0x8e70 (2020-05-23 07:24:36.095) J:\GitHub\SmartHarvestSE\tasks.cpp(277): [DEBUG] Enqueued dead body to loot later 0xff0024eb
Line 3785565 : 0x8e70 (2020-05-23 07:24:36.095) J:\GitHub\SmartHarvestSE\utils.cpp(211): [MESSAGE] TIME(Process Auto-loot Candidate Restless Skeleton/0xff0024ed)=114 micros

When we come to process this dead body a little later, we get CTD. The FormID has morphed, meaning the REFR was junked.

Line 3797443 : 0x8e70 (2020-05-23 07:24:39.705) J:\GitHub\SmartHarvestSE\tasks.cpp(295): [DEBUG] Process enqueued dead body 0x10000000
Line 3797443 : 0x8e70 (2020-05-23 07:24:40.707) J:\GitHub\SmartHarvestSE\LogStackWalker.cpp(7): [MESSAGE] Callstack dump :
...snip...
J:\GitHub\CommonLibSSE\src\RE\TESForm.cpp (110): RE::TESForm::GetFormID
J:\GitHub\SmartHarvestSE\tasks.cpp (1033): SearchTask::DoPeriodicSearch
J:\GitHub\SmartHarvestSE\tasks.cpp (690): SearchTask::ScanThread
J:\GitHub\SmartHarvestSE\tasks.cpp (705): `SearchTask::Start'::`2'::<lambda_1>::operator()
C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.25.28610\include\type_traits (1610): std::_Invoker_functor::_Call<`SearchTask::Start'::`2'::<lambda_1> >
C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.25.28610\include\type_traits (1610): std::invoke<`SearchTask::Start'::`2'::<lambda_1> >
C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.25.28610\include\thread (44): std::thread::_Invoke<std::tuple<`SearchTask::Start'::`2'::<lambda_1> >,0>

The failing REFR is a dynamic form referencing a dynamic base form. Earlier REFRs dynamic -> non-dynamic worked OK.

Dynamic forms may be deleted by script. This means we must be especially wary in handling them. 
For now, choose to flat-out ignore any REFR to a Dynamic Base - manual looting is still possible if REFR is not deleted. Filtering also includes
never recording a Dynamic REFR or Base Form in our filter lists, as Dynamic REFR FormIDs are recycled.
*/

bool PlayerCellHelper::CanLoot(RE::TESObjectREFR* refr) const
{
	// prioritize checks that do not require obtaining a lock
	if (!refr)
	{
		DBG_VMESSAGE("null REFR");
		return false;
	}

	if (!refr->GetBaseObject())
	{
		DBG_VMESSAGE("null base object for REFR 0x%08x", refr->GetFormID());
		return false;
	}

	DataCase* data = DataCase::GetInstance();
	if (data->IsReferenceBlocked(refr))
	{
		DBG_VMESSAGE("skip blocked REFR for object/container 0x%08x", refr->formID);
		return false;
	}

	// check blacklist early - this may be a malformed REFR e.g. GetBaseObject() blank, 0x00000000 FormID
	// as observed in play testing
	if (data->IsReferenceOnBlacklist(refr))
	{
		DBG_VMESSAGE("skip blacklisted REFR 0x%08x", refr->GetFormID());
		return false;
	}

	// if 3D not loaded do not measure
	if (!refr->Is3DLoaded())
	{
		DBG_VMESSAGE("skip REFR, 3D not loaded %s/0x%08x", refr->GetBaseObject()->GetName(), refr->GetBaseObject()->formID);
		return false;
	}

	if (refr->formType == RE::FormType::ActorCharacter)
	{
		if (!refr->IsDead(true))
		{
			DBG_VMESSAGE("skip living ActorCharacter %s/0x%08x", refr->GetBaseObject()->GetName(), refr->GetBaseObject()->formID);
			return false;
		}
		if (refr == RE::PlayerCharacter::GetSingleton())
		{
			DBG_VMESSAGE("skip PlayerCharacter %s/0x%08x", refr->GetBaseObject()->GetName(), refr->GetBaseObject()->formID);
			return false;
		}
	}

	if ((refr->GetBaseObject()->formType == RE::FormType::Flora || refr->GetBaseObject()->formType == RE::FormType::Tree) &&
		((refr->formFlags & RE::TESObjectREFR::RecordFlags::kHarvested) == RE::TESObjectREFR::RecordFlags::kHarvested))
	{
		DBG_VMESSAGE("skip harvested REFR 0x%08x to Flora %s/0x%08x", refr->GetFormID(), refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID());
		return false;
	}

	if (refr->GetBaseObject()->formType == RE::FormType::Furniture ||
		refr->GetBaseObject()->formType == RE::FormType::Hazard ||
		refr->GetBaseObject()->formType == RE::FormType::Door)
	{
		DBG_VMESSAGE("skip ineligible Form Type %s/0x%08x", refr->GetBaseObject()->GetName(), refr->GetBaseObject()->formID);
		return false;
	}

	// anything out of looting range is skipped
	if (!WithinLootingRange(refr))
		return false;

	if (SearchTask::IsLockedForHarvest(refr))
	{
		DBG_VMESSAGE("skip REFR, harvest pending %s/0x%08x", refr->GetBaseObject()->GetName(), refr->GetBaseObject()->formID);
		return false;
	}
	if (SearchTask::IsLootedContainer(refr))
	{
		DBG_VMESSAGE("skip looted container %s/0x%08x", refr->GetBaseObject()->GetName(), refr->GetBaseObject()->formID);
		return false;
	}
	// FormID can be retrieved using pointer, but we should not dereference the pointer as the REFR may have been recycled
	RE::FormID dynamicForm(SearchTask::LootedDynamicContainerFormID(refr));
	if (dynamicForm != InvalidForm)
	{
		DBG_VMESSAGE("skip looted dynamic container at %p with Form ID 0x%08x", refr, dynamicForm);
		return false;
	}
	if (data->IsFormBlocked(refr->GetBaseObject()))
	{
		DBG_VMESSAGE("skip blocked REFR base form 0x%08x", refr->formID);
		return false;
	}

	RE::TESFullName* fullName = refr->GetBaseObject()->As<RE::TESFullName>();
	if (!fullName || fullName->GetFullNameLength() == 0)
	{
		data->BlacklistReference(refr);
		DBG_VMESSAGE("blacklist REFR with blank name 0x%08x", refr->formID);
		return false;
	}

	DBG_VMESSAGE("lootable candidate 0x%08x", refr->formID);
	return true;
}

void PlayerCellHelper::GetCellReferences(const RE::TESObjectCELL* cell)
{
	// Do not scan reference list until cell is attached
	if (!cell->IsAttached())
		return;

	for (const RE::TESObjectREFRPtr& refptr : cell->references)
	{
		RE::TESObjectREFR* refr(refptr.get());
		if (refr)
		{
			if (!CanLoot(refr))
			{
				++m_eliminated;
				continue;
			}

			m_targets.push_back(refr);
		}
	}
}

void PlayerCellHelper::GetAdjacentCells(RE::TESObjectCELL* cell)
{
	// if this is the same cell we last checked, the list of adjacent cells does not need rebuilding
	if (m_cell == cell)
		return;

	m_cell = cell;
	m_adjacentCells.clear();

	// for exterior cells, also check directly adjacent cells for lootable goodies. Restrict to cells in the same worldspace.
	if (!m_cell->IsInteriorCell())
	{
		DBG_MESSAGE("Check for adjacent cells to 0x%08x", m_cell->GetFormID());
		RE::TESWorldSpace* worldSpace(m_cell->worldSpace);
		if (worldSpace)
		{
			DBG_MESSAGE("Worldspace is %s/0x%08x", worldSpace->GetName(), worldSpace->GetFormID());
			for (const auto& worldCell : worldSpace->cellMap)
			{
				RE::TESObjectCELL* candidateCell(worldCell.second);
				// skip player cell, handled above
				if (candidateCell == m_cell)
				{
					DBG_MESSAGE("Player cell, already handled");
					continue;
				}
				// do not loot across interior/exterior boundary
				if (candidateCell->IsInteriorCell())
				{
					DBG_MESSAGE("Candidate cell 0x%08x flagged as interior", candidateCell->GetFormID());
					continue;
				}
				// check for adjacency on the cell grid
				if (!IsAdjacent(candidateCell))
				{
					DBG_MESSAGE("Skip non-adjacent cell 0x%08x", candidateCell->GetFormID());
					continue;
				}
				m_adjacentCells.push_back(candidateCell);
				DBG_MESSAGE("Record adjacent cell 0x%08x", candidateCell->GetFormID());
			}
		}
	}
}

bool PlayerCellHelper::IsAdjacent(RE::TESObjectCELL* cell) const
{
	// XCLC data available since both are exterior cells, by construction
	const auto checkCoordinates(cell->GetCoordinates());
	const auto myCoordinates(m_cell->GetCoordinates());
	return std::abs(myCoordinates->cellX - checkCoordinates->cellX) <= 1 &&
		std::abs(myCoordinates->cellY - checkCoordinates->cellY) <= 1;
}

std::vector<RE::TESObjectREFR*> PlayerCellHelper::GetReferences(RE::TESObjectCELL* cell, const double radius)
{
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Filter loot candidates in/near cell");
#endif
	if (!cell || !cell->IsAttached())
		return std::vector<RE::TESObjectREFR*>();

	m_radius = radius;
	m_eliminated = 0;

	// find the adjacent cells, we only need to scan those and current player cell
	GetAdjacentCells(cell);
	GetCellReferences(m_cell);

	// for exterior cells, also check directly adjacent cells for lootable goodies. Restrict to cells in the same worldspace.
	if (!m_cell->IsInteriorCell())
	{
		DBG_VMESSAGE("Scan cells adjacent to 0x%08x", m_cell->GetFormID());
		for (const auto& adjacentCell : m_adjacentCells)
		{
			// sanity checks
			if (!adjacentCell || !adjacentCell->IsAttached())
			{
				DBG_VMESSAGE("Adjacent cell null or unattached");
				continue;
			}
			DBG_VMESSAGE("Check adjacent cell 0x%08x", adjacentCell->GetFormID());
			GetCellReferences(adjacentCell);
		}
	}
	// Summary of unlootable REFRs
	DBG_VMESSAGE("Eliminated %d REFRs for cell 0x%08x", m_eliminated, m_cell->GetFormID());
	// set up return value and clear accumulator
	std::vector<RE::TESObjectREFR*> result;
	result.swap(m_targets);
	return result;
}
