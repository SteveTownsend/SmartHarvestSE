#include "PrecompiledHeaders.h"

#include "CommonLibSSE/include/RE/RTTI.h"

#include "utils.h"

#include "debugs.h"

void DumpKeywordForm(RE::BGSKeywordForm* keywordForm)
{
#if _DEBUG
	if (keywordForm)
	{
		DBG_MESSAGE("keywordForm - %p", keywordForm);

		for (UInt32 idx = 0; idx < keywordForm->numKeywords; ++idx)
		{
			std::optional<RE::BGSKeyword*> keyword(keywordForm->GetKeywordAt(idx));
			if (keyword)
				DBG_MESSAGE("%s (%08x)", keyword.value()->GetFormEditorID(), keyword.value()->formID);
		}
	}
#endif
}

void DumpKeyword(const RE::TESForm* pForm)
{
#if _DEBUG
	if (!pForm)
		return;

	TESFormHelper pFormEx(pForm);
	RE::BGSKeywordForm* keywordForm = pFormEx.GetKeywordForm();
	if (keywordForm)
		DumpKeywordForm(keywordForm);
#endif
}

void DumpExtraData(const RE::ExtraDataList* extraList)
{
#if _DEBUG
	DBG_MESSAGE("extraData");
	if (extraList)
		return;

	int index(0);

	for (const RE::BSExtraData& extraData : *extraList)
	{
		DBG_MESSAGE("Check:[%03X]", extraData.GetType());

		RE::NiPointer<RE::TESObjectREFR> targetRef;
		RE::RTTI::DumpTypeName(const_cast<RE::BSExtraData*>(&extraData));
		if (extraData.GetType() == RE::ExtraDataType::kCount)
			DBG_MESSAGE("%02x (%d)", extraData.GetType(), ((RE::ExtraCount&)extraData).count);
		else if (extraData.GetType() == RE::ExtraDataType::kCharge)
			DBG_MESSAGE("%02x %s (%0.2f)", extraData.GetType(), ((RE::ExtraCharge&)extraData).charge);
		else if (extraData.GetType() == RE::ExtraDataType::kLocationRefType)
		{
			DBG_MESSAGE("%02x %s ([%s] %08x)", extraData.GetType(), ((RE::ExtraLocationRefType*)const_cast<RE::BSExtraData*>(&extraData))->locRefType->GetName(),
				((RE::ExtraLocationRefType*)const_cast<RE::BSExtraData*>(&extraData))->locRefType->formID);
			//DumpClass(extraData, sizeof(ExtraLocationRefType)/8);
		}
		else if (extraData.GetType() == RE::ExtraDataType::kOwnership)
			DBG_MESSAGE("%02x %s ([%s] %08x)", extraData.GetType(), reinterpret_cast<const RE::ExtraOwnership&>(const_cast<RE::BSExtraData&>(extraData)).owner->GetName(),
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
			DBG_MESSAGE("%02x", extraData.GetType());
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
				DBG_MESSAGE("%02x ERR?????", extraData.GetType());
			else
			{
				UInt32 length = exLinkRef->linkedRefs.size();
				for (auto pair : exLinkRef->linkedRefs)
				{
					if (!pair.refr)
						continue;

					if (!pair.keyword)
						DBG_MESSAGE("%02x ([NULL] %08x) %d / %d", extraData.GetType(), pair.refr->formID, (index + 1), length);
					else
						DBG_MESSAGE("%02x ([%s] %08x) %d / %d", extraData.GetType(), (pair.keyword)->GetName(), pair.refr->formID, (index + 1), length);
				}
			}
		}
		else
			DBG_MESSAGE("%02x", extraData.GetType());
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

void DumpReference(const TESObjectREFRHelper& refr, const char* typeName)
{
#if _DEBUG
	const RE::TESForm* target(refr.GetLootable() ? refr.GetLootable() : refr.GetReference()->GetBaseObject());
	DBG_VMESSAGE("0x%08x 0x%02x(%02d) [%s] - typename %s", target->formID, target->formType, target->formType, refr.GetReference()->GetName(), typeName);

	TESFormHelper itemEx(target);
	DumpItemVW(itemEx);

	DumpKeyword(target);

	const RE::ExtraDataList *extraData = &refr.GetReference()->extraList;
	DumpExtraData(extraData);

	DBG_MESSAGE("--------------------");
#endif
}

void DumpContainer(const TESObjectREFRHelper& refr)
{
#if _DEBUG
	DBG_MESSAGE("%08x %02x(%02d) [%s]", refr.GetReference()->GetBaseObject()->formID, refr.GetReference()->GetBaseObject()->formType,
		refr.GetReference()->GetBaseObject()->formType, refr.GetReference()->GetName());

	DBG_MESSAGE("RE::TESContainer--");

	RE::TESContainer *container = const_cast<RE::TESObjectREFR*>(refr.GetReference())->GetContainer();
	if (container)
	{
		container->ForEachContainerObject([&](RE::ContainerObject* entry) -> bool {
			TESFormHelper itemEx(entry->obj);

			DBG_MESSAGE("itemType:: %d:(%08x)", itemEx.m_form->formType, itemEx.m_form->formID);

			if (itemEx.m_form->formType == RE::FormType::LeveledItem)
			{
				DBG_MESSAGE("%08x LeveledItem", itemEx.m_form->formID);
			}
			else
			{
				bool bPlayable = IsPlayable(itemEx.m_form);
				const RE::TESFullName* name = itemEx.m_form->As<RE::TESFullName>();
				std::string typeName = GetObjectTypeName(GetBaseFormObjectType(itemEx.m_form));

				DBG_MESSAGE("%08x [%s] count=%d playable=%d  - %s", itemEx.m_form->formID, name->GetFullName(), entry->count, bPlayable, typeName.c_str());

				TESFormHelper refHelper(refr.GetReference()->GetBaseObject());
				DumpItemVW(refHelper);

				DumpKeyword(refHelper.m_form);
			}
			return true;
		});
	}

	DBG_MESSAGE("ExtraContainerChanges--");

	const RE::ExtraContainerChanges* exChanges = refr.GetReference()->extraList.GetByType<RE::ExtraContainerChanges>();
	if (exChanges && exChanges->changes->entryList)
	{
		for (const RE::InventoryEntryData* entryData : *exChanges->changes->entryList)
		{
			TESFormHelper itemEx(const_cast<RE::InventoryEntryData*>(entryData)->GetObject());

			bool bPlayable = IsPlayable(itemEx.m_form);
			std::string typeName = GetObjectTypeName(GetBaseFormObjectType(itemEx.m_form));
			const RE::TESFullName *name = itemEx.m_form->As<RE::TESFullName>();
			DBG_MESSAGE("- %08x [%s] %p count=%d playable=%d  - %s", itemEx.m_form->formID, name->GetFullName(), entryData, entryData->countDelta, bPlayable, typeName.c_str());

			DumpItemVW(itemEx);
			DumpKeyword(itemEx.m_form);

			if (!entryData->extraLists)
			{
				DBG_MESSAGE("extraLists - not found");
				continue;
			}

			for (RE::ExtraDataList* extraList : *entryData->extraLists)
			{
				if (!extraList)
					continue;

				DBG_MESSAGE("extraList - %p", extraList);
				DumpExtraData(extraList);
			}
		}
		DBG_MESSAGE("*");
	}

	DBG_MESSAGE("ExtraDatas--");

	const RE::ExtraDataList *extraData = &refr.GetReference()->extraList;
    DumpExtraData(extraData);
	DBG_MESSAGE("--------------------");
#endif
}
