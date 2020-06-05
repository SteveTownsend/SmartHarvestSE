#include "PrecompiledHeaders.h"

#include "papyrus.h"

#include "IHasValueWeight.h"
#include "tasks.h"
#include "basketfile.h"

#include <winver.h>
#include <iostream>

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

	SInt32 GetResourceTypeByName(RE::StaticFunctionTag* base, RE::BSFixedString resourceTypeName)
	{
		return static_cast<SInt32>(ResourceTypeByName(resourceTypeName.c_str()));
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
			if (IsValueWeightExempt(static_cast<ObjectType>(index)) && tmp_value > LootingType::LootAlwaysSilent)
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
			SearchTask::AddLocationToBlackList(location);
		}
	}
	void DropLocationFromList(RE::StaticFunctionTag* base, const int locationType, const RE::TESForm* location)
	{
		if (locationType == LocationTypeExcluded)
		{
			SearchTask::DropLocationFromBlackList(location);
		}
	}

	bool UnlockHarvest(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, const bool isSilent)
	{
		return SearchTask::UnlockHarvest(refr, isSilent);
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
	void SyncWhiteList(RE::StaticFunctionTag* base)
	{
		BasketFile::GetSingleton()->SyncList(BasketFile::WHITELIST);
	}
	bool SaveWhiteList(RE::StaticFunctionTag* base)
	{
		return BasketFile::GetSingleton()->SaveFile(BasketFile::WHITELIST, "WhiteList.tsv");
	}
	bool LoadWhiteList(RE::StaticFunctionTag* base)
	{
		return BasketFile::GetSingleton()->LoadFile(BasketFile::WHITELIST, "WhiteList.tsv");
	}

	void ClearBlackList(RE::StaticFunctionTag* base)
	{
		SearchTask::ResetExcludedLocations();
	}
	void MergeBlackList(RE::StaticFunctionTag* base)
	{
		SearchTask::MergeBlackList();
	}
	bool SaveBlackList(RE::StaticFunctionTag* base)
	{
		return BasketFile::GetSingleton()->SaveFile(BasketFile::BLACKLIST, "BlackList.tsv");
	}
	bool LoadBlackList(RE::StaticFunctionTag* base)
	{
		return BasketFile::GetSingleton()->LoadFile(BasketFile::BLACKLIST, "BlackList.tsv");
	}

	RE::BSFixedString PrintFormID(RE::StaticFunctionTag* base, const int formID)
	{
		std::ostringstream formIDStr;
		formIDStr << "0x" << std::hex << std::setw(8) << std::setfill('0') << static_cast<RE::FormID>(formID);
		std::string result(formIDStr.str());
#if _DEBUG
		_MESSAGE("FormID 0x%08x mapped to %s", formID, result.c_str());
#endif
		return RE::BSFixedString(result.c_str());
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
	a_vm->RegisterFunction("DebugTrace", SHSE_NAME, papyrus::DebugTrace);
	a_vm->RegisterFunction("GetPluginName", SHSE_NAME, papyrus::GetPluginName);
	a_vm->RegisterFunction("GetPluginVersion", SHSE_NAME, papyrus::GetPluginVersion);
	a_vm->RegisterFunction("GetTextFormID", SHSE_NAME, papyrus::GetTextFormID);
	a_vm->RegisterFunction("GetTextObjectType", SHSE_NAME, papyrus::GetTextObjectType);

	a_vm->RegisterFunction("UnlockHarvest", SHSE_NAME, papyrus::UnlockHarvest);
	a_vm->RegisterFunction("BlockReference", SHSE_NAME, papyrus::BlockReference);
	a_vm->RegisterFunction("UnblockEverything", SHSE_NAME, papyrus::UnblockEverything);

	a_vm->RegisterFunction("GetSetting", SHSE_NAME, papyrus::GetSetting);
	a_vm->RegisterFunction("GetSettingToObjectArrayEntry", SHSE_NAME, papyrus::GetSettingToObjectArrayEntry);
	a_vm->RegisterFunction("PutSetting", SHSE_NAME, papyrus::PutSetting);
	a_vm->RegisterFunction("PutSettingObjectArray", SHSE_NAME, papyrus::PutSettingObjectArray);

	a_vm->RegisterFunction("GetObjectTypeNameByType", SHSE_NAME, papyrus::GetObjectTypeNameByType);
	a_vm->RegisterFunction("GetObjectTypeByName", SHSE_NAME, papyrus::GetObjectTypeByName);
	a_vm->RegisterFunction("GetResourceTypeByName", SHSE_NAME, papyrus::GetResourceTypeByName);

	a_vm->RegisterFunction("Reconfigure", SHSE_NAME, papyrus::Reconfigure);
	a_vm->RegisterFunction("LoadIniFile", SHSE_NAME, papyrus::LoadIniFile);
	a_vm->RegisterFunction("SaveIniFile", SHSE_NAME, papyrus::SaveIniFile);

	a_vm->RegisterFunction("SetIngredientForCritter", SHSE_NAME, papyrus::SetIngredientForCritter);

	a_vm->RegisterFunction("SyncWhiteListWithPlugin", SHSE_NAME, papyrus::SyncWhiteList);
	a_vm->RegisterFunction("SaveWhiteList", SHSE_NAME, papyrus::SaveWhiteList);
	a_vm->RegisterFunction("LoadWhiteList", SHSE_NAME, papyrus::LoadWhiteList);

	a_vm->RegisterFunction("ClearPluginBlackList", SHSE_NAME, papyrus::ClearBlackList);
	a_vm->RegisterFunction("MergePluginBlackList", SHSE_NAME, papyrus::MergeBlackList);
	a_vm->RegisterFunction("SaveBlackList", SHSE_NAME, papyrus::SaveBlackList);
	a_vm->RegisterFunction("LoadBlackList", SHSE_NAME, papyrus::LoadBlackList);
	a_vm->RegisterFunction("PrintFormID", SHSE_NAME, papyrus::PrintFormID);

	a_vm->RegisterFunction("AllowSearch", SHSE_NAME, papyrus::AllowSearch);
	a_vm->RegisterFunction("DisallowSearch", SHSE_NAME, papyrus::DisallowSearch);
	a_vm->RegisterFunction("IsSearchAllowed", SHSE_NAME, papyrus::IsSearchAllowed);
	a_vm->RegisterFunction("AddLocationToList", SHSE_NAME, papyrus::AddLocationToList);
	a_vm->RegisterFunction("DropLocationFromList", SHSE_NAME, papyrus::DropLocationFromList);

	a_vm->RegisterFunction("GetTranslation", SHSE_NAME, papyrus::GetTranslation);
	a_vm->RegisterFunction("Replace", SHSE_NAME, papyrus::Replace);
	a_vm->RegisterFunction("ReplaceArray", SHSE_NAME, papyrus::ReplaceArray);

	return true;
}
