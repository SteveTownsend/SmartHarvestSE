#include "PrecompiledHeaders.h"

#include "dataCase.h"
#include "objects.h"
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
#if _DEBUG
		_DMESSAGE("REFR 0x%08x {%.2f,%.2f,%.2f} trivially too far from player {%.2f,%.2f,%.2f}",
			formID, refr->GetPositionX(), refr->GetPositionY(), refr->GetPositionZ(),
			RE::PlayerCharacter::GetSingleton()->GetPositionX(),
			RE::PlayerCharacter::GetSingleton()->GetPositionY(),
			RE::PlayerCharacter::GetSingleton()->GetPositionZ());
#endif
    	return false;
	}
	double distance(sqrt((dx*dx) + (dy*dy) + (dz*dz)));
#if _DEBUG
	_DMESSAGE("REFR 0x%08x is %.2f units away, loot range %.2f units", formID, distance, m_radius);
#endif
	return distance <= m_radius;
}

bool PlayerCellHelper::CanLoot(RE::TESObjectREFR* refr) const
{
	// prioritize checks that do not require obtaining a lock
	if (!refr)
	{
#if _DEBUG
		_DMESSAGE("null REFR");
#endif
		return false;
	}

	// anything out of looting range is skipped - best way to narrow the list of candidates for looting
	if (!WithinLootingRange(refr))
		return false;

	RE::FormID formID(refr->formID);
	// if 3D not loaded do not measure
	if (!refr->Is3DLoaded())
	{
#if _DEBUG
		_DMESSAGE("skip REFR, 3D not loaded %s/0x%08x", refr->data.objectReference->GetName(), refr->data.objectReference->formID);
#endif
		return false;
	}
	if (refr->formType == RE::FormType::ActorCharacter)
	{
		if (!refr->IsDead(true))
		{
#if _DEBUG
			_DMESSAGE("skip living ActorCharacter %s/0x%08x", refr->data.objectReference->GetName(), refr->data.objectReference->formID);
#endif
			return false;
		}
		if (refr == RE::PlayerCharacter::GetSingleton())
		{
#if _DEBUG
			_DMESSAGE("skip PlayerCharacter %s/0x%08x", refr->data.objectReference->GetName(), refr->data.objectReference->formID);
#endif
			return false;
		}
	}

	if ((refr->data.objectReference->formType == RE::FormType::Flora || refr->data.objectReference->formType == RE::FormType::Tree) &&
		((refr->formFlags & RE::TESObjectREFR::RecordFlags::kHarvested) == RE::TESObjectREFR::RecordFlags::kHarvested))
	{
#if _DEBUG
		_DMESSAGE("skip harvested Flora%s/0x%08x", refr->data.objectReference->GetName(), refr->data.objectReference->formID);
#endif
		return false;
	}

	if (SearchTask::IsLockedForAutoHarvest(refr))
	{
#if _DEBUG
		_DMESSAGE("skip REFR, harvest pending %s/0x%08x", refr->data.objectReference->GetName(), refr->data.objectReference->formID);
#endif
		return false;
	}
	if (SearchTask::IsLootedContainer(refr))
	{
#if _DEBUG
		_DMESSAGE("skip looted container %s/0x%08x", refr->data.objectReference->GetName(), refr->data.objectReference->formID);
#endif
		return false;
	}

	DataCase* data = DataCase::GetInstance();
	if (data->IsReferenceBlocked(refr))
	{
#if _DEBUG
		_DMESSAGE("skip blocked REFR for object/container 0x%08x", formID);
#endif
		return false;
	}
	if (data->IsFormBlocked(refr->data.objectReference))
	{
#if _DEBUG
		_DMESSAGE("skip blocked REFR base form 0x%08x", formID);
#endif
		return false;
	}

	RE::TESFullName* fullName = refr->data.objectReference->As<RE::TESFullName>();
	if (!fullName || fullName->GetFullNameLength() == 0)
	{
		data->BlockForm(refr->data.objectReference);
#if _DEBUG
		_DMESSAGE("block REFR with blank name 0x%08x", formID);
#endif
		return false;
	}

#if _DEBUG
	_DMESSAGE("lootable candidate 0x%08x", formID);
#endif
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
				continue;

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
#if _DEBUG
		_DMESSAGE("Check for adjacent cells to 0x%08x", m_cell->GetFormID());
#endif
		RE::TESWorldSpace* worldSpace(m_cell->worldSpace);
		if (worldSpace)
		{
#if _DEBUG
			_DMESSAGE("Worldspace is %s/0x%08x", worldSpace->GetName(), worldSpace->GetFormID());
#endif
			for (const auto& worldCell : worldSpace->cellMap)
			{
				RE::TESObjectCELL* candidateCell(worldCell.second);
				// skip player cell, handled above
				if (candidateCell == m_cell)
				{
#if _DEBUG
					_DMESSAGE("Player cell, already handled");
#endif
					continue;
				}
				// do not loot across interior/exterior boundary
				if (candidateCell->IsInteriorCell())
				{
#if _DEBUG
					_DMESSAGE("Candidate cell 0x%08x flagged as interior", candidateCell->GetFormID());
#endif
					continue;
				}
				// check for adjacency on the cell grid
				if (!IsAdjacent(candidateCell))
				{
#if _DEBUG
					_DMESSAGE("Skip non-adjacent cell 0x%08x", candidateCell->GetFormID());
#endif
					continue;
				}
				m_adjacentCells.push_back(candidateCell);
#if _DEBUG
				_MESSAGE("Record adjacent cell 0x%08x", candidateCell->GetFormID());
#endif
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
	WindowsUtils::ScopedTimer elapsed("Filter loot candidates in/near cell");
	if (!cell || !cell->IsAttached())
		return std::vector<RE::TESObjectREFR*>();

	m_radius = radius;

	// find the adjacent cells, we only need to scan those and current player cell
	GetAdjacentCells(cell);
	GetCellReferences(m_cell);

	// for exterior cells, also check directly adjacent cells for lootable goodies. Restrict to cells in the same worldspace.
	if (!m_cell->IsInteriorCell())
	{
#if _DEBUG
		_DMESSAGE("Scan cells adjacent to 0x%08x", m_cell->GetFormID());
#endif
		for (const auto& adjacentCell : m_adjacentCells)
		{
			// sanity checks
			if (!adjacentCell || !adjacentCell->IsAttached())
			{
#if _DEBUG
				_DMESSAGE("Adjacent cell null or unattached");
#endif
				continue;
			}
#if _DEBUG
			_MESSAGE("Check adjacent cell 0x%08x", adjacentCell->GetFormID());
#endif
			GetCellReferences(adjacentCell);
		}
	}
	// set up return value and clear accumulator
	std::vector<RE::TESObjectREFR*> result;
	result.swap(m_targets);
	return result;
}
