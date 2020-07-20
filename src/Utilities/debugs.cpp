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

#include "CommonLibSSE/include/RE/RTTI.h"

#include "Utilities/utils.h"
#include "FormHelpers/FormHelper.h"
#include "Utilities/debugs.h"
#include "Looting/objects.h"
#include "Looting/LootableREFR.h"

namespace shse
{

void DumpKeywordForm(RE::BGSKeywordForm* keywordForm)
{
#if _DEBUG
	if (keywordForm)
	{
		for (UInt32 idx = 0; idx < keywordForm->numKeywords; ++idx)
		{
			std::optional<RE::BGSKeyword*> keyword(keywordForm->GetKeywordAt(idx));
			if (keyword)
				DBG_MESSAGE("Keyword %s/0x%08x", FormUtils::SafeGetFormEditorID(keyword.value()).c_str(), keyword.value()->formID);
		}
	}
#endif
}

void DumpKeyword(const RE::TESForm* pForm, const INIFile::SecondaryType scope)
{
#if _DEBUG
	if (!pForm)
		return;

	TESFormHelper pFormEx(pForm, scope);
	RE::BGSKeywordForm* keywordForm = pFormEx.GetKeywordForm();
	if (keywordForm)
		DumpKeywordForm(keywordForm);
#endif
}

void DumpExtraData(const RE::ExtraDataList* extraList)
{
#if _DEBUG
	if (extraList)
		return;

	int index(0);
	for (const RE::BSExtraData& extraData : *extraList)
	{
		DBG_MESSAGE("extraData:[0x%03x]", extraData.GetType());

		RE::NiPointer<RE::TESObjectREFR> targetRef;
		RE::RTTI::DumpTypeName(const_cast<RE::BSExtraData*>(&extraData));
		if (extraData.GetType() == RE::ExtraDataType::kCount)
			DBG_MESSAGE("kCount (%d)", ((RE::ExtraCount&)extraData).count);
		else if (extraData.GetType() == RE::ExtraDataType::kCharge)
			DBG_MESSAGE("kCharge %s (%0.2f)", ((RE::ExtraCharge&)extraData).charge);
		else if (extraData.GetType() == RE::ExtraDataType::kLocationRefType)
		{
			DBG_MESSAGE("kLocationRefType %s %s/0x%08x", ((RE::ExtraLocationRefType*)const_cast<RE::BSExtraData*>(&extraData))->locRefType->GetName(),
				((RE::ExtraLocationRefType*)const_cast<RE::BSExtraData*>(&extraData))->locRefType->formID);
			//DumpClass(extraData, sizeof(ExtraLocationRefType)/8);
		}
		else if (extraData.GetType() == RE::ExtraDataType::kOwnership)
			DBG_MESSAGE("kOwnership %s %s/0x%08x", reinterpret_cast<const RE::ExtraOwnership&>(const_cast<RE::BSExtraData&>(extraData)).owner->GetName(),
				reinterpret_cast<RE::ExtraOwnership*>(const_cast<RE::BSExtraData*>(&extraData))->owner->formID);
		else if (extraData.GetType() == RE::ExtraDataType::kAshPileRef)
		{
			/* TODO fix this up
			RE::ObjectRefHandle handle = extraData.GetAshPileRefHandle();
			if (RE::LookupReferenceByHandle(handle, targetRef))
				DBG_MESSAGE("%02x AshRef(Handle)=%08x(%d)", extraData.GetType(), targetRef->formID, handle);
			else
				DBG_MESSAGE("%02x AshRef(Handle)=?", extraData.GetType());
			*/
		}
		else if (extraData.GetType() == RE::ExtraDataType::kActivateRef)
		{
			RE::ExtraActivateRef* exActivateRef = static_cast<RE::ExtraActivateRef*>(const_cast<RE::BSExtraData*>(&extraData));
			/* TODO fix this up
			DumpClass(exActivateRef, sizeof(RE::ExtraActivateRef) / 8);
			*/
			DBG_MESSAGE("kActivateRef");
		}
		else if (extraData.GetType() == RE::ExtraDataType::kActivateRefChildren)
		{
			RE::ExtraActivateRefChildren* exActivateRefChain = static_cast<RE::ExtraActivateRefChildren*>(const_cast<RE::BSExtraData*>(&extraData));
			/* TODO fix this up
			DumpClass(exActivateRefChain, sizeof(RE::ExtraActivateRefChildren) / 8);
			DBG_MESSAGE("%02x (%08x)", extraData.GetType(), ((RE::ExtraActivateRefChildren&)extraData).data->unk3->formID);
			*/
		}
		else if (extraData.GetType() == RE::ExtraDataType::kLinkedRef)
		{
			RE::ExtraLinkedRef* exLinkRef = static_cast<RE::ExtraLinkedRef*>(const_cast<RE::BSExtraData*>(&extraData));
			if (!exLinkRef)
				DBG_MESSAGE("kLinkedRef ERR?????");
			else
			{
				UInt32 length = exLinkRef->linkedRefs.size();
				for (auto pair : exLinkRef->linkedRefs)
				{
					if (!pair.refr)
						continue;

					if (!pair.keyword)
						DBG_MESSAGE("kLinkedRef NULL/0x%08x) #%d/%d", pair.refr->formID, (index + 1), length);
					else
						DBG_MESSAGE("kLinkedRef %s/0x%08x #%d/%d", (pair.keyword)->GetName(), pair.refr->formID, (index + 1), length);
				}
			}
		}
		else
			DBG_MESSAGE("extraData type 0x%02x", extraData.GetType());
		++index;
	}
#endif
}

void DumpItemVW(const TESFormHelper& itemEx)
{
#if _DEBUG
	double worth(itemEx.GetWorth());
	double weight(itemEx.GetWeight());

	double vw = (worth > 0. && weight > 0.) ? worth / weight : 0.;
	DBG_MESSAGE("Worth(%0.2f)  Weight(%0.2f)  V/W(%0.2f)", worth, weight, vw);
#endif
}

void DumpContainer(const LootableREFR& refr)
{
#if _DEBUG
	RE::TESContainer *container = const_cast<RE::TESObjectREFR*>(refr.GetReference())->GetContainer();
	if (container)
	{
		DBG_MESSAGE("CONT %08x %02x(%02d) [%s]", refr.GetReference()->GetBaseObject()->formID, refr.GetReference()->GetBaseObject()->formType,
			refr.GetReference()->GetBaseObject()->formType, refr.GetReference()->GetName());

		container->ForEachContainerObject([&](RE::ContainerObject* entry) -> bool {
			TESFormHelper itemEx(entry->obj, refr.Scope());

			if (itemEx.Form()->formType == RE::FormType::LeveledItem)
			{
				DBG_MESSAGE("%d:%08x LeveledItem", itemEx.Form()->formType, itemEx.Form()->formID);
			}
			else
			{
				bool bPlayable = IsPlayable(itemEx.Form());
				const RE::TESFullName* name = itemEx.Form()->As<RE::TESFullName>();
				std::string typeName = GetObjectTypeName(GetBaseFormObjectType(itemEx.Form()));

				DBG_MESSAGE("%d:%08x [%s] count=%d playable=%d - %s", itemEx.Form()->formType, itemEx.Form()->formID, name->GetFullName(), entry->count, bPlayable, typeName.c_str());

				TESFormHelper refHelper(refr.GetReference()->GetBaseObject(), refr.Scope());
				DumpItemVW(refHelper);

				DumpKeyword(refHelper.Form(), refr.Scope());
			}
			return true;
		});
	}
	else
	{
		DBG_MESSAGE("Not CONT %08x %02x(%02d) [%s]", refr.GetReference()->GetBaseObject()->formID, refr.GetReference()->GetBaseObject()->formType,
			refr.GetReference()->GetBaseObject()->formType, refr.GetReference()->GetName());
	}

	const RE::ExtraContainerChanges* exChanges = refr.GetReference()->extraList.GetByType<RE::ExtraContainerChanges>();
	if (exChanges && exChanges->changes->entryList)
	{
		for (const RE::InventoryEntryData* entryData : *exChanges->changes->entryList)
		{
			TESFormHelper itemEx(const_cast<RE::InventoryEntryData*>(entryData)->GetObject(), refr.Scope());

			bool bPlayable = IsPlayable(itemEx.Form());
			std::string typeName = GetObjectTypeName(GetBaseFormObjectType(itemEx.Form()));
			const RE::TESFullName *name = itemEx.Form()->As<RE::TESFullName>();
			DBG_MESSAGE("ExtraContainerChanges %08x [%s] %p count=%d playable=%d  - %s", itemEx.Form()->formID, name->GetFullName(), entryData, entryData->countDelta, bPlayable, typeName.c_str());

			DumpItemVW(itemEx);
			DumpKeyword(itemEx.Form(), refr.Scope());

			if (!entryData->extraLists)
			{
				continue;
			}

			for (RE::ExtraDataList* extraList : *entryData->extraLists)
			{
				if (!extraList)
					continue;

				DumpExtraData(extraList);
			}
		}
	}

	const RE::ExtraDataList *extraData = &refr.GetReference()->extraList;
    DumpExtraData(extraData);
#endif
}

}