#include "PrecompiledHeaders.h"

#include "CommonLibSSE/include/RE/BSScript/TypeTraits.h"
#include "CommonLibSSE/include/RE/BSScript/Array.h"
#include "CommonLibSSE/include/RE/BSScript/NativeFunction.h"
#include "papyrus.h"

#include "IHasValueWeight.h"
#include "tasks.h"
#include "iniSettings.h"
#include "basketfile.h"
#include "objects.h"
#include "dataCase.h"

#include <winver.h>

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
	// available in release build, but typically unused
	void DebugTrace(RE::StaticFunctionTag* base, RE::BSFixedString str)
	{
		_MESSAGE("%s", str);
	}

	RE::BSFixedString GetPluginName(RE::StaticFunctionTag* base, RE::TESForm* thisForm)
	{
		if (!thisForm)
			return nullptr;
		return ::GetPluginName(thisForm).c_str();
	}

	RE::BSFixedString GetPluginVersion(RE::StaticFunctionTag* base)
	{
		return RE::BSFixedString(VersionInfo::Instance().GetPluginVersionString());
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

	RE::BSFixedString GetObjectTypeNameByType(RE::StaticFunctionTag* base, SInt32 objectNumber)
	{
		RE::BSFixedString result;
		std::string str = GetObjectTypeName(objectNumber);
		if (str.empty() || str.c_str() == "unknown")
			return result;
		else
			return str.c_str();
	}

	SInt32 GetObjectTypeByName(RE::StaticFunctionTag* base, RE::BSFixedString objectTypeName)
	{
		return static_cast<SInt32>(GetObjectTypeByTypeName(objectTypeName.c_str()));
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

		float result(static_cast<float>(ini->GetSetting(first, second, str.c_str())));
#if _DEBUG
		_DMESSAGE("Config setting %d/%d/%s = %f", first, second, str.c_str(), result);
#endif
		return result;
	}

	float GetSettingToObjectArrayEntry(RE::StaticFunctionTag* base, SInt32 section_first, SInt32 section_second, SInt32 index)
	{
		INIFile::PrimaryType first = static_cast<INIFile::PrimaryType>(section_first);
		INIFile::SecondaryType second = static_cast<INIFile::SecondaryType>(section_second);

		INIFile* ini = INIFile::GetInstance()->GetInstance();
		if (!ini || !ini->IsType(first) || !ini->IsType(second))
			return 0.0;

		std::string key = GetObjectTypeName(index);
		::ToLower(key);
		// constrain INI values to sensible values
		float value(0.0f);
		if (second == INIFile::SecondaryType::valueWeight)
		{
			float tmp_value = static_cast<float>(ini->GetSetting(first, second, key.c_str()));
			if (tmp_value < 0.0f)
			{
				value = 0.0f;
			}
			else if (tmp_value > IHasValueWeight::ValueWeightMaximum)
			{
				value = IHasValueWeight::ValueWeightMaximum;
			}
			else
			{
				value = tmp_value;
			}
		}
		else
		{
			LootingType tmp_value = LootingTypeFromIniSetting(ini->GetSetting(first, second, key.c_str()));
			// weightless objects and OreVeins are always looted unless explicitly disabled
			if (ValueWeightExempt(static_cast<ObjectType>(index)) && tmp_value > LootingType::LootAlwaysSilent)
			{
				value = static_cast<float>(tmp_value == LootingType::LootIfValuableEnoughNotify ? LootingType::LootAlwaysNotify : LootingType::LootAlwaysSilent);
			}
			else
			{
				value = static_cast<float>(tmp_value);
			}
		}
#if _DEBUG
		_DMESSAGE("Config setting %d/%d/%s = %f", first, second, key.c_str(), value);
#endif
		return value;
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

	void PutSettingObjectArray(RE::StaticFunctionTag* base, SInt32 section_first, SInt32 section_second, std::vector<float> value_arr)
	{
		INIFile::PrimaryType first = static_cast<INIFile::PrimaryType>(section_first);
		INIFile::SecondaryType second = static_cast<INIFile::SecondaryType>(section_second);

		INIFile* ini = INIFile::GetInstance()->GetInstance();
		if (!ini || !ini->IsType(first) || !ini->IsType(second))
			return;

		SInt32 index(0);
		for (auto tmp_value : value_arr)
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
		DataCase::GetInstance()->SetLootableForProducer(critter, ingredient);
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

	bool BlockReference(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr)
	{
		return DataCase::GetInstance()->BlockReference(refr);
	}

	void UnblockEverything(RE::StaticFunctionTag* base)
	{
		static const bool gameReload(false);
		SearchTask::ResetRestrictions(gameReload);
	}
	void SyncUserList(RE::StaticFunctionTag* base)
	{
		BasketFile::GetSingleton()->SyncList(BasketFile::USERLIST);
	}
	bool SaveUserList(RE::StaticFunctionTag* base)
	{
		return BasketFile::GetSingleton()->SaveFile(BasketFile::USERLIST, "userlist.tsv");
	}
	bool LoadUserList(RE::StaticFunctionTag* base)
	{
		return BasketFile::GetSingleton()->LoadFile(BasketFile::USERLIST, "userlist.tsv");
	}

	void ClearExcludeList(RE::StaticFunctionTag* base)
	{
		SearchTask::ResetExcludedLocations();
	}
	void MergeExcludeList(RE::StaticFunctionTag* base)
	{
		SearchTask::MergeExcludeList();
	}
	bool SaveExcludeList(RE::StaticFunctionTag* base)
	{
		return BasketFile::GetSingleton()->SaveFile(BasketFile::EXCLUDELIST, "excludelist.tsv");
	}
	bool LoadExcludeList(RE::StaticFunctionTag* base)
	{
		return BasketFile::GetSingleton()->LoadFile(BasketFile::EXCLUDELIST, "excludelist.tsv");
	}

	RE::BSFixedString GetTranslation(RE::StaticFunctionTag* base, RE::BSFixedString key)
	{
		DataCase* data = DataCase::GetInstance();
		return data->GetTranslation(key.c_str());
	}

	RE::BSFixedString Replace(RE::StaticFunctionTag* base, RE::BSFixedString str, RE::BSFixedString target, RE::BSFixedString replacement)
	{
		std::string s_str(str.c_str());
		std::string s_target(target.c_str());
		std::string s_replacement(replacement.c_str());
		return (StringUtils::Replace(s_str, s_target, s_replacement)) ? s_str.c_str() : nullptr;
	}

	RE::BSFixedString ReplaceArray(RE::StaticFunctionTag* base, RE::BSFixedString str, std::vector<RE::BSFixedString> targets, std::vector<RE::BSFixedString> replacements)
	{
		std::string result(str.c_str());
		if (result.empty() || targets.size() != replacements.size())
			return nullptr;

		RE::BSFixedString target;
		RE::BSFixedString replacement;
		for (std::vector<RE::BSFixedString>::const_iterator target = targets.cbegin(), replacement = replacements.cbegin();
			target != targets.cend(); ++target, ++replacement)
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
	a_vm->RegisterFunction("DebugTrace", AHSE_NAME, papyrus::DebugTrace);
	a_vm->RegisterFunction("GetPluginName", AHSE_NAME, papyrus::GetPluginName);
	a_vm->RegisterFunction("GetPluginVersion", AHSE_NAME, papyrus::GetPluginVersion);
	a_vm->RegisterFunction("GetTextFormID", AHSE_NAME, papyrus::GetTextFormID);
	a_vm->RegisterFunction("GetTextObjectType", AHSE_NAME, papyrus::GetTextObjectType);

	a_vm->RegisterFunction("UnlockAutoHarvest", AHSE_NAME, papyrus::UnlockAutoHarvest);
	a_vm->RegisterFunction("BlockReference", AHSE_NAME, papyrus::BlockReference);
	a_vm->RegisterFunction("UnblockEverything", AHSE_NAME, papyrus::UnblockEverything);

	a_vm->RegisterFunction("GetSetting", AHSE_NAME, papyrus::GetSetting);
	a_vm->RegisterFunction("GetSettingToObjectArrayEntry", AHSE_NAME, papyrus::GetSettingToObjectArrayEntry);
	a_vm->RegisterFunction("PutSetting", AHSE_NAME, papyrus::PutSetting);
	a_vm->RegisterFunction("PutSettingObjectArray", AHSE_NAME, papyrus::PutSettingObjectArray);

	a_vm->RegisterFunction("GetObjectTypeNameByType", AHSE_NAME, papyrus::GetObjectTypeNameByType);
	a_vm->RegisterFunction("GetObjectTypeByName", AHSE_NAME, papyrus::GetObjectTypeByName);

	a_vm->RegisterFunction("Reconfigure", AHSE_NAME, papyrus::Reconfigure);
	a_vm->RegisterFunction("LoadIniFile", AHSE_NAME, papyrus::LoadIniFile);
	a_vm->RegisterFunction("SaveIniFile", AHSE_NAME, papyrus::SaveIniFile);

	a_vm->RegisterFunction("SetIngredientForCritter", AHSE_NAME, papyrus::SetIngredientForCritter);

	a_vm->RegisterFunction("SyncUserListWithPlugin", AHSE_NAME, papyrus::SyncUserList);
	a_vm->RegisterFunction("SaveUserList", AHSE_NAME, papyrus::SaveUserList);
	a_vm->RegisterFunction("LoadUserList", AHSE_NAME, papyrus::LoadUserList);

	a_vm->RegisterFunction("ClearPluginExcludeList", AHSE_NAME, papyrus::ClearExcludeList);
	a_vm->RegisterFunction("MergePluginExcludeList", AHSE_NAME, papyrus::MergeExcludeList);
	a_vm->RegisterFunction("SaveExcludeList", AHSE_NAME, papyrus::SaveExcludeList);
	a_vm->RegisterFunction("LoadExcludeList", AHSE_NAME, papyrus::LoadExcludeList);

	a_vm->RegisterFunction("AllowSearch", AHSE_NAME, papyrus::AllowSearch);
	a_vm->RegisterFunction("DisallowSearch", AHSE_NAME, papyrus::DisallowSearch);
	a_vm->RegisterFunction("IsSearchAllowed", AHSE_NAME, papyrus::IsSearchAllowed);
	a_vm->RegisterFunction("AddLocationToList", AHSE_NAME, papyrus::AddLocationToList);
	a_vm->RegisterFunction("DropLocationFromList", AHSE_NAME, papyrus::DropLocationFromList);

	a_vm->RegisterFunction("GetTranslation", AHSE_NAME, papyrus::GetTranslation);
	a_vm->RegisterFunction("Replace", AHSE_NAME, papyrus::Replace);
	a_vm->RegisterFunction("ReplaceArray", AHSE_NAME, papyrus::ReplaceArray);

	return true;
}
