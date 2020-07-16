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

#include "Looting/tasks.h"
#include "Utilities/utils.h"
#include "Utilities/version.h"
#include "VM/papyrus.h"
#include "Data/dataCase.h"

#include <shlobj.h>
#include <sstream>
#include <KnownFolders.h>

void SaveCallback(SKSE::SerializationInterface* a_intfc)
{
	DBG_MESSAGE("Serialization Save hook called");
	// JSON Initially
	// then add compression per https://github.com/google/brotli
	// output LoadOrder
	// output Collection Defs
	// output Collection contents
	// implement and output Location history
	// implement and output Followers-in-Party history
#if 0
	SInt32 num = 42;
	std::vector<SInt32> arr;
	for (std::size_t i = 0; i < 10; ++i) {
		arr.push_back(i);
	}

	if (!a_intfc->WriteRecord('NUM_', 1, num)) {
		_ERROR("Failed to serialize num!");
	}

	if (!a_intfc->OpenRecord('ARR_', 1)) {
		_ERROR("Failed to open record for arr!");
	}
	else {
		std::size_t size = arr.size();
		if (!a_intfc->WriteRecordData(size)) {
			_ERROR("Failed to write size of arr!");
		}
		else {
			for (auto& elem : arr) {
				if (!a_intfc->WriteRecordData(elem)) {
					_ERROR("Failed to write data for elem!");
					break;
				}
			}
		}
	}
#endif
}


void LoadCallback(SKSE::SerializationInterface* a_intfc)
{
	DBG_MESSAGE("Serialization Load hook called");
	// JSON Initially, then compressed
	// read LoadOrder
	// read Collection Defs
	// read Collection contents
	// read Location history
	// read Followers-in-Party history
#if 0
	SInt32 num;
	std::vector<SInt32> arr;

	UInt32 type;
	UInt32 version;
	UInt32 length;
	while (a_intfc->GetNextRecordInfo(type, version, length)) {
		switch (type) {
		case 'NUM_':
			if (!a_intfc->ReadRecordData(num)) {
				_ERROR("Failed to load num!");
			}
			break;
		case 'ARR_':
		{
			std::size_t size;
			if (!a_intfc->ReadRecordData(size)) {
				_ERROR("Failed to load size!");
				break;
			}

			for (UInt32 i = 0; i < size; ++i) {
				SInt32 elem;
				if (!a_intfc->ReadRecordData(elem)) {
					_ERROR("Failed to load elem!");
					break;
				}
				else {
					arr.push_back(elem);
				}
			}
		}
		break;
		default:
			_ERROR("Unrecognized signature type!");
			break;
		}
	}
#endif
}
void SKSEMessageHandler(SKSE::MessagingInterface::Message* msg)
{
	static bool scanOK(true);
	switch (msg->type)
	{
	case SKSE::MessagingInterface::kDataLoaded:
		DBG_MESSAGE("Loading Papyrus");
		SKSE::GetPapyrusInterface()->Register(papyrus::RegisterFuncs);
		REL_MESSAGE("Registered Papyrus functions!");
		break;

	case SKSE::MessagingInterface::kPreLoadGame:
		REL_MESSAGE("Game load starting");
		shse::SearchTask::PrepareForReload();
		break;

	case SKSE::MessagingInterface::kNewGame:
	case SKSE::MessagingInterface::kPostLoadGame:
		REL_MESSAGE("Game load done, initializing Tasks");
		// if checks fail, abort scanning
		if (!shse::SearchTask::Init())
		{
			REL_FATALERROR("SearchTask initialization failed - no looting");
			return;
		}
		REL_MESSAGE("Initialized SearchTask, looting available");
		shse::SearchTask::AfterReload();
		break;
	}
}

extern "C"
{

bool SKSEPlugin_Query(const SKSE::QueryInterface * a_skse, SKSE::PluginInfo * a_info)
{
	std::wostringstream path;
	path << L"/My Games/Skyrim Special Edition/SKSE/" << std::wstring(L_SHSE_NAME) << L".log";
	std::wstring wLogPath(path.str());
	SKSE::Logger::OpenRelative(FOLDERID_Documents, wLogPath);
	SKSE::Logger::SetPrintLevel(SKSE::Logger::Level::kDebugMessage);
	SKSE::Logger::SetFlushLevel(SKSE::Logger::Level::kDebugMessage);
	SKSE::Logger::UseLogStamp(true);
	SKSE::Logger::UseTimeStamp(true);
	SKSE::Logger::UseThreadID(true);
#if _DEBUG
	SKSE::Logger::HookPapyrusLog(true);
#endif
	REL_MESSAGE("%s v%s", SHSE_NAME, VersionInfo::Instance().GetPluginVersionString().c_str());

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = SHSE_NAME;
	a_info->version = VersionInfo::Instance().GetVersionMajor();

	if (a_skse->IsEditor()) {
		REL_FATALERROR("Loaded in editor, marking as incompatible!\n");
		return false;
	}
	SKSE::Version runtimeVer(a_skse->RuntimeVersion());
	if (runtimeVer < SKSE::RUNTIME_1_5_73)
	{
		REL_FATALERROR("Unsupported runtime version %08x!\n", runtimeVer);
		return false;
	}

	return true;
}

bool SKSEPlugin_Load(const SKSE::LoadInterface * skse)
{
	REL_MESSAGE("%s plugin loaded", SHSE_NAME);
	if (!SKSE::Init(skse)) {
		return false;
	}
	SKSE::GetMessagingInterface()->RegisterListener("SKSE", SKSEMessageHandler);

	auto serialization = SKSE::GetSerializationInterface();
	serialization->SetUniqueID('SHSE');
	serialization->SetSaveCallback(SaveCallback);
	serialization->SetSaveCallback(LoadCallback);

	return true;
}

};
