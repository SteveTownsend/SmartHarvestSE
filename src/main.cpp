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
#include "PluginFacade.h"

#include "Utilities/utils.h"
#include "Utilities/version.h"
#include "VM/papyrus.h"
#include "Data/dataCase.h"
#include "Data/LoadOrder.h"

#include <shlobj.h>
#include <sstream>
#include <KnownFolders.h>
#include <filesystem>

constexpr const char* SAVEDATAFILE("SaveData.compressed.json");
void SaveCallback(SKSE::SerializationInterface* a_intfc)
{
	DBG_MESSAGE("Serialization Save hook called");
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Serialization Save hook");
#endif
	// Serialize JSON and compress per https://github.com/google/brotli
	// output LoadOrder
	// output Collection Defs
	// output Collection contents
	// output Location history
	// output Followers-in-Party history
	// output Party Kills history

	nlohmann::json j(shse::LoadOrder::Instance());
	DBG_MESSAGE("LORD:\n%s", j.dump().c_str());
	std::string compressed(CompressionUtils::EncodeBrotli(j));
	std::ofstream saveData(SAVEDATAFILE, std::ios::out | std::ios::binary);
	saveData.write(compressed.c_str(), compressed.length());
	saveData.close();
#if 0
	if (!a_intfc->WriteRecord('LORD', 1, CompressionUtils::EncodeBrotli(j).c_str())) 
	{
		REL_ERROR("Failed to serialize LORD");
	}
#endif
}


void LoadCallback(SKSE::SerializationInterface* a_intfc)
{
	DBG_MESSAGE("Serialization Load hook called");
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Serialization Load hook");
#endif
	try {
		// decompress per https://github.com/google/brotli and rehydrate to JSON
		size_t fileSize(std::filesystem::file_size(SAVEDATAFILE));
		std::ifstream readData(SAVEDATAFILE, std::ios::in | std::ios::binary);
		std::string roundTrip(fileSize, 0);
		readData.read(const_cast<char*>(roundTrip.c_str()), roundTrip.length());
		nlohmann::json jRead(CompressionUtils::DecodeBrotli(roundTrip));
		DBG_MESSAGE("Read:\n%s", jRead.dump().c_str());
	}
	catch (const std::exception& exc)
	{
		DBG_ERROR("LoadFile error on %s: %s", SAVEDATAFILE, exc.what());
	}
	// read LoadOrder
	// read Collection Defs
	// read Collection contents
	// read Location history
	// read Followers-in-Party history
	// read Party Kills history
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
		shse::PluginFacade::Instance().PrepareForReload();
		break;

	case SKSE::MessagingInterface::kNewGame:
	case SKSE::MessagingInterface::kPostLoadGame:
		REL_MESSAGE("Game load done, initializing Tasks");
		// if checks fail, abort scanning
		if (!shse::PluginFacade::Instance().Init())
		{
			REL_FATALERROR("SearchTask initialization failed - no looting");
			return;
		}
		REL_MESSAGE("Initialized SearchTask, looting available");
		shse::PluginFacade::Instance().AfterReload();
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
	serialization->SetLoadCallback(LoadCallback);

	return true;
}

};
