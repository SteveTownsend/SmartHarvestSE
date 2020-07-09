#include "PrecompiledHeaders.h"

#include "Data/dataCase.h"
#include "Utilities/utils.h"
#include "Looting/tasks.h"
#include "WorldState/ActorTracker.h"
#include "WorldState/LocationTracker.h"
#include "FormHelpers/PlayerCellHelper.h"

namespace shse
{

PlayerCellHelper::PlayerCellHelper(DistanceToTarget& refs, const IRangeChecker& rangeCheck) :
	m_refs(refs), m_eliminated(0), m_rangeCheck(rangeCheck), m_nearestDoor(0.)
{
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

bool PlayerCellHelper::CanLoot(const RE::TESObjectREFR* refr) const
{
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

	// if 3D not loaded do not measure
	if (!refr->Is3DLoaded())
	{
		DBG_VMESSAGE("skip REFR 0x%08x, 3D not loaded %s/0x%08x", refr->GetFormID(), refr->GetBaseObject()->GetName(), refr->GetBaseObject()->formID);
		return false;
	}

	if (refr->GetBaseObject()->formType == RE::FormType::Furniture ||
		refr->GetBaseObject()->formType == RE::FormType::Hazard)
	{
		DBG_VMESSAGE("skip ineligible Form Type %s/0x%08x", refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID());
		return false;
	}

	DataCase* data = DataCase::GetInstance();
	if (data->IsFormBlocked(refr->GetBaseObject()))
	{
		DBG_VMESSAGE("skip REFR 0x%08x, blocked base form 0x%08x", refr->formID, refr->GetBaseObject() ? refr->GetBaseObject()->GetFormID() : InvalidForm);
		return false;
	}

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

	const RE::TESFullName* fullName = refr->GetBaseObject()->As<RE::TESFullName>();
	if (!fullName || fullName->GetFullNameLength() == 0)
	{
		data->BlacklistReference(refr);
		DBG_VMESSAGE("blacklist REFR with blank name 0x%08x, base form 0x%08x", refr->formID, refr->GetBaseObject()->GetFormID());
		return false;
	}

	if (refr == RE::PlayerCharacter::GetSingleton())
	{
		DBG_VMESSAGE("skip PlayerCharacter %s/0x%08x", refr->GetBaseObject()->GetName(), refr->GetBaseObject()->formID);
		return false;
	}

	// Actor derives from REFR
	if (refr->GetFormType() == RE::FormType::ActorCharacter)
	{
		if (!refr->IsDead(true))
		{
			DBG_VMESSAGE("skip living Actor/NPC %s/0x%08x", refr->GetBaseObject()->GetName(), refr->GetBaseObject()->formID);
			shse::ActorTracker::Instance().RecordLiveSighting(refr);
			return false;
		}
	}

	if ((refr->GetBaseObject()->formType == RE::FormType::Flora || refr->GetBaseObject()->formType == RE::FormType::Tree) &&
		((refr->formFlags & RE::TESObjectREFR::RecordFlags::kHarvested) == RE::TESObjectREFR::RecordFlags::kHarvested))
	{
		DBG_VMESSAGE("skip REFR 0x%08x to harvested Flora %s/0x%08x", refr->GetFormID(), refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID());
		return false;
	}

	// skip anything not in the required distance-to-player range
	if (!m_rangeCheck.IsValid(refr))
		return false;

	// Looting through Doors not allowed - check distance to this one and record if this is the nearest so far
	if (refr->GetBaseObject()->formType == RE::FormType::Door)
	{
		if (m_nearestDoor == 0. || m_rangeCheck.Distance() < m_nearestDoor)
		{
			DBG_VMESSAGE("New nearest Door 0x%08x(%s) at distance %.2f", refr->GetFormID(), refr->GetBaseObject()->GetName(), m_rangeCheck.Distance());
			m_nearestDoor = m_rangeCheck.Distance();
		}
		else
		{
			DBG_VMESSAGE("skip Door 0x%08x(%s) at distance %.2f", refr->GetFormID(), refr->GetBaseObject()->GetName(), m_rangeCheck.Distance());
		}
		return false;
	}

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

	DBG_VMESSAGE("lootable candidate 0x%08x", refr->formID);
	return true;
}

bool PlayerCellHelper::IsLootCandidate(const RE::TESObjectREFR* refr) const
{
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
		if (refr == RE::PlayerCharacter::GetSingleton())
		{
			DBG_VMESSAGE("skip PlayerCharacter %s/0x%08x", refr->GetBaseObject()->GetName(), refr->GetBaseObject()->formID);
			return false;
		}
	}

	// skip anything not in the required distance-to-player range
	if (!m_rangeCheck.IsValid(refr))
		return false;

	// FormID can be retrieved using pointer, but we should not dereference the pointer as the REFR may have been recycled
	RE::FormID dynamicForm(SearchTask::LootedDynamicContainerFormID(refr));
	if (dynamicForm != InvalidForm)
	{
		DBG_VMESSAGE("skip looted dynamic container at %p with Form ID 0x%08x", refr, dynamicForm);
		return false;
	}
	const RE::TESFullName* fullName = refr->GetBaseObject()->As<RE::TESFullName>();
	if (!fullName || fullName->GetFullNameLength() == 0)
	{
		data->BlacklistReference(refr);
		DBG_VMESSAGE("blacklist REFR with blank name 0x%08x", refr->formID);
		return false;
	}

	DBG_VMESSAGE("lootable candidate 0x%08x", refr->formID);
	return true;
}

void PlayerCellHelper::FindLootableReferences() const
{
	m_predicate = std::bind(&PlayerCellHelper::CanLoot, this, std::placeholders::_1);
	FilterNearbyReferences();
}

void PlayerCellHelper::FindAllCandidates() const
{
	m_predicate = std::bind(&PlayerCellHelper::IsLootCandidate, this, std::placeholders::_1);
	FilterNearbyReferences();
}

void PlayerCellHelper::FilterCellReferences(const RE::TESObjectCELL* cell) const
{
	// Do not scan reference list until cell is attached
	if (!cell->IsAttached())
		return;

	for (const RE::TESObjectREFRPtr& refptr : cell->references)
	{
		RE::TESObjectREFR* refr(refptr.get());
		if (refr)
		{
			if (!m_predicate(refr))
			{
				++m_eliminated;
				continue;
			}

			m_refs.emplace_back(m_rangeCheck.Distance(), refr);
		}
	}
}

void PlayerCellHelper::FilterNearbyReferences() const
{
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Filter loot candidates in/near cell");
#endif
	const RE::TESObjectCELL* cell(LocationTracker::Instance().PlayerCell());
	if (!cell)
		return;

	// For exterior cells, also check directly adjacent cells for lootable goodies. Restrict to cells in the same worldspace.
	// If current cell fills the list then ignore others.
	FilterCellReferences(cell);
	if (!LocationTracker::Instance().IsPlayerIndoors())
	{
		DBG_VMESSAGE("Scan cells adjacent to 0x%08x", cell->GetFormID());
		for (const auto& adjacentCell : LocationTracker::Instance().AdjacentCells())
		{
			// sanity checks
			if (!adjacentCell || !adjacentCell->IsAttached())
			{
				DBG_VMESSAGE("Adjacent cell null or unattached");
				continue;
			}
			DBG_VMESSAGE("Check adjacent cell 0x%08x", adjacentCell->GetFormID());
			FilterCellReferences(adjacentCell);
		}
	}
	// Summary of unlootable REFRs
	DBG_VMESSAGE("Eliminated %d REFRs for cell 0x%08x", m_eliminated, cell->GetFormID());
}

}