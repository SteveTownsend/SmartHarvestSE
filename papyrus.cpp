#include "PrecompiledHeaders.h"

#include "CommonLibSSE/include/RE/BSScript/TypeTraits.h"
#include "CommonLibSSE/include/RE/BSScript/VMArray.h"
#include "CommonLibSSE/include/RE/BSScript/NativeFunction.h"
#include "papyrus.h"

#include "TESFormHelper.h"
#include "tasks.h"
#include "iniSettings.h"
#include "basketfile.h"
#include "objects.h"
#include "dataCase.h"
#include <list>

namespace
{
	std::string GetPluginName(RE::TESForm* thisForm)
	{
		std::string result;
		UInt8 loadOrder = (thisForm->formID) >> 24;
		if (loadOrder < 0xFF)
		{
			RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
			const RE::TESFile* modInfo = dhnd->LookupLoadedModByIndex(loadOrder);
			if (modInfo)
				result = std::string(&modInfo->fileName[0]);
		}
		return result;
	}

	void ToLower(std::string &str)
	{
		for (auto &c : str)
			c = tolower(c);
	}
	void ToUpper(std::string &str)
	{
		for (auto &c : str)
			c = toupper(c);
	}

	std::string ToStringID(UInt32 id)
	{
		std::stringstream ss;
		ss << std::hex << std::setfill('0') << std::setw(8) << std::uppercase << id;
		return ss.str();
	}
}

bool Replace(std::string& str, const std::string& target, const std::string& replacement)
{
	if (str.empty() || target.empty())
		return false;

	bool result = false;
	std::string::size_type pos = 0;
	while ((pos = str.find(target, pos)) != std::string::npos)
	{
		if (!result)
			result = true;
		str.replace(pos, target.length(), replacement);
		pos += replacement.length();
	}
	return result;
}

namespace papyrus
{
	void DebugTrace(RE::StaticFunctionTag* base, RE::BSFixedString str)
	{
#if _DEBUG
		_MESSAGE("%s", str);
#endif
	}

	RE::BSFixedString GetPluginName(RE::StaticFunctionTag* base, RE::TESForm* thisForm)
	{
		if (!thisForm)
			return nullptr;
		return ::GetPluginName(thisForm).c_str();
	}

	RE::BSFixedString GetTextFormID(RE::StaticFunctionTag* base, RE::TESForm* thisForm)
	{
		if (!thisForm)
			return nullptr;
		return ::ToStringID(thisForm->formID).c_str();
	}

	RE::BSFixedString GetTextObjectType(RE::StaticFunctionTag* base, RE::TESForm* thisForm)
	{
		if (!thisForm)
			return nullptr;

		ObjectType objType = ClassifyType(thisForm, true);
		if (objType == ObjectType::unknown)
			return "NON-CLASSIFIED";

		std::string result = GetObjectTypeName(objType);
		::ToUpper(result);
		return (!result.empty()) ? result.c_str() : nullptr;
	}

	RE::BSFixedString GetObjectKeyString(RE::StaticFunctionTag* base, SInt32 objectNumber)
	{
		RE::BSFixedString result;
		std::string str = GetObjectTypeName(objectNumber);
		if (str.empty() || str.c_str() == "unknown")
			return result;
		else
			return str.c_str();
	}

	float GetSetting(RE::StaticFunctionTag* base, SInt32 section_first, SInt32 section_second, RE::BSFixedString key)
	{
		INIFile::PrimaryType first = static_cast<INIFile::PrimaryType>(section_first);
		INIFile::SecondaryType second = static_cast<INIFile::SecondaryType>(section_second);

		INIFile* ini = INIFile::GetInstance()->GetInstance();
		if (!ini || !ini->IsType(first) || !ini->IsType(second))
			return 0.0;

		std::string str = key.c_str();
		::ToLower(str);

		return static_cast<float>(ini->GetSetting(first, second, str.c_str()));
	}

	void GetSettingToObjectArray(RE::StaticFunctionTag* base, SInt32 section_first, SInt32 section_second, RE::BSScript::VMArray<float> value_arr)
	{
		INIFile::PrimaryType first = static_cast<INIFile::PrimaryType>(section_first);
		INIFile::SecondaryType second = static_cast<INIFile::SecondaryType>(section_second);

		INIFile* ini = INIFile::GetInstance()->GetInstance();
		if (!ini || !ini->IsType(first) || !ini->IsType(second))
			return;

		SInt32 index(0);
		for (auto & nextValue : value_arr)
		{
			std::string key = GetObjectTypeName(index);
			::ToLower(key);
			float tmp_value = static_cast<float>(ini->GetSetting(first, second, key.c_str()));
			nextValue = tmp_value;
			++index;
		}
	}

	void PutSetting(RE::StaticFunctionTag* base, SInt32 section_first, SInt32 section_second, RE::BSFixedString key, float value)
	{
		INIFile::PrimaryType first = static_cast<INIFile::PrimaryType>(section_first);
		INIFile::SecondaryType second = static_cast<INIFile::SecondaryType>(section_second);

		INIFile* ini = INIFile::GetInstance()->GetInstance();
		if (!ini || !ini->IsType(first) || !ini->IsType(second))
			return;

		std::string str = key.c_str();
		::ToLower(str);

		ini->PutSetting(first, second, str.c_str(), static_cast<double>(value));
	}

	void PutSettingObjectArray(RE::StaticFunctionTag* base, SInt32 section_first, SInt32 section_second, RE::BSScript::VMArray<float> value_arr)
	{
		INIFile::PrimaryType first = static_cast<INIFile::PrimaryType>(section_first);
		INIFile::SecondaryType second = static_cast<INIFile::SecondaryType>(section_second);

		INIFile* ini = INIFile::GetInstance()->GetInstance();
		if (!ini || !ini->IsType(first) || !ini->IsType(second))
			return;

		SInt32 index(0);
		for (float tmp_value : value_arr)
		{
			std::string key = GetObjectTypeName(index);
			::ToLower(key);
			ini->PutSetting(first, second, key.c_str(), static_cast<double>(tmp_value));
			++index;
		}
	}

	bool Reconfigure(RE::StaticFunctionTag* base)
	{
		INIFile* ini = INIFile::GetInstance();
		if (ini)
		{
			ini->Free();
			return true;
		}
		return false;
	}

	void LoadIniFile(RE::StaticFunctionTag* base)
	{
		INIFile* ini = INIFile::GetInstance();
		if (!ini || !ini->LoadFile())
		{
#if _DEBUG
			_MESSAGE("LoadFile error");
#endif
		}
	}

	void SaveIniFile(RE::StaticFunctionTag* base)
	{
		INIFile::GetInstance()->SaveFile();
	}

	void SetIngredientForCritter(RE::StaticFunctionTag* base, RE::TESForm* critter, RE::TESForm* ingredient)
	{
		DataCase::GetInstance()->SetIngredientForCritter(critter, ingredient);
	}

	SInt32 GetCloseReferences(RE::StaticFunctionTag* base, SInt32 type1)
	{
		return SearchTask::StartSearch(type1);
	}

	void AllowSearch(RE::StaticFunctionTag* base)
	{
#if _DEBUG
		_MESSAGE("Reference Search enabled");
#endif
		SearchTask::Allow();
	}
	void DisallowSearch(RE::StaticFunctionTag* base)
	{
#if _DEBUG
		_MESSAGE("Reference Search disabled");
#endif
		SearchTask::Disallow();
	}
	bool IsSearchAllowed(RE::StaticFunctionTag* base)
	{
		return SearchTask::IsAllowed();
	}

	constexpr int LocationTypeUser = 1;
	constexpr int LocationTypeExcluded = 2;

	void AddLocationToList(RE::StaticFunctionTag* base, const int locationType, const RE::TESForm* location)
	{
		if (locationType == LocationTypeExcluded)
		{
			SearchTask::AddLocationToExcludeList(location);
		}
	}
	void DropLocationFromList(RE::StaticFunctionTag* base, const int locationType, const RE::TESForm* location)
	{
		if (locationType == LocationTypeExcluded)
		{
			SearchTask::DropLocationFromExcludeList(location);
		}
	}

	bool UnlockAutoHarvest(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr)
	{
		return SearchTask::UnlockAutoHarvest(refr);
	}

	bool UnlockPossiblePlayerHouse(RE::StaticFunctionTag* base, RE::TESForm* location)
	{
		return SearchTask::UnlockPossiblePlayerHouse(location->As<RE::BGSLocation>());
	}

	bool BlockReference(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr)
	{
		return DataCase::GetInstance()->BlockReference(refr);
	}

	void UnblockEverything(RE::StaticFunctionTag* base)
	{
		DataCase::GetInstance()->ListsClear();
		SearchTask::UnlockAll();
	}
	void SyncUserlist(RE::StaticFunctionTag* base)
	{
		BasketFile::GetSingleton()->SyncList(BasketFile::USERLIST);
	}
	bool SaveUserlist(RE::StaticFunctionTag* base)
	{
		return BasketFile::GetSingleton()->SaveFile(BasketFile::USERLIST, "userlist.tsv");
	}
	bool LoadUserlist(RE::StaticFunctionTag* base)
	{
		return BasketFile::GetSingleton()->LoadFile(BasketFile::USERLIST, "userlist.tsv");
	}

	void SyncExcludelist(RE::StaticFunctionTag* base)
	{
		BasketFile::GetSingleton()->SyncList(BasketFile::EXCLUDELIST);
	}
	bool SaveExcludelist(RE::StaticFunctionTag* base)
	{
		return BasketFile::GetSingleton()->SaveFile(BasketFile::EXCLUDELIST, "excludelist.tsv");
	}
	bool LoadExcludelist(RE::StaticFunctionTag* base)
	{
		return BasketFile::GetSingleton()->LoadFile(BasketFile::EXCLUDELIST, "excludelist.tsv");
	}

	RE::BSFixedString GetTranslation(RE::StaticFunctionTag* base, RE::BSFixedString key)
	{
		std::string str = key.c_str();
		DataCase* data = DataCase::GetInstance();
		if (data->lists.translations.count(str) == 0)
			return nullptr;
		return data->lists.translations[str].c_str();
	}

	RE::BSFixedString Replace(RE::StaticFunctionTag* base, RE::BSFixedString str, RE::BSFixedString target, RE::BSFixedString replacement)
	{
		std::string s_str(str.c_str());
		std::string s_target(target.c_str());
		std::string s_replacement(replacement.c_str());
		return (StringUtils::Replace(s_str, s_target, s_replacement)) ? s_str.c_str() : nullptr;
	}

	RE::BSFixedString ReplaceArray(RE::StaticFunctionTag* base, RE::BSFixedString str, RE::BSScript::VMArray<RE::BSFixedString> targets, RE::BSScript::VMArray<RE::BSFixedString> replacements)
	{
		std::string result(str.c_str());
		if (result.empty() || targets.size() != replacements.size())
			return nullptr;

		RE::BSFixedString target;
		RE::BSFixedString replacement;
		using StringList = RE::BSScript::VMArray<RE::BSFixedString>;
		for (StringList::iterator target = targets.begin(), replacement = replacements.begin(); target != targets.end(); ++target, ++replacement)
		{
			RE::BSFixedString oldStr(*target);
			RE::BSFixedString newStr(*replacement);
			std::string s_target(oldStr.c_str());
			std::string s_replacement(newStr.c_str());

			if (!StringUtils::Replace(result, s_target, s_replacement))
				return nullptr;
		}
		return result.c_str();
	}
}

bool papyrus::RegisterFuncs(RE::BSScript::Internal::VirtualMachine* a_vm)
{
	papyrus::RegisterFunction(a_vm, "DebugTrace", AHSE_NAME, papyrus::DebugTrace);
	papyrus::RegisterFunction(a_vm, "GetPluginName", AHSE_NAME, papyrus::GetPluginName);
	papyrus::RegisterFunction(a_vm, "GetTextFormID", AHSE_NAME, papyrus::GetTextFormID);
	papyrus::RegisterFunction(a_vm, "GetTextObjectType", AHSE_NAME, papyrus::GetTextObjectType);

	papyrus::RegisterFunction(a_vm, "GetCloseReferences", AHSE_NAME, papyrus::GetCloseReferences);
	papyrus::RegisterFunction(a_vm, "UnlockAutoHarvest", AHSE_NAME, papyrus::UnlockAutoHarvest);
	papyrus::RegisterFunction(a_vm, "UnlockPossiblePlayerHouse", AHSE_NAME, papyrus::UnlockPossiblePlayerHouse);
	papyrus::RegisterFunction(a_vm, "BlockReference", AHSE_NAME, papyrus::BlockReference);
	papyrus::RegisterFunction(a_vm, "UnblockEverything", AHSE_NAME, papyrus::UnblockEverything);
#if 0
	papyrus::RegisterFunction(a_vm, "PlayPickupSound", AHSE_NAME, papyrus::PlayPickupSound);
#endif	
	papyrus::RegisterFunction(a_vm, "GetSetting", AHSE_NAME, papyrus::GetSetting);
	papyrus::RegisterFunction(a_vm, "GetSettingToObjectArray", AHSE_NAME, papyrus::GetSettingToObjectArray);
	papyrus::RegisterFunction(a_vm, "PutSetting", AHSE_NAME, papyrus::PutSetting);
	papyrus::RegisterFunction(a_vm, "PutSettingObjectArray", AHSE_NAME, papyrus::PutSettingObjectArray);

	papyrus::RegisterFunction(a_vm, "GetObjectKeyString", AHSE_NAME, papyrus::GetObjectKeyString);

	papyrus::RegisterFunction(a_vm, "Reconfigure", AHSE_NAME, papyrus::Reconfigure);
	papyrus::RegisterFunction(a_vm, "LoadIniFile", AHSE_NAME, papyrus::LoadIniFile);
	papyrus::RegisterFunction(a_vm, "SaveIniFile", AHSE_NAME, papyrus::SaveIniFile);

	papyrus::RegisterFunction(a_vm, "SetIngredientForCritter", AHSE_NAME, papyrus::SetIngredientForCritter);

	papyrus::RegisterFunction(a_vm, "SyncUserlist", AHSE_NAME, papyrus::SyncUserlist);
	papyrus::RegisterFunction(a_vm, "SaveUserlist", AHSE_NAME, papyrus::SaveUserlist);
	papyrus::RegisterFunction(a_vm, "LoadUserlist", AHSE_NAME, papyrus::LoadUserlist);

	papyrus::RegisterFunction(a_vm, "SyncExcludelist", AHSE_NAME, papyrus::SyncExcludelist);
	papyrus::RegisterFunction(a_vm, "SaveExcludelist", AHSE_NAME, papyrus::SaveExcludelist);
	papyrus::RegisterFunction(a_vm, "LoadExcludelist", AHSE_NAME, papyrus::LoadExcludelist);

	papyrus::RegisterFunction(a_vm, "AllowSearch", AHSE_NAME, papyrus::AllowSearch);
	papyrus::RegisterFunction(a_vm, "DisallowSearch", AHSE_NAME, papyrus::DisallowSearch);
	papyrus::RegisterFunction(a_vm, "IsSearchAllowed", AHSE_NAME, papyrus::IsSearchAllowed);
	papyrus::RegisterFunction(a_vm, "AddLocationToList", AHSE_NAME, papyrus::AddLocationToList);
	papyrus::RegisterFunction(a_vm, "DropLocationFromList", AHSE_NAME, papyrus::DropLocationFromList);

	papyrus::RegisterFunction(a_vm, "GetTranslation", AHSE_NAME, papyrus::GetTranslation);
	papyrus::RegisterFunction(a_vm, "Replace", AHSE_NAME, papyrus::Replace);
	papyrus::RegisterFunction(a_vm, "ReplaceArray", AHSE_NAME, papyrus::ReplaceArray);

	return true;
}
