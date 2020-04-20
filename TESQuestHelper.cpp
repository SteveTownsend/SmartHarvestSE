#include "PrecompiledHeaders.h"

#include "TESQuestHelper.h"
#include "utils.h"
#include "CommonLibSSE/include/RE/BGSBaseAlias.h"
#include "CommonLibSSE/include/RE/BGSRefAlias.h"
#include "CommonLibSSE/include/RE/BSScript/IObjectHandlePolicy.h"

RE::TESQuest* GetTargetQuest(const char* espName, UInt32 questID)
{
	static RE::TESQuest* result = nullptr;
#if _DEBUG
	static bool listed(false);
#endif
	if (!result)
	{
		UInt32 formID = 0;
		std::optional<UInt8> idx = RE::TESDataHandler::GetSingleton()->GetLoadedModIndex(espName);
		if (idx.has_value())
		{
			formID = (idx.value() << 24) | questID;
#if _DEBUG
			_DMESSAGE("Got formID for questID %08.2x", questID);
#endif
		}
#if _DEBUG
		else if (!listed)
		{
			for (const auto& nextFile : RE::TESDataHandler::GetSingleton()->compiledFileCollection.files)
			{
				_DMESSAGE("Mod loaded %s", &nextFile->fileName);
			}
			listed = true;
		}
#endif
		if (formID != 0)
		{
			RE::TESForm* questForm = RE::TESForm::LookupByID(formID);
#if _DEBUG
			_DMESSAGE("Got Base Form %s", questForm ? questForm->GetFormEditorID() : "nullptr");
#endif
			result = questForm ? questForm->As<RE::TESQuest>() : nullptr;
#if _DEBUG
			_DMESSAGE("Got Quest Form %s", questForm->As<RE::TESQuest>() ? questForm->GetFormEditorID() : "nullptr");
#endif
		}
	}
	return result;
}

RE::VMHandle TESQuestHelper::GetAliasHandle(UInt32 index)
{
	static RE::VMHandle result(0);
	if (result == 0)
	{
		if (m_quest && m_quest->IsRunning())
		{
#if _DEBUG
			_DMESSAGE("Quest %s is running", m_quest->GetFormEditorID());
#endif
			RE::BGSBaseAlias* baseAlias(m_quest->aliases[index]);
			if (!baseAlias)
			{
#if _DEBUG
				_DMESSAGE("Quest has no alias at index %d", index);
#endif
				return 0;
			}

			RE::BGSRefAlias* refAlias = static_cast<RE::BGSRefAlias*>(baseAlias);
			if (!refAlias)
			{
#if _DEBUG
				_DMESSAGE("Quest is not type BGSRefAlias");
#endif
				return 0;
			}
#if _DEBUG
			_MESSAGE("Got BGSRefAlias");
#endif

			RE::SkyrimScript::HandlePolicy& policy(RE::SkyrimVM::GetSingleton()->handlePolicy);
			result = policy.GetHandleForObject(RefAliasID, refAlias);
#if _DEBUG
			_MESSAGE("Got Alias Handle %d", result);
#endif
		}
	}
	return result;
}
