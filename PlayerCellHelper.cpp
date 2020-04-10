#include "PrecompiledHeaders.h"

#include "dataCase.h"
#include "objects.h"
#include "tasks.h"
#include "PlayerCellHelper.h"

#include <limits>

RE::TESForm* TESObjectCELLHelper::GetOwner(void)
{
	for (RE::BSExtraData& extraData : m_cell->extraList)
	{
		if (extraData.GetType() == RE::ExtraDataType::kOwnership)
		{
#if _DEBUG
			_MESSAGE("TESObjectCELLEx::GetOwner Hit %08x", reinterpret_cast<RE::ExtraOwnership&>(extraData).owner->formID);
#endif
			return reinterpret_cast<RE::ExtraOwnership&>(extraData).owner;
		}
	}
	return nullptr;
}

bool TESObjectCELLHelper::IsPlayerOwned()
{
	RE::TESForm* owner = GetOwner();
	if (owner)
	{
		if (owner->formType == RE::FormType::NPC)
		{
			RE::TESNPC* npc = skyrim_cast<RE::TESNPC*, RE::TESForm>(owner);
			RE::TESNPC* playerBase = skyrim_cast<RE::TESNPC*, RE::TESForm>(RE::PlayerCharacter::GetSingleton()->data.objectReference);
			return (npc && npc == playerBase);
		}
		else if (owner->formType == RE::FormType::Faction)
		{
			RE::TESFaction* faction = skyrim_cast<RE::TESFaction*, RE::TESForm>(owner);
			if (faction)
			{
				if (RE::PlayerCharacter::GetSingleton()->IsInFaction(faction))
					return true;

				return false;
			}
		}
	}
	return false;
}

double GetDistance(const RE::TESObjectREFR* refr, const double radius)
{
	RE::FormID formID(refr->formID);
	double dx = fabs(refr->GetPositionX() - RE::PlayerCharacter::GetSingleton()->GetPositionX());
	double dy = fabs(refr->GetPositionY() - RE::PlayerCharacter::GetSingleton()->GetPositionY());
	double dz = fabs(refr->GetPositionZ() - RE::PlayerCharacter::GetSingleton()->GetPositionZ());

	// don't do FP math if we can trivially see it's too far away
	if (dx > radius || dy > radius || dz > radius)
	{
#if _DEBUG
		_DMESSAGE("REFR 0x%08x {%8.2f,%8.2f,%8.2f} trivially too far from player {%8.2f,%8.2f,%8.2f}",
			formID, refr->GetPositionX(), refr->GetPositionY(), refr->GetPositionZ(),
			RE::PlayerCharacter::GetSingleton()->GetPositionX(),
			RE::PlayerCharacter::GetSingleton()->GetPositionY(),
			RE::PlayerCharacter::GetSingleton()->GetPositionZ());
#endif
    	return std::numeric_limits<double>::max();
	}
	double distance(sqrt((dx*dx) + (dy*dy) + (dz*dz)));
#if _DEBUG
	_DMESSAGE("REFR 0x%08x is %8.2f units from PlayerCharacter", formID, distance);
#endif
	return distance;
}

struct CanLoot_t {
	bool operator()(RE::TESObjectREFR* refr)
	{
		if (!refr)
		{
#if _DEBUG
			_DMESSAGE("null REFR");
#endif
			return false;
		}
		RE::FormID formID(refr->formID);
		if (refr == RE::PlayerCharacter::GetSingleton())
		{
#if _DEBUG
			_DMESSAGE("skip PlayerCharacter 0x%08x", formID);
#endif
			return false;
		}
		if (SearchTask::IsLockedForAutoHarvest(refr))
		{
#if _DEBUG
			_DMESSAGE("skip REFR, harvest pending 0x%08x", formID);
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

		// if 3D not loaded do not measure
		if (!refr->Is3DLoaded())
		{
#if _DEBUG
			_DMESSAGE("skip REFR, 3D not loaded 0x%08x", formID);
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

		if (refr->formType == RE::FormType::ActorCharacter && !refr->IsDead(true))
		{
#if _DEBUG
			_DMESSAGE("skip living ActorCharacter 0x%08x", formID);
#endif
			return false;
		}

		if ((refr->data.objectReference->formType == RE::FormType::Flora || refr->data.objectReference->formType == RE::FormType::Tree) &&
			((refr->formFlags & RE::TESObjectREFR::RecordFlags::kHarvested) == RE::TESObjectREFR::RecordFlags::kHarvested))
		{
#if _DEBUG
			_DMESSAGE("skip harvested Flora 0x%08x", formID);
#endif
			return false;
		}

#if _DEBUG
		_DMESSAGE("lootable candidate 0x%08x", formID);
#endif
		return true;
	}
} CanLoot;

UInt32 GridCellArrayHelper::GetReferences(std::vector<RE::TESObjectREFR*> *out)
{
	TESObjectCELLHelper parent(RE::PlayerCharacter::GetSingleton()->parentCell);
	return parent.GetReferences(out, m_radius, CanLoot);
}