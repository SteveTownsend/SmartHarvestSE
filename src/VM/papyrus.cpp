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

#include <winver.h>
#include <iostream>

#include "PluginFacade.h"
#include "Data/dataCase.h"
#include "FormHelpers/IHasValueWeight.h"
#include "Looting/ScanGovernor.h"
#include "Looting/ManagedLists.h"
#include "WorldState/LocationTracker.h"
#include "WorldState/AdventureTargets.h"
#include "Looting/ProducerLootables.h"
#include "Utilities/utils.h"
#include "Utilities/version.h"
#include "Looting/objects.h"
#include "Looting/TheftCoordinator.h"
#include "Collections/CollectionManager.h"
#include "WorldState/LocationTracker.h"
#include "WorldState/PlayerState.h"
#include "WorldState/PopulationCenters.h"
#include "WorldState/QuestTargets.h"
#include "WorldState/Saga.h"

namespace
{
	std::string GetPluginName(RE::TESForm* thisForm)
	{
		std::string result;
		uint8_t loadOrder = (thisForm->formID) >> 24;
		if (loadOrder < 0xFF)
		{
			RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
			const RE::TESFile* modInfo = dhnd->LookupLoadedModByIndex(loadOrder);
			if (modInfo)
				result = std::string(&modInfo->fileName[0]);
		}
		return result;
	}
}

namespace papyrus
{
	// available in release build, but typically unused
	void DebugTrace(RE::StaticFunctionTag*, RE::BSFixedString str)
	{
		DBG_MESSAGE("{}", str.c_str());
	}

	// available in release build for important output
	void AlwaysTrace(RE::StaticFunctionTag*, RE::BSFixedString str)
	{
		REL_MESSAGE("{}", str.c_str());
	}

	RE::BSFixedString GetPluginName(RE::StaticFunctionTag*, RE::TESForm* thisForm)
	{
		if (!thisForm)
			return nullptr;
		return ::GetPluginName(thisForm).c_str();
	}

	RE::BSFixedString GetPluginVersion(RE::StaticFunctionTag*)
	{
		return RE::BSFixedString(VersionInfo::Instance().GetPluginVersionString());
	}

	RE::BSFixedString GetTextObjectType(RE::StaticFunctionTag*, RE::TESForm* thisForm)
	{
		if (!thisForm)
			return nullptr;

		RE::TESBoundObject* baseObj(thisForm->As<RE::TESBoundObject>());
		ObjectType objType = baseObj ? shse::GetEffectiveObjectType(baseObj) : ObjectType::unknown;
		if (objType == ObjectType::unknown)
			return "NON-CLASSIFIED";

		std::string result = shse::GetObjectTypeName(objType);
		StringUtils::ToUpper(result);
		return (!result.empty()) ? result.c_str() : nullptr;
	}

	RE::BSFixedString GetObjectTypeNameByType(RE::StaticFunctionTag*, int32_t objectNumber)
	{
		RE::BSFixedString result;
		std::string str = shse::GetObjectTypeName(ObjectType(objectNumber));
		if (str.empty() || str == "unknown")
			return result;
		else
			return str.c_str();
	}

	int32_t GetObjectTypeByName(RE::StaticFunctionTag*, RE::BSFixedString objectTypeName)
	{
		return static_cast<int32_t>(shse::GetObjectTypeByTypeName(objectTypeName.c_str()));
	}

	int32_t GetResourceTypeByName(RE::StaticFunctionTag*, RE::BSFixedString resourceTypeName)
	{
		return static_cast<int32_t>(shse::ResourceTypeByName(resourceTypeName.c_str()));
	}

	float GetSetting(RE::StaticFunctionTag*, int32_t section_first, int32_t section_second, RE::BSFixedString key)
	{
		INIFile::PrimaryType first = static_cast<INIFile::PrimaryType>(section_first);
		INIFile::SecondaryType second = static_cast<INIFile::SecondaryType>(section_second);

		INIFile* ini = INIFile::GetInstance();
		if (!ini || !ini->IsType(first) || !ini->IsType(second))
			return 0.0;

		std::string str = key.c_str();
		StringUtils::ToLower(str);

		float result(static_cast<float>(ini->GetSetting(first, second, str.c_str())));
		DBG_VMESSAGE("Config setting {}/{}/{} = {}", first, second, str.c_str(), result);
		return result;
	}

	float GetSettingObjectArrayEntry(RE::StaticFunctionTag*, int32_t section_first, int32_t section_second, int32_t index)
	{
		INIFile::PrimaryType first = static_cast<INIFile::PrimaryType>(section_first);
		INIFile::SecondaryType second = static_cast<INIFile::SecondaryType>(section_second);

		INIFile* ini = INIFile::GetInstance();
		if (!ini || !ini->IsType(first) || !ini->IsType(second))
			return 0.0;

		std::string key(shse::GetObjectTypeName(ObjectType(index)));
		StringUtils::ToLower(key);
		// constrain INI values to sensible values
		float value(0.0f);
		if (second == INIFile::SecondaryType::valueWeight)
		{
			float tmp_value = static_cast<float>(ini->GetSetting(first, second, key.c_str()));
			if (tmp_value < 0.0f)
			{
				value = 0.0f;
			}
			else if (tmp_value > shse::IHasValueWeight::ValueWeightMaximum)
			{
				value = shse::IHasValueWeight::ValueWeightMaximum;
			}
			else
			{
				value = tmp_value;
			}
		}
		else
		{
			shse::LootingType tmp_value = shse::LootingTypeFromIniSetting(ini->GetSetting(first, second, key.c_str()));
			// weightless objects and OreVeins are always looted unless explicitly disabled : oreVeins use a third option for BYOH materials, though
			if (ObjectType(index) == ObjectType::oreVein)
			{
				value = static_cast<float>(std::min(tmp_value, shse::LootingType::LootOreVeinAlways));
			}
			else if(shse::IsValueWeightExempt(static_cast<ObjectType>(index)) && tmp_value > shse::LootingType::LootAlwaysSilent)
			{
				value = static_cast<float>(tmp_value == shse::LootingType::LootIfValuableEnoughNotify ? shse::LootingType::LootAlwaysNotify : shse::LootingType::LootAlwaysSilent);
			}
			else
			{
				value = static_cast<float>(tmp_value);
			}
		}
		DBG_VMESSAGE("Config setting {}/{}/{} = {}", first, second, key.c_str(), value);
		return value;
	}

	int GetSettingGlowArrayEntry(RE::StaticFunctionTag*, int32_t section_first, int32_t section_second, int32_t index)
	{
		INIFile::PrimaryType first = static_cast<INIFile::PrimaryType>(section_first);
		INIFile::SecondaryType second = static_cast<INIFile::SecondaryType>(section_second);

		INIFile* ini = INIFile::GetInstance();
		if (!ini || !ini->IsType(first) || !ini->IsType(second))
			return static_cast<int>(shse::GlowReason::SimpleTarget);

		std::string key(shse::GlowName(shse::GlowReason(index)));
		StringUtils::ToLower(key);
		// constrain INI values to sensible values
		int value(0);
		int tmp_value = static_cast<int>(ini->GetSetting(first, second, key.c_str()));
		if (tmp_value < 0)
		{
			value = 0;
		}
		else
		{
			value = tmp_value;
		}
		DBG_VMESSAGE("Config setting (glow array) {}/{}/{} = {}", first, second, key.c_str(), value);
		return value;
	}

	void PutSetting(RE::StaticFunctionTag*, int32_t section_first, int32_t section_second, RE::BSFixedString key, float value)
	{
		INIFile::PrimaryType first = static_cast<INIFile::PrimaryType>(section_first);
		INIFile::SecondaryType second = static_cast<INIFile::SecondaryType>(section_second);

		INIFile* ini = INIFile::GetInstance();
		if (!ini || !ini->IsType(first) || !ini->IsType(second))
			return;

		std::string str = key.c_str();
		StringUtils::ToLower(str);

		DBG_VMESSAGE("Config setting (put) {}/{}/{} = {}", first, second, str, value);
		ini->PutSetting(first, second, str.c_str(), static_cast<double>(value));
	}

	void PutSettingObjectArrayEntry(RE::StaticFunctionTag*, int32_t section_first, int32_t section_second, int index, float value)
	{
		INIFile::PrimaryType first = static_cast<INIFile::PrimaryType>(section_first);
		INIFile::SecondaryType second = static_cast<INIFile::SecondaryType>(section_second);

		INIFile* ini = INIFile::GetInstance();
		if (!ini || !ini->IsType(first) || !ini->IsType(second))
			return;

		std::string key(shse::GetObjectTypeName(ObjectType(index)));
		StringUtils::ToLower(key);
		DBG_VMESSAGE("Put config setting (array) {}/{}/{} = {}", first, second, key.c_str(), value);
		ini->PutSetting(first, second, key.c_str(), static_cast<double>(value));
	}

	void PutSettingGlowArrayEntry(RE::StaticFunctionTag*, int32_t section_first, int32_t section_second, int index, int value)
	{
		INIFile::PrimaryType first = static_cast<INIFile::PrimaryType>(section_first);
		INIFile::SecondaryType second = static_cast<INIFile::SecondaryType>(section_second);

		INIFile* ini = INIFile::GetInstance();
		if (!ini || !ini->IsType(first) || !ini->IsType(second))
			return;

		std::string key(shse::GlowName(shse::GlowReason(index)));
		StringUtils::ToLower(key);
		DBG_VMESSAGE("Put config setting (glow array) {}/{}/{} = {}", first, second, key.c_str(), value);
		ini->PutSetting(first, second, key.c_str(), static_cast<double>(value));
	}

	bool Reconfigure(RE::StaticFunctionTag*)
	{
		INIFile* ini = INIFile::GetInstance();
		if (ini)
		{
			ini->Free();
			return true;
		}
		return false;
	}

	void LoadIniFile(RE::StaticFunctionTag*, const bool useDefaults)
	{
		INIFile* ini = INIFile::GetInstance();
		if (!ini || !ini->LoadFile(useDefaults))
		{
			REL_ERROR("LoadFile error");
		}
	}

	void SaveIniFile(RE::StaticFunctionTag*)
	{
		INIFile::GetInstance()->SaveFile();
	}

	void SetLootableForProducer(RE::StaticFunctionTag*, RE::TESForm* critter, RE::TESForm* lootable)
	{
		shse::ProducerLootables::Instance().SetLootableForProducer(critter, lootable ? lootable->As<RE::TESBoundObject>() : nullptr);
	}

	void PrepareSPERGMining(RE::StaticFunctionTag*)
	{
		shse::ScanGovernor::Instance().SPERGMiningStart();
	}

	void PostprocessSPERGMining(RE::StaticFunctionTag*)
	{
		shse::ScanGovernor::Instance().SPERGMiningEnd();
	}

	void AllowSearch(RE::StaticFunctionTag*)
	{
		REL_MESSAGE("Reference Search enabled");
		// clean lists since load game or MCM has just been active, then enable scan
		shse::PluginFacade::Instance().OnSettingsPushed();
		shse::ScanGovernor::Instance().Allow();
	}

	void DisallowSearch(RE::StaticFunctionTag*)
	{
		REL_MESSAGE("Reference Search disabled");
		// clean lists since load game or MCM has just been active, then enable scan
		shse::PluginFacade::Instance().OnSettingsPushed();
		shse::ScanGovernor::Instance().Disallow();
	}

	void SyncScanActive(RE::StaticFunctionTag*, const bool isActive)
	{
		REL_MESSAGE("Reference Search {}", isActive ? "unpaused" : "paused");
		shse::ScanGovernor::Instance().SetScanActive(isActive);
	}

	void ReportOKToScan(RE::StaticFunctionTag*, const bool delayed, const int nonce)
	{
		shse::UIState::Instance().ReportVMGoodToGo(delayed, nonce);
	}

	constexpr int White = 1;
	constexpr int Black = 2;
	constexpr int Transfer = 3;
	constexpr int EquippedOrWorn = 4;

	void ResetList(RE::StaticFunctionTag*, const int entryType)
	{
		if (entryType == Black)
		{
			shse::ManagedList::BlackList().Reset();
		}
		else if (entryType == White)
		{
			shse::ManagedList::WhiteList().Reset();
		}
		else if (entryType == EquippedOrWorn)
		{
			shse::ManagedList::EquippedOrWorn().Reset();
		}
		else
		{
			shse::ManagedList::TransferList().Reset();
		}
	}
	void AddEntryToList(RE::StaticFunctionTag*, const int entryType, RE::TESForm* entry)
	{
		if (entryType == Black)
		{
			shse::ManagedList::BlackList().Add(entry);
		}
		else if (entryType == White)
		{
			shse::ManagedList::WhiteList().Add(entry);
		}
		else if (entryType == EquippedOrWorn)
		{
			shse::ManagedList::EquippedOrWorn().Add(entry);
		}
	}
	void AddEntryToTransferList(RE::StaticFunctionTag*, RE::TESForm* entry, const std::string name)
	{
		shse::ManagedList::TransferList().AddNamed(entry, name);
	}
	// This is the last function called by the scripts when re-syncing state
	// This is called for game reload, or whitelist/blacklist updates (reload=false)
	void SyncDone(RE::StaticFunctionTag*, const bool reload)
	{
		if (reload)
		{
			shse::PluginFacade::Instance().OnVMSync();
		}
		else
		{
			shse::PluginFacade::Instance().ResetTransientState(reload);
		}
	}

	void GameIsReady(RE::StaticFunctionTag*)
	{
		shse::PluginFacade::Instance().OnGameLoaded();
	}

	const RE::TESForm* GetPlayerPlace(RE::StaticFunctionTag*)
	{
		return shse::LocationTracker::Instance().CurrentPlayerPlace();
	}

	bool UnlockHarvest(RE::StaticFunctionTag*, const int refrID, const int baseID,
		const RE::BSFixedString baseName, const bool isSilent)
	{
		return shse::ScanGovernor::Instance().UnlockHarvest(RE::FormID(refrID), RE::FormID(baseID), baseName.c_str(), isSilent);
	}

	void ProcessContainerCollectibles(RE::StaticFunctionTag*, RE::TESObjectREFR* refr)
	{
		shse::CollectionManager::Instance().CollectFromContainer(refr);
	}

	void NotifyManualLootItem(RE::StaticFunctionTag*, RE::TESObjectREFR* refr)
	{
		shse::ProcessManualLootREFR(refr);
	}

	bool IsQuestTarget(RE::StaticFunctionTag*, RE::TESForm* item)
	{
		bool cannotLoot(shse::QuestTargets::Instance().UserCannotPermission(item));
		REL_MESSAGE("Item {}/0x{:08x} is {}a Quest Target", item ? item->GetName() : "invalid", item ? item->GetFormID() : InvalidForm,
			cannotLoot ? "" : "not ");
		return cannotLoot;
	}

	// only called for Container or Actor, so check for Dynamic Base is correct
	bool IsREFRDynamic(RE::TESObjectREFR* refr)
	{
		// Do not allow processing of bad REFR or Base
		if (!refr || !refr->GetBaseObject())
			return true;
		return refr->IsDynamicForm() || refr->GetBaseObject()->IsDynamicForm();
	}

	bool IsDynamic(RE::StaticFunctionTag*, RE::TESObjectREFR* refr)
	{
		return IsREFRDynamic(refr);
	}

