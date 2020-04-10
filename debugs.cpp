#include "PrecompiledHeaders.h"

#include "skse64_common/Utilities.h"
#include "CommonLibSSE/include/RE/ExtraLocationRefType.h"
#include "CommonLibSSE/include/RE/BGSKeywordForm.h"
#include "utils.h"
#include "objects.h"
#include "dataCase.h"
#include "TESFormHelper.h"

#include "debugs.h"

void DumpKeywordForm(RE::BGSKeywordForm* keywordForm)
{
#if _DEBUG
	if (keywordForm)
	{
		_MESSAGE("keywordForm - %p", keywordForm);

		for (UInt32 idx = 0; idx < keywordForm->numKeywords; ++idx)
		{
			std::optional<RE::BGSKeyword*> keyword(keywordForm->GetKeywordAt(idx));
			if (keyword)
				_MESSAGE("%s (%08x)", keyword.value()->GetFormEditorID(), keyword.value()->formID);
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
	_MESSAGE("extraData");
	if (extraList)
		return;

	int index(0);

	for (const RE::BSExtraData& extraData : *extraList)
	{
		_MESSAGE("Check:[%03X]", extraData.GetType());

		RE::NiPointer<RE::TESObjectREFR> targetRef;
		const char *className = GetObjectClassName(&const_cast<RE::BSExtraData&>(extraData));
		if (extraData.GetType() == RE::ExtraDataType::kCount)
			_MESSAGE("%02x %s (%d)", extraData.GetType(), className, ((RE::ExtraCount&)extraData).count);
		else if (extraData.GetType() == RE::ExtraDataType::kCharge)
			_MESSAGE("%02x %s (%0.2f)", extraData.GetType(), className, ((RE::ExtraCharge&)extraData).charge);
		else if (extraData.GetType() == RE::ExtraDataType::kLocationRefType)
		{
			_MESSAGE("%02x %s ([%s] %08x)", extraData.GetType(), className, ((RE::ExtraLocationRefType*)const_cast<RE::BSExtraData*>(&extraData))->locRefType->GetName(),
				((RE::ExtraLocationRefType*)const_cast<RE::BSExtraData*>(&extraData))->locRefType->formID);
			//DumpClass(extraData, sizeof(ExtraLocationRefType)/8);
		}
		else if (extraData.GetType() == RE::ExtraDataType::kOwnership)
			_MESSAGE("%02x %s ([%s] %08x)", extraData.GetType(), className, reinterpret_cast<const RE::ExtraOwnership&>(const_cast<RE::BSExtraData&>(extraData)).owner->GetName(),
				reinterpret_cast<RE::ExtraOwnership*>(const_cast<RE::BSExtraData*>(&extraData))->owner->formID);
		else if (extraData.GetType() == RE::ExtraDataType::kAshPileRef)
		{
			/* TODO fix this up
			RE::ObjectRefHandle handle = extraData.GetAshPileRefHandle();
			if (RE::LookupReferenceByHandle(handle, targetRef))
				_MESSAGE("%02x %s AshRef(Handle)=%08x(%d)", extraData.GetType(), className, targetRef->formID, handle);
			else
				_MESSAGE("%02x %s AshRef(Handle)=?", extraData.GetType(), className);
			*/
		}
		else if (extraData.GetType() == RE::ExtraDataType::kActivateRef)
		{
			RE::ExtraActivateRef* exActivateRef = static_cast<RE::ExtraActivateRef*>(const_cast<RE::BSExtraData*>(&extraData));
			DumpClass(exActivateRef, sizeof(RE::ExtraActivateRef) / 8);

			_MESSAGE("%02x %s", extraData.GetType(), className);
		}
		else if (extraData.GetType() == RE::ExtraDataType::kActivateRefChildren)
		{
			RE::ExtraActivateRefChildren* exActivateRefChain = static_cast<RE::ExtraActivateRefChildren*>(const_cast<RE::BSExtraData*>(&extraData));
			DumpClass(exActivateRefChain, sizeof(RE::ExtraActivateRefChildren) / 8);
			/* TODO fix this up
			_MESSAGE("%02x %s (%08X)", extraData.GetType(), className, ((RE::ExtraActivateRefChildren&)extraData).data->unk3->formID);
			*/
		}
		else if (extraData.GetType() == RE::ExtraDataType::kLinkedRef)
		{
			RE::ExtraLinkedRef* exLinkRef = static_cast<RE::ExtraLinkedRef*>(const_cast<RE::BSExtraData*>(&extraData));
			if (!exLinkRef)
				_MESSAGE("%02x %s  ERR?????", extraData.GetType(), className);
			else
			{
				UInt32 length = exLinkRef->linkedRefs.size();
				for (auto pair : exLinkRef->linkedRefs)
				{
					if (!pair.refr)
						continue;

					if (!pair.keyword)
						_MESSAGE("%02x %s ([NULL] %08x) %d / %d", extraData.GetType(), className, pair.refr->formID, (index + 1), length);
					else
						_MESSAGE("%02x %s ([%s] %08x) %d / %d", extraData.GetType(), className, (pair.keyword)->GetName(), pair.refr->formID, (index + 1), length);
				}
			}
		}
		else
			_MESSAGE("%02x %s", extraData.GetType(), className);
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
	_MESSAGE("Worth(%0.2f)  Weight(%0.2f)  V/W(%0.2f)", worth, weight, vw);
#endif
}

void DumpReference(const TESObjectREFRHelper& refr, const char* typeName)
{
#if _DEBUG
	const RE::TESForm* target(refr.GetLootable() ? refr.GetLootable() : refr.m_ref->data.objectReference);
	_MESSAGE("0x%08x 0x%02x(%02d) [%s] - %s", target->formID, target->formType, target->formType, refr.m_ref->GetName(), typeName);

	TESFormHelper itemEx(target);
	DumpItemVW(itemEx);

	DumpKeyword(refr.m_ref->data.objectReference);

	const RE::ExtraDataList *extraData = &refr.m_ref->extraList;
	DumpExtraData(extraData);

	_MESSAGE("--------------------\n");
#endif
}

void DumpContainer(const TESObjectREFRHelper& refr)
{
#if _DEBUG
	_MESSAGE("%08x %02x(%02d) [%s]", refr.m_ref->data.objectReference->formID, refr.m_ref->data.objectReference->formType,
		refr.m_ref->data.objectReference->formType, refr.m_ref->GetName());

	_MESSAGE("RE::TESContainer--");

	RE::TESContainer *container = const_cast<RE::TESObjectREFR*>(refr.m_ref)->GetContainer();
	if (container)
	{
		container->ForEachContainerObject([&](RE::ContainerObject* entry) -> bool {
			TESFormHelper itemEx(entry->obj);

			_MESSAGE("itemType:: %d:(%08x)", itemEx.m_form->formType, itemEx.m_form->formID);

			if (itemEx.m_form->formType == RE::FormType::LeveledItem)
			{
				_MESSAGE("%08x LeveledItem", itemEx.m_form->formID);
			}
			else
			{
				bool bPlayable = IsPlayable(itemEx.m_form);
				RE::TESFullName* name = skyrim_cast<RE::TESFullName*, RE::TESForm>(itemEx.m_form);
				std::string typeName = GetObjectTypeName(ClassifyType(itemEx.m_form));

				_MESSAGE("%08x [%s] count=%d playable=%d  - %s", itemEx.m_form->formID, name->GetFullName(), entry->count, bPlayable, typeName.c_str());

				TESFormHelper refHelper(refr.m_ref->data.objectReference);
				DumpItemVW(refHelper);

				DumpKeyword(refHelper.m_form);
			}
			return true;
		});
	}

	_MESSAGE("ExtraContainerChanges--");

	const RE::ExtraContainerChanges* exChanges = refr.m_ref->extraList.GetByType<RE::ExtraContainerChanges>();
	if (exChanges && exChanges->changes->entryList)
	{
		for (const RE::InventoryEntryData* entryData : *exChanges->changes->entryList)
		{
			TESFormHelper itemEx(const_cast<RE::InventoryEntryData*>(entryData)->GetObject());

			bool bPlayable = IsPlayable(itemEx.m_form);
			std::string typeName = GetObjectTypeName(ClassifyType(itemEx.m_form));
			RE::TESFullName *name = skyrim_cast<RE::TESFullName*, RE::TESForm>(itemEx.m_form);
			_MESSAGE("- %08x [%s] %p count=%d playable=%d  - %s", itemEx.m_form->formID, name->GetFullName(), entryData, entryData->countDelta, bPlayable, typeName.c_str());

			DumpItemVW(itemEx);
			DumpKeyword(itemEx.m_form);

			if (!entryData->extraLists)
			{
				_MESSAGE("extraLists - not found");
				continue;
			}

			for (RE::ExtraDataList* extraList : *entryData->extraLists)
			{
				if (!extraList)
					continue;

				_MESSAGE("extraList - %p", extraList);
				DumpExtraData(extraList);
			}
		}
		_MESSAGE("*");
	}

	_MESSAGE("ExtraDatas--");

	const RE::ExtraDataList *extraData = &refr.m_ref->extraList;
    DumpExtraData(extraData);
	_MESSAGE("--------------------\n");
#endif
}
