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
#include "Data/CosaveData.h"
#include "Data/dataCase.h"

#include <shlobj.h>
#include <sstream>
#include <KnownFolders.h>
#include <filesystem>

#include <spdlog/sinks/basic_file_sink.h>

std::shared_ptr<spdlog::logger> SHSELogger;
const std::string LoggerName = "SHSE_Logger";

void SaveCallback(SKSE::SerializationInterface* a_intfc)
{
	DBG_MESSAGE("Serialization Save hook called");
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Serialization Save hook");
#endif
	if (!shse::CosaveData::Instance().Serialize(a_intfc))
	{
		REL_ERROR("Cosave data write failed");
		shse::CosaveData::Instance().Clear();
	}
}

void LoadCallback(SKSE::SerializationInterface* a_intfc)
{
	DBG_MESSAGE("Serialization Load hook called");
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Serialization Load hook");
#endif
	if (!shse::CosaveData::Instance().Deserialize(a_intfc))
	{
		REL_ERROR("Cosave data load failed");
	}
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
		// at this point CosaveData contains any saved data, if this was a saved-game load
		const bool onGameReload(msg->type == SKSE::MessagingInterface::kPostLoadGame);
		REL_MESSAGE("Game ready: new game = {}", onGameReload ? "false" : "true");
		// if checks fail, abort scanning
		if (!shse::PluginFacade::Instance().Init(onGameReload))
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
#if _DEBUG
	int MyCrtReportHook(int reportType, char* message, int* returnValue)
	{
		__try {
			RaiseException(EXCEPTION_NONCONTINUABLE_EXCEPTION, EXCEPTION_NONCONTINUABLE, 0, NULL);
		}
		__except (LogStackWalker::LogStack(GetExceptionInformation())) {
			REL_FATALERROR("JSON Collection Definitions threw structured exception");
		}
		return 0;
	}
#endif

bool SKSEPlugin_Query(const SKSE::QueryInterface * a_skse, SKSE::PluginInfo * a_info)
{
#if _DEBUG
	_CrtSetReportHook(MyCrtReportHook);
#endif
	std::filesystem::path logPath(SKSE::log::log_directory());
	try
	{
		std::wstring fileName(logPath.generic_wstring());
		fileName.append(L"/");
		fileName.append(L_SHSE_NAME);
		fileName.append(L".log");
		SHSELogger = spdlog::basic_logger_mt(LoggerName, fileName, true);
		SHSELogger->set_pattern("%Y-%m-%d %T.%e %8l %6t %v");
	}
	catch (const spdlog::spdlog_ex&)
	{
		return false;
	}	
	spdlog::set_level(spdlog::level::trace); // Set global log level
	spdlog::flush_on(spdlog::level::trace);	// always flush
#if 0
#if _DEBUG
	SKSE::add_papyrus_sink();	// TODO what goes in here now
#endif
#endif
	REL_MESSAGE("{} v{}", SHSE_NAME, VersionInfo::Instance().GetPluginVersionString().c_str());

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = SHSE_NAME;
	a_info->version = VersionInfo::Instance().GetVersionMajor();

	if (a_skse->IsEditor()) {
		REL_FATALERROR("Loaded in editor, marking as incompatible");
		return false;
	}
	SKSE::Version runtimeVer(a_skse->RuntimeVersion());
	if (runtimeVer < SKSE::RUNTIME_1_5_73)
	{
		REL_FATALERROR("Unsupported runtime version {}", runtimeVer.GetString());
		return false;
	}
	return true;
}

bool SKSEPlugin_Load(const SKSE::LoadInterface * skse)
{
	REL_MESSAGE("{} plugin loaded", SHSE_NAME);
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