	std::string ValidTransferTargetLocation(RE::StaticFunctionTag*, RE::TESObjectREFR* refr, const bool linksChest, const bool knownGood)
	{
		// Check if player can designate this REFR as a target for loot transfer
		// Do not allow processing of bad REFR or Base
		if (!refr)
		{
			REL_ERROR("Blank REFR for ValidTransferTargetLocation");
			return "";
		}
		DBG_VMESSAGE("Check REFR 0x{:08x} as transfer target - linked to chest {}, known good {}", refr->GetFormID(), linksChest, knownGood);
		if (IsREFRDynamic(refr))
		{
			REL_ERROR("Dynamic REFR for ValidTransferTargetLocation");
			return "";
		}
		const RE::TESObjectCONT* container(nullptr);
		DBG_VMESSAGE("Check REFR 0x{:08x} for container", refr->GetFormID());
		if (linksChest)
		{
			DBG_VMESSAGE("REFR indicates linked chest");
			// script indicates we should check for ACTI with link to a Container
			const RE::TESObjectACTI* activator(refr->GetBaseObject()->As<RE::TESObjectACTI>());
			if (activator)
			{
				DBG_VMESSAGE("Check ACTI {}/0x{:08x} for linked container", activator->GetFullName(), activator->GetFormID());
				const RE::TESObjectREFR* linkedRefr(refr->GetLinkedRef(nullptr));
				if (linkedRefr)
				{
					container = linkedRefr->GetBaseObject()->As<RE::TESObjectCONT>();
					if (container)
					{
						DBG_VMESSAGE("ACTI {}/0x{:08x} has linked container {}/0x{:08x}", activator->GetFullName(), activator->GetFormID(),
							container->GetFullName(), container->GetFormID());
					}
				}
			}
		}
		else
		{
			container = refr->GetBaseObject()->As<RE::TESObjectCONT>();
		}
		if (!container)
		{
			REL_ERROR("REFR 0x{:08x} for ValidTransferTargetLocation is not a container", refr->GetFormID());
			return "";
		}

		if (container->data.flags.any(RE::CONT_DATA::Flag::kRespawn))
		{
			REL_ERROR("Respawning container {}/0x{:08x} for REFR 0x{:08x} not a valid transfer target", container->GetFullName(), container->GetFormID(), refr->GetFormID());
			return "";
		}
		if (shse::ManagedList::TransferList().HasContainer(container->GetFormID()))
		{
			REL_ERROR("Multiplexed linked container {}/0x{:08x} for REFR 0x{:08x} is already a transfer target", container->GetFullName(), container->GetFormID(), refr->GetFormID());
			return "";
		}
		// must be in player house to be safe, unless known good
		if (!knownGood && !shse::LocationTracker::Instance().IsPlayerAtHome())
		{
			REL_ERROR("Player not in their house, cannot use {}/0x{:08x} for REFR 0x{:08x} as a transfer target", container->GetFullName(), container->GetFormID(), refr->GetFormID());
			return "";
		}
		if (knownGood)
		{
			std::string placeName(shse::LocationTracker::Instance().CurrentPlayerPlace()->GetName());
			if (placeName.empty())
			{
				placeName = "Unknown";
			}
			DBG_VMESSAGE("Forced known good @ {} -> Transfer target {}/0x{:08x}", placeName, container->GetFullName(), container->GetFormID());
			return placeName;
		}
		else
		{
			std::string houseName(shse::LocationTracker::Instance().CurrentPlayerPlace()->GetName());
			DBG_VMESSAGE("Player house {} -> Transfer target {}/0x{:08x}", houseName, container->GetFullName(), container->GetFormID());
			return houseName;
		}
	}

	bool SupportsExcessHandling(RE::StaticFunctionTag*, int32_t index)
	{
		return TypeSupportsExcessHandling(ObjectType(index));
	}

	std::string SellItem(RE::StaticFunctionTag*, RE::TESForm* item, const bool excessOnly)
	{
		shse::ContainerLister lister(INIFile::SecondaryType::deadbodies, RE::PlayerCharacter::GetSingleton());
		return lister.SellItem(item->As<RE::TESBoundObject>(), excessOnly);
	}

	std::string TransferItem(RE::StaticFunctionTag*, RE::TESForm* item, const bool excessOnly)
	{
		shse::ContainerLister lister(INIFile::SecondaryType::deadbodies, RE::PlayerCharacter::GetSingleton());
		return lister.TransferItem(item->As<RE::TESBoundObject>(), excessOnly);
	}

	std::string DeleteItem(RE::StaticFunctionTag*, RE::TESForm* item, const bool excessOnly)
	{
		shse::ContainerLister lister(INIFile::SecondaryType::deadbodies, RE::PlayerCharacter::GetSingleton());
		return lister.DeleteItem(item->As<RE::TESBoundObject>(), excessOnly);
	}

	std::string CheckItemAsExcess(RE::StaticFunctionTag*, RE::TESForm* item)
	{
		shse::ContainerLister lister(INIFile::SecondaryType::deadbodies, RE::PlayerCharacter::GetSingleton());
		return lister.CheckItemAsExcess(item->As<RE::TESBoundObject>());
	}

	bool IsLootableObject(RE::StaticFunctionTag*, RE::TESObjectREFR* refr)
	{
		// Do not allow processing of bad REFR or Base
		if (!refr || !refr->GetBaseObject() || !refr->Is3DLoaded())
			return false;
		// non-lootable forms
		RE::FormType formType(refr->GetBaseObject()->GetFormType());
		if (formType == RE::FormType::Door ||
			formType == RE::FormType::Furniture ||
			formType == RE::FormType::Hazard ||
			formType == RE::FormType::IdleMarker ||
			formType == RE::FormType::MovableStatic ||
			formType == RE::FormType::Projectile ||			// do not allow whitelist/blacklist - need to do from Inventory
			formType == RE::FormType::Static)
		{
			return false;
		}
		return true;
	}

