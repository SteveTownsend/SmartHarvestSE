#include "PrecompiledHeaders.h"

#include <winver.h>
#include <iostream>

#include "IHasValueWeight.h"
#include "tasks.h"
#include "ManagedLists.h"
#include "LocationTracker.h"

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

namespace papyrus
{
	// available in release build, but typically unused
	void DebugTrace(RE::StaticFunctionTag* base, RE::BSFixedString str)
	{
		DBG_MESSAGE("%s", str);
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

	RE::BSFixedString GetTextObjectType(RE::StaticFunctionTag* base, RE::TESForm* thisForm)
	{
		if (!thisForm)
			return nullptr;

		ObjectType objType = GetBaseFormObjectType(thisForm, true);
		if (objType == ObjectType::unknown)
			return "NON-CLASSIFIED";

		std::string result = GetObjectTypeName(objType);
		::ToUpper(result);
		return (!result.empty()) ? result.c_str() : nullptr;
	}

	RE::BSFixedString GetObjectTypeNameByType(RE::StaticFunctionTag* base, SInt32 objectNumber)
	{
		RE::BSFixedString result;
		std::string str = GetObjectTypeName(ObjectType(objectNumber));
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
		DBG_VMESSAGE("Config setting %d/%d/%s = %f", first, second, str.c_str(), result);
		return result;
	}

	float GetSettingObjectArrayEntry(RE::StaticFunctionTag* base, SInt32 section_first, SInt32 section_second, SInt32 index)
	{
		INIFile::PrimaryType first = static_cast<INIFile::PrimaryType>(section_first);
		INIFile::SecondaryType second = static_cast<INIFile::SecondaryType>(section_second);

		INIFile* ini = INIFile::GetInstance()->GetInstance();
		if (!ini || !ini->IsType(first) || !ini->IsType(second))
			return 0.0;

		std::string key = GetObjectTypeName(ObjectType(index));
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
		DBG_VMESSAGE("Config setting %d/%d/%s = %f", first, second, key.c_str(), value);
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

	void PutSettingObjectArrayEntry(RE::StaticFunctionTag* base, SInt32 section_first, SInt32 section_second, int index, float value)
	{
		INIFile::PrimaryType first = static_cast<INIFile::PrimaryType>(section_first);
		INIFile::SecondaryType second = static_cast<INIFile::SecondaryType>(section_second);

		INIFile* ini = INIFile::GetInstance()->GetInstance();
		if (!ini || !ini->IsType(first) || !ini->IsType(second))
			return;

		std::string key = GetObjectTypeName(ObjectType(index));
		::ToLower(key);
		DBG_VMESSAGE("Put config setting (array) %d/%d/%s = %f", first, second, key.c_str(), value);
		ini->PutSetting(first, second, key.c_str(), static_cast<double>(value));
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
			REL_ERROR("LoadFile error");
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
		REL_MESSAGE("Reference Search enabled");
		SearchTask::Allow();
	}

	void DisallowSearch(RE::StaticFunctionTag* base)
	{
		REL_MESSAGE("Reference Search disabled");
		SearchTask::Disallow();
	}

	bool IsSearchAllowed(RE::StaticFunctionTag* base)
	{
		return SearchTask::IsAllowed();
	}

	void ReportOKToScan(RE::StaticFunctionTag* base, const bool goodToGo, const int nonce)
	{
		UIState::Instance().ReportVMGoodToGo(goodToGo, nonce);
	}

	constexpr int WhiteList = 1;
	constexpr int BlackList = 2;

	void ResetList(RE::StaticFunctionTag* base, const bool reloadGame, const int entryType)
	{
		if (entryType == BlackList)
		{
			ManagedList::BlackList().Reset(reloadGame);
		}
		else
		{
			ManagedList::WhiteList().Reset(reloadGame);
		}
	}
	void AddEntryToList(RE::StaticFunctionTag* base, const int entryType, const RE::TESForm* entry)
	{
		if (entryType == BlackList)
		{
			ManagedList::BlackList().Add(entry);
		}
		else
		{
			ManagedList::WhiteList().Add(entry);
		}
	}
	void SyncDone(RE::StaticFunctionTag* base)
	{
		SearchTask::SyncDone();
	}

	RE::TESForm* GetPlayerPlace(RE::StaticFunctionTag* base)
	{
		return LocationTracker::Instance().CurrentPlayerPlace();
	}

	bool UnlockHarvest(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, const bool isSilent)
	{
		return SearchTask::UnlockHarvest(refr, isSilent);
	}

	void BlockFirehose(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr)
	{
		return DataCase::GetInstance()->BlockFirehoseSource(refr);
	}

	RE::BSFixedString PrintFormID(RE::StaticFunctionTag* base, const int formID)
	{
		std::ostringstream formIDStr;
		formIDStr << "0x" << std::hex << std::setw(8) << std::setfill('0') << static_cast<RE::FormID>(formID);
		std::string result(formIDStr.str());
		DBG_VMESSAGE("FormID 0x%08x mapped to %s", formID, result.c_str());
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

	bool CollectionsInUse(RE::StaticFunctionTag* base)
	{
		return shse::CollectionManager::Instance().IsReady();
	}

	void FlushAddedItems(RE::StaticFunctionTag* base, std::vector<int> formIDs, std::vector<int> objectTypes, const int itemCount)
	{
		std::vector<std::pair<RE::FormID, ObjectType>> looted;
		looted.reserve(itemCount);
		auto formID(formIDs.cbegin());
		auto objectType(objectTypes.cbegin());
		int current(0);
		while (current < itemCount)
		{
			looted.emplace_back(RE::FormID(*formID), ObjectType(*objectType));
			++current;
			++formID;
			++objectType;
		}
		shse::CollectionManager::Instance().EnqueueAddedItems(looted);
	}

	void ToggleCalibration(RE::StaticFunctionTag* base)
	{
		SearchTask::ToggleCalibration();
	}

	bool RegisterFuncs(RE::BSScript::Internal::VirtualMachine* a_vm)
	{
		a_vm->RegisterFunction("DebugTrace", SHSE_PROXY, papyrus::DebugTrace);
		a_vm->RegisterFunction("GetPluginName", SHSE_PROXY, papyrus::GetPluginName);
		a_vm->RegisterFunction("GetPluginVersion", SHSE_PROXY, papyrus::GetPluginVersion);
		a_vm->RegisterFunction("GetTextObjectType", SHSE_PROXY, papyrus::GetTextObjectType);

		a_vm->RegisterFunction("UnlockHarvest", SHSE_PROXY, papyrus::UnlockHarvest);
		a_vm->RegisterFunction("BlockFirehose", SHSE_PROXY, papyrus::BlockFirehose);

		a_vm->RegisterFunction("GetSetting", SHSE_PROXY, papyrus::GetSetting);
		a_vm->RegisterFunction("GetSettingObjectArrayEntry", SHSE_PROXY, papyrus::GetSettingObjectArrayEntry);
		a_vm->RegisterFunction("PutSetting", SHSE_PROXY, papyrus::PutSetting);
		a_vm->RegisterFunction("PutSettingObjectArrayEntry", SHSE_PROXY, papyrus::PutSettingObjectArrayEntry);

		a_vm->RegisterFunction("GetObjectTypeNameByType", SHSE_PROXY, papyrus::GetObjectTypeNameByType);
		a_vm->RegisterFunction("GetObjectTypeByName", SHSE_PROXY, papyrus::GetObjectTypeByName);
		a_vm->RegisterFunction("GetResourceTypeByName", SHSE_PROXY, papyrus::GetResourceTypeByName);

		a_vm->RegisterFunction("Reconfigure", SHSE_PROXY, papyrus::Reconfigure);
		a_vm->RegisterFunction("LoadIniFile", SHSE_PROXY, papyrus::LoadIniFile);
		a_vm->RegisterFunction("SaveIniFile", SHSE_PROXY, papyrus::SaveIniFile);

		a_vm->RegisterFunction("SetIngredientForCritter", SHSE_PROXY, papyrus::SetIngredientForCritter);

		a_vm->RegisterFunction("ResetList", SHSE_PROXY, papyrus::ResetList);
		a_vm->RegisterFunction("AddEntryToList", SHSE_PROXY, papyrus::AddEntryToList);
		a_vm->RegisterFunction("SyncDone", SHSE_PROXY, papyrus::SyncDone);
		a_vm->RegisterFunction("PrintFormID", SHSE_PROXY, papyrus::PrintFormID);

		a_vm->RegisterFunction("AllowSearch", SHSE_PROXY, papyrus::AllowSearch);
		a_vm->RegisterFunction("DisallowSearch", SHSE_PROXY, papyrus::DisallowSearch);
		a_vm->RegisterFunction("IsSearchAllowed", SHSE_PROXY, papyrus::IsSearchAllowed);
		a_vm->RegisterFunction("ReportOKToScan", SHSE_PROXY, papyrus::ReportOKToScan);
		a_vm->RegisterFunction("GetPlayerPlace", SHSE_PROXY, papyrus::GetPlayerPlace);

		a_vm->RegisterFunction("GetTranslation", SHSE_PROXY, papyrus::GetTranslation);
		a_vm->RegisterFunction("Replace", SHSE_PROXY, papyrus::Replace);
		a_vm->RegisterFunction("ReplaceArray", SHSE_PROXY, papyrus::ReplaceArray);

		a_vm->RegisterFunction("CollectionsInUse", SHSE_PROXY, papyrus::CollectionsInUse);
		a_vm->RegisterFunction("FlushAddedItems", SHSE_PROXY, papyrus::FlushAddedItems);

		a_vm->RegisterFunction("ToggleCalibration", SHSE_PROXY, papyrus::ToggleCalibration);

		return true;
	}
}
