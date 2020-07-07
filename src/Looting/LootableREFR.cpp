#include "PrecompiledHeaders.h"

#include "Looting/LootableREFR.h"
#include "FormHelpers/ExtraDataListHelper.h"
#include "FormHelpers/FormHelper.h"
#include "Looting/objects.h"

LootableREFR::LootableREFR(const RE::TESObjectREFR* ref, const INIFile::SecondaryType scope) : m_ref(ref), m_lootable(nullptr), m_scope(scope)
{
	m_objectType = GetREFRObjectType(m_ref);
	m_typeName = GetObjectTypeName(m_objectType);
}

bool LootableREFR::IsQuestItem(const bool requireFullQuestFlags)
{
	if (!m_ref)
		return false;

	RE::RefHandle handle;
	RE::CreateRefHandle(handle, const_cast<RE::TESObjectREFR*>(m_ref));

	RE::NiPointer<RE::TESObjectREFR> targetRef;
	RE::LookupReferenceByHandle(handle, targetRef);

	if (!targetRef)
		targetRef.reset(const_cast<RE::TESObjectREFR*>(m_ref));

	ExtraDataListHelper extraListEx(&targetRef->extraList);
	if (!extraListEx.m_extraData)
		return false;

	return extraListEx.IsQuestObject(requireFullQuestFlags);
}

std::pair<bool, SpecialObjectHandling> LootableREFR::TreatAsCollectible(void) const
{
	TESFormHelper itemEx(m_lootable ? m_lootable : m_ref->GetBaseObject(), m_scope);
	return itemEx.TreatAsCollectible();
}

bool LootableREFR::IsValuable() const
{
	TESFormHelper itemEx(m_lootable ? m_lootable : m_ref->GetBaseObject(), m_scope);
	return itemEx.IsValuable();
}

RE::TESForm* LootableREFR::GetLootable() const
{
	return m_lootable;
}

void LootableREFR::SetLootable(RE::TESForm* lootable)
{
	m_lootable = lootable;
}

SInt32 LootableREFR::CalculateWorth(void) const
{
	TESFormHelper itemEx(m_lootable ? m_lootable : m_ref->GetBaseObject(), m_scope);
	return itemEx.GetWorth();
}

double LootableREFR::GetWeight(void) const
{
	TESFormHelper itemEx(m_lootable ? m_lootable : m_ref->GetBaseObject(), m_scope);
	return itemEx.GetWeight();
}

const char* LootableREFR::GetName() const
{
	return m_ref->GetName();
}

UInt32 LootableREFR::GetFormID() const
{
	return m_ref->GetBaseObject()->formID;
}

SInt16 LootableREFR::GetItemCount()
{
	if (!m_ref)
		return 1;
	if (!m_ref->GetBaseObject())
		return 1;

	const RE::ExtraCount* exCount(m_ref->extraList.GetByType<RE::ExtraCount>());
	if (exCount)
	{
		DBG_VMESSAGE("Pick up %d instances of %s/0x%08x", exCount->count,
			m_ref->GetBaseObject()->GetName(), m_ref->GetBaseObject()->GetFormID());
		return exCount->count;
	}
	if (m_lootable)
		return 1;
	if (m_objectType == ObjectType::oreVein)
	{
		// limit ore harvesting to constrain Player Home mining
		return static_cast<SInt16>(INIFile::GetInstance()->GetInstance()->GetSetting(
			INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "maxMiningItems"));
	}
	return 1;
}