	RE::BSFixedString PrintFormID(RE::StaticFunctionTag*, const int formID)
	{
		return RE::BSFixedString(StringUtils::FormIDString(RE::FormID(formID)).c_str());
	}

	RE::BSFixedString GetTranslation(RE::StaticFunctionTag*, RE::BSFixedString key)
	{
		shse::DataCase* data = shse::DataCase::GetInstance();
		return data->GetTranslation(key.c_str());
	}

	RE::BSFixedString Replace(RE::StaticFunctionTag*, RE::BSFixedString str, RE::BSFixedString target, RE::BSFixedString replacement)
	{
		std::string s_str(str.c_str());
		return (StringUtils::Replace(s_str, target.c_str(), replacement.c_str())) ? s_str.c_str() : nullptr;
	}

	RE::BSFixedString ReplaceArray(RE::StaticFunctionTag*, RE::BSFixedString str, std::vector<RE::BSFixedString> targets, std::vector<RE::BSFixedString> replacements)
	{
		std::string result(str.c_str());
		if (result.empty() || targets.size() != replacements.size())
			return nullptr;

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

	bool CollectionsInUse(RE::StaticFunctionTag*)
	{
		return shse::CollectionManager::Instance().IsAvailable();
	}

	void FlushAddedItems(RE::StaticFunctionTag*, const float gameTime, const std::vector<RE::TESForm*> forms,
		const std::vector<int> scopes, const std::vector<int> objectTypes, const int itemCount)
	{
		DBG_MESSAGE("Flush {}/{} added items", itemCount, forms.size());
		auto form(forms.cbegin());
		auto scope(scopes.cbegin());
		auto objectType(objectTypes.cbegin());
		int current(0);
		shse::PlayerState::Instance().UpdateGameTime(gameTime);
		while (current < itemCount)
		{
			// checked API
			shse::CollectionManager::Instance().CheckEnqueueAddedItem(
				(*form)->As<RE::TESBoundObject>(), INIFile::SecondaryType(*scope), ObjectType(*objectType));
			++current;
			++form;
			++scope;
			++objectType;
		}
	}

	void PushGameTime(RE::StaticFunctionTag*, const float gameTime)
	{
		shse::PlayerState::Instance().UpdateGameTime(gameTime);
	}

	int CollectionGroups(RE::StaticFunctionTag*)
	{
		return shse::CollectionManager::Instance().NumberOfFiles();
	}

	std::string CollectionGroupName(RE::StaticFunctionTag*, const int fileIndex)
	{
		return shse::CollectionManager::Instance().GroupNameByIndex(fileIndex);
	}

	std::string CollectionGroupFile(RE::StaticFunctionTag*, const int fileIndex)
	{
		return shse::CollectionManager::Instance().GroupFileByIndex(fileIndex);
	}

	int CollectionsInGroup(RE::StaticFunctionTag*, const std::string fileName)
	{
		return shse::CollectionManager::Instance().NumberOfActiveCollections(fileName);
	}

	std::string CollectionNameByIndexInGroup(RE::StaticFunctionTag*, const std::string groupName, const int collectionIndex)
	{
		return shse::CollectionManager::Instance().NameByIndexInGroup(groupName, collectionIndex);
	}

	std::string CollectionDescriptionByIndexInGroup(RE::StaticFunctionTag*, const std::string groupName, const int collectionIndex)
	{
		return shse::CollectionManager::Instance().DescriptionByIndexInGroup(groupName, collectionIndex);
	}

	bool CollectionAllowsRepeats(RE::StaticFunctionTag*, const std::string groupName, const std::string collectionName)
	{
		return shse::CollectionManager::Instance().PolicyRepeat(groupName, collectionName);
	}
	bool CollectionNotifies(RE::StaticFunctionTag*, const std::string groupName, const std::string collectionName)
	{
		return shse::CollectionManager::Instance().PolicyNotify(groupName, collectionName);
	}
	int CollectionAction(RE::StaticFunctionTag*, const std::string groupName, const std::string collectionName)
	{
		return static_cast<int>(shse::CollectionManager::Instance().PolicyAction(groupName, collectionName));
	}
	void PutCollectionAllowsRepeats(RE::StaticFunctionTag*, const std::string groupName, const std::string collectionName, const bool allowRepeats)
	{
		shse::CollectionManager::Instance().PolicySetRepeat(groupName, collectionName, allowRepeats);
	}
	void PutCollectionNotifies(RE::StaticFunctionTag*, const std::string groupName, const std::string collectionName, const bool notifies)
	{
		shse::CollectionManager::Instance().PolicySetNotify(groupName, collectionName, notifies);
	}
	void PutCollectionAction(RE::StaticFunctionTag*, const std::string groupName, const std::string collectionName, const int action)
	{
		shse::CollectionManager::Instance().PolicySetAction(groupName, collectionName, shse::CollectibleHandlingFromIniSetting(double(action)));
	}

	bool CollectionGroupAllowsRepeats(RE::StaticFunctionTag*, const std::string groupName)
	{
		return shse::CollectionManager::Instance().GroupPolicyRepeat(groupName);
	}
	bool CollectionGroupNotifies(RE::StaticFunctionTag*, const std::string groupName)
	{
		return shse::CollectionManager::Instance().GroupPolicyNotify(groupName);
	}
	int CollectionGroupAction(RE::StaticFunctionTag*, const std::string groupName)
	{
		return static_cast<int>(shse::CollectionManager::Instance().GroupPolicyAction(groupName));
	}
	void PutCollectionGroupAllowsRepeats(RE::StaticFunctionTag*, const std::string groupName, const bool allowRepeats)
	{
		shse::CollectionManager::Instance().GroupPolicySetRepeat(groupName, allowRepeats);
	}
	void PutCollectionGroupNotifies(RE::StaticFunctionTag*, const std::string groupName, const bool notifies)
	{
		shse::CollectionManager::Instance().GroupPolicySetNotify(groupName, notifies);
	}
	void PutCollectionGroupAction(RE::StaticFunctionTag*, const std::string groupName, const int action)
	{
		shse::CollectionManager::Instance().GroupPolicySetAction(groupName, shse::CollectibleHandlingFromIniSetting(double(action)));
	}

	int CollectionTotal(RE::StaticFunctionTag*, const std::string groupName, const std::string collectionName)
	{
		return static_cast<int>(shse::CollectionManager::Instance().TotalItems(groupName, collectionName));
	}

	std::string CollectionStatus(RE::StaticFunctionTag*, const std::string groupName, const std::string collectionName)
	{
		return shse::CollectionManager::Instance().StatusMessage(groupName, collectionName);
	}

	int CollectionObtained(RE::StaticFunctionTag*, const std::string groupName, const std::string collectionName)
	{
		return static_cast<int>(shse::CollectionManager::Instance().ItemsObtained(groupName, collectionName));
	}

	int AdventureTypeCount(RE::StaticFunctionTag*)
	{
		return static_cast<int>(shse::AdventureTargets::Instance().AvailableAdventureTypes());
	}

	std::string AdventureTypeName(RE::StaticFunctionTag*, const int adventureType)
	{
		return shse::AdventureTargets::Instance().AdventureTypeName(size_t(adventureType));
	}

	int ViableWorldsByType(RE::StaticFunctionTag*, const int adventureType)
	{
		return static_cast<int>(shse::AdventureTargets::Instance().ViableWorldCount(size_t(adventureType)));
	}

	std::string WorldNameByIndex(RE::StaticFunctionTag*, const int worldIndex)
	{
		return shse::AdventureTargets::Instance().ViableWorldNameByIndexInView(size_t(worldIndex));
	}

	void SetAdventureTarget(RE::StaticFunctionTag*, const int worldIndex)
	{
		return shse::AdventureTargets::Instance().SelectCurrentDestination(size_t(worldIndex));
	}

	void ClearAdventureTarget(RE::StaticFunctionTag*)
	{
		return shse::AdventureTargets::Instance().AbandonCurrentDestination();
	}

	bool HasAdventureTarget(RE::StaticFunctionTag*)
	{
		return shse::AdventureTargets::Instance().HasActiveTarget();
	}

	void ToggleCalibration(RE::StaticFunctionTag*, const bool shaderTest)
	{
		shse::ScanGovernor::Instance().ToggleCalibration(shaderTest);
	}

	void ShowLocation(RE::StaticFunctionTag*)
	{
		shse::LocationTracker::Instance().DisplayPlayerLocation();
	}

	void GlowNearbyLoot(RE::StaticFunctionTag*)
	{
		shse::ScanGovernor::Instance().InvokeLootSense();
	}

	const RE::Actor* GetDetectingActor(RE::StaticFunctionTag*, const int actorIndex, const bool dryRun)
	{
		if (dryRun)
		{
			return shse::ScanGovernor::Instance().ActorByIndex(static_cast<size_t>(actorIndex));
		}
		else
		{
			return shse::TheftCoordinator::Instance().ActorByIndex(static_cast<size_t>(actorIndex));
		}
	}

	void ReportPlayerDetectionState(RE::StaticFunctionTag*, const bool detected)
	{
		shse::TheftCoordinator::Instance().StealOrForgetItems(detected);
	}

	void CheckLootable(RE::StaticFunctionTag*, RE::TESObjectREFR* refr)
	{
		shse::ScanGovernor::Instance().DisplayLootability(refr);
	}

	int StartTimer(RE::StaticFunctionTag*, const std::string timerContext)
	{
		return WindowsUtils::ScopedTimerFactory::Instance().StartTimer(timerContext);
	}

	void StopTimer(RE::StaticFunctionTag*, const int timerHandle)
	{
		WindowsUtils::ScopedTimerFactory::Instance().StopTimer(timerHandle);
	}

	int GetTimelineDays(RE::StaticFunctionTag*)
	{
		return static_cast<int>(shse::Saga::Instance().DaysWithEvents());
	}

	std::string TimelineDayName(RE::StaticFunctionTag*, const int timelineDay)
	{
		return shse::Saga::Instance().DateStringByIndex(static_cast<unsigned int>(timelineDay));
	}

	int PageCountForDay(RE::StaticFunctionTag*)
	{
		return static_cast<int>(shse::Saga::Instance().CurrentDayPageCount());
	}

	std::string GetSagaDayPage(RE::StaticFunctionTag*, const int pageNumber)
	{
		return shse::Saga::Instance().PageByNumber(static_cast<unsigned int>(pageNumber));
	}

	bool RegisterFuncs(RE::BSScript::Internal::VirtualMachine* a_vm)
	{
		a_vm->RegisterFunction("DebugTrace", SHSE_PROXY, papyrus::DebugTrace);
		a_vm->RegisterFunction("AlwaysTrace", SHSE_PROXY, papyrus::AlwaysTrace);
		a_vm->RegisterFunction("GetPluginName", SHSE_PROXY, papyrus::GetPluginName);
		a_vm->RegisterFunction("GetPluginVersion", SHSE_PROXY, papyrus::GetPluginVersion);
		a_vm->RegisterFunction("GetTextObjectType", SHSE_PROXY, papyrus::GetTextObjectType);

		a_vm->RegisterFunction("UnlockHarvest", SHSE_PROXY, papyrus::UnlockHarvest);
		a_vm->RegisterFunction("NotifyManualLootItem", SHSE_PROXY, papyrus::NotifyManualLootItem);
		a_vm->RegisterFunction("IsQuestTarget", SHSE_PROXY, papyrus::IsQuestTarget);
		a_vm->RegisterFunction("IsDynamic", SHSE_PROXY, papyrus::IsDynamic);
		a_vm->RegisterFunction("IsLootableObject", SHSE_PROXY, papyrus::IsLootableObject);
		a_vm->RegisterFunction("ValidTransferTargetLocation", SHSE_PROXY, papyrus::ValidTransferTargetLocation);
		a_vm->RegisterFunction("SupportsExcessHandling", SHSE_PROXY, papyrus::SupportsExcessHandling);
		a_vm->RegisterFunction("SellItem", SHSE_PROXY, papyrus::SellItem);
		a_vm->RegisterFunction("TransferItem", SHSE_PROXY, papyrus::TransferItem);
		a_vm->RegisterFunction("DeleteItem", SHSE_PROXY, papyrus::DeleteItem);
		a_vm->RegisterFunction("CheckItemAsExcess", SHSE_PROXY, papyrus::CheckItemAsExcess);
		a_vm->RegisterFunction("ProcessContainerCollectibles", SHSE_PROXY, papyrus::ProcessContainerCollectibles);

		a_vm->RegisterFunction("GetSetting", SHSE_PROXY, papyrus::GetSetting);
		a_vm->RegisterFunction("GetSettingObjectArrayEntry", SHSE_PROXY, papyrus::GetSettingObjectArrayEntry);
		a_vm->RegisterFunction("GetSettingGlowArrayEntry", SHSE_PROXY, papyrus::GetSettingGlowArrayEntry);
		a_vm->RegisterFunction("PutSetting", SHSE_PROXY, papyrus::PutSetting);
		a_vm->RegisterFunction("PutSettingObjectArrayEntry", SHSE_PROXY, papyrus::PutSettingObjectArrayEntry);
		a_vm->RegisterFunction("PutSettingGlowArrayEntry", SHSE_PROXY, papyrus::PutSettingGlowArrayEntry);

		a_vm->RegisterFunction("GetObjectTypeNameByType", SHSE_PROXY, papyrus::GetObjectTypeNameByType);
		a_vm->RegisterFunction("GetObjectTypeByName", SHSE_PROXY, papyrus::GetObjectTypeByName);
		a_vm->RegisterFunction("GetResourceTypeByName", SHSE_PROXY, papyrus::GetResourceTypeByName);

		a_vm->RegisterFunction("Reconfigure", SHSE_PROXY, papyrus::Reconfigure);
		a_vm->RegisterFunction("LoadIniFile", SHSE_PROXY, papyrus::LoadIniFile);
		a_vm->RegisterFunction("SaveIniFile", SHSE_PROXY, papyrus::SaveIniFile);

		a_vm->RegisterFunction("SetLootableForProducer", SHSE_PROXY, papyrus::SetLootableForProducer);
		a_vm->RegisterFunction("PrepareSPERGMining", SHSE_PROXY, papyrus::PrepareSPERGMining);
		a_vm->RegisterFunction("PostprocessSPERGMining", SHSE_PROXY, papyrus::PostprocessSPERGMining);

		a_vm->RegisterFunction("ResetList", SHSE_PROXY, papyrus::ResetList);
		a_vm->RegisterFunction("AddEntryToList", SHSE_PROXY, papyrus::AddEntryToList);
		a_vm->RegisterFunction("AddEntryToTransferList", SHSE_PROXY, papyrus::AddEntryToTransferList);
		a_vm->RegisterFunction("SyncDone", SHSE_PROXY, papyrus::SyncDone);
		a_vm->RegisterFunction("PrintFormID", SHSE_PROXY, papyrus::PrintFormID);

		a_vm->RegisterFunction("AllowSearch", SHSE_PROXY, papyrus::AllowSearch);
		a_vm->RegisterFunction("DisallowSearch", SHSE_PROXY, papyrus::DisallowSearch);
		a_vm->RegisterFunction("SyncScanActive", SHSE_PROXY, papyrus::SyncScanActive);
		a_vm->RegisterFunction("ReportOKToScan", SHSE_PROXY, papyrus::ReportOKToScan);
		a_vm->RegisterFunction("GameIsReady", SHSE_PROXY, papyrus::GameIsReady);
		a_vm->RegisterFunction("GetPlayerPlace", SHSE_PROXY, papyrus::GetPlayerPlace);

		a_vm->RegisterFunction("GetTranslation", SHSE_PROXY, papyrus::GetTranslation);
		a_vm->RegisterFunction("Replace", SHSE_PROXY, papyrus::Replace);
		a_vm->RegisterFunction("ReplaceArray", SHSE_PROXY, papyrus::ReplaceArray);

		a_vm->RegisterFunction("CollectionsInUse", SHSE_PROXY, papyrus::CollectionsInUse);
		a_vm->RegisterFunction("FlushAddedItems", SHSE_PROXY, papyrus::FlushAddedItems);
		a_vm->RegisterFunction("PushGameTime", SHSE_PROXY, papyrus::PushGameTime);
		a_vm->RegisterFunction("CollectionGroups", SHSE_PROXY, papyrus::CollectionGroups);
		a_vm->RegisterFunction("CollectionGroupName", SHSE_PROXY, papyrus::CollectionGroupName);
		a_vm->RegisterFunction("CollectionGroupFile", SHSE_PROXY, papyrus::CollectionGroupFile);
		a_vm->RegisterFunction("CollectionsInGroup", SHSE_PROXY, papyrus::CollectionsInGroup);
		a_vm->RegisterFunction("CollectionNameByIndexInGroup", SHSE_PROXY, papyrus::CollectionNameByIndexInGroup);
		a_vm->RegisterFunction("CollectionDescriptionByIndexInGroup", SHSE_PROXY, papyrus::CollectionDescriptionByIndexInGroup);
		a_vm->RegisterFunction("CollectionAllowsRepeats", SHSE_PROXY, papyrus::CollectionAllowsRepeats);
		a_vm->RegisterFunction("CollectionNotifies", SHSE_PROXY, papyrus::CollectionNotifies);
		a_vm->RegisterFunction("CollectionAction", SHSE_PROXY, papyrus::CollectionAction);
		a_vm->RegisterFunction("CollectionTotal", SHSE_PROXY, papyrus::CollectionTotal);
		a_vm->RegisterFunction("CollectionStatus", SHSE_PROXY, papyrus::CollectionStatus);
		a_vm->RegisterFunction("CollectionObtained", SHSE_PROXY, papyrus::CollectionObtained);
		a_vm->RegisterFunction("PutCollectionAllowsRepeats", SHSE_PROXY, papyrus::PutCollectionAllowsRepeats);
		a_vm->RegisterFunction("PutCollectionNotifies", SHSE_PROXY, papyrus::PutCollectionNotifies);
		a_vm->RegisterFunction("PutCollectionAction", SHSE_PROXY, papyrus::PutCollectionAction);
		a_vm->RegisterFunction("CollectionGroupAllowsRepeats", SHSE_PROXY, papyrus::CollectionGroupAllowsRepeats);
		a_vm->RegisterFunction("CollectionGroupNotifies", SHSE_PROXY, papyrus::CollectionGroupNotifies);
		a_vm->RegisterFunction("CollectionGroupAction", SHSE_PROXY, papyrus::CollectionGroupAction);
		a_vm->RegisterFunction("PutCollectionGroupAllowsRepeats", SHSE_PROXY, papyrus::PutCollectionGroupAllowsRepeats);
		a_vm->RegisterFunction("PutCollectionGroupNotifies", SHSE_PROXY, papyrus::PutCollectionGroupNotifies);
		a_vm->RegisterFunction("PutCollectionGroupAction", SHSE_PROXY, papyrus::PutCollectionGroupAction);

		a_vm->RegisterFunction("AdventureTypeCount", SHSE_PROXY, papyrus::AdventureTypeCount);
		a_vm->RegisterFunction("AdventureTypeName", SHSE_PROXY, papyrus::AdventureTypeName);
		a_vm->RegisterFunction("ViableWorldsByType", SHSE_PROXY, papyrus::ViableWorldsByType);
		a_vm->RegisterFunction("WorldNameByIndex", SHSE_PROXY, papyrus::WorldNameByIndex);
		a_vm->RegisterFunction("SetAdventureTarget", SHSE_PROXY, papyrus::SetAdventureTarget);
		a_vm->RegisterFunction("ClearAdventureTarget", SHSE_PROXY, papyrus::ClearAdventureTarget);
		a_vm->RegisterFunction("HasAdventureTarget", SHSE_PROXY, papyrus::HasAdventureTarget);

		a_vm->RegisterFunction("ToggleCalibration", SHSE_PROXY, papyrus::ToggleCalibration);
		a_vm->RegisterFunction("ShowLocation", SHSE_PROXY, papyrus::ShowLocation);
		a_vm->RegisterFunction("GlowNearbyLoot", SHSE_PROXY, papyrus::GlowNearbyLoot);

		a_vm->RegisterFunction("GetDetectingActor", SHSE_PROXY, papyrus::GetDetectingActor);
		a_vm->RegisterFunction("ReportPlayerDetectionState", SHSE_PROXY, papyrus::ReportPlayerDetectionState);
		a_vm->RegisterFunction("CheckLootable", SHSE_PROXY, papyrus::CheckLootable);

		a_vm->RegisterFunction("StartTimer", SHSE_PROXY, papyrus::StartTimer);
		a_vm->RegisterFunction("StopTimer", SHSE_PROXY, papyrus::StopTimer);

		a_vm->RegisterFunction("GetTimelineDays", SHSE_PROXY, papyrus::GetTimelineDays);
		a_vm->RegisterFunction("TimelineDayName", SHSE_PROXY, papyrus::TimelineDayName);
		a_vm->RegisterFunction("PageCountForDay", SHSE_PROXY, papyrus::PageCountForDay);
		a_vm->RegisterFunction("GetSagaDayPage", SHSE_PROXY, papyrus::GetSagaDayPage);
		return true;
	}
}
