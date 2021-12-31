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
#include "Utilities/LogStackWalker.h"
#include "VM/papyrus.h"
#include "Data/CosaveData.h"
#include "Data/dataCase.h"

#include <shlobj.h>
#include <sstream>
#include <KnownFolders.h>
#include <filesystem>

#include <spdlog/sinks/basic_file_sink.h>

#define DLLEXPORT __declspec(dllexport)

std::shared_ptr<spdlog::logger> SHSELogger;
const std::string LoggerName = "SHSE_Logger";
const std::string LogLevelVariable = "SmartHarvestSELogLevel";
void SaveCallback(SKSE::SerializationInterface* a_intfc)
{
	DBG_MESSAGE("Serialization Save hook called");
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Serialization Save hook");
#endif
	if (!shse::CosaveData::Instance().Serialize(a_intfc))
	{
		REL_ERROR("Cosave data write failed");
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
		shse::PluginFacade::Instance().PrepareForReloadOrNewGame();
		break;

	case SKSE::MessagingInterface::kNewGame:
		REL_MESSAGE("New game starting");
		shse::PluginFacade::Instance().PrepareForReloadOrNewGame();
		// fall through to rest of required logic

	case SKSE::MessagingInterface::kPostLoadGame:
		// at this point CosaveData contains any saved data, if this was a saved-game load
		const bool onGameReload(msg->type == SKSE::MessagingInterface::kPostLoadGame);
		REL_MESSAGE("Game ready: new game = {}", onGameReload ? "false" : "true");
		// if checks fail, abort scanning
		if (!shse::PluginFacade::Instance().Init())
		{
			REL_FATALERROR("SearchTask initialization failed - no looting");
			return;
		}
		REL_MESSAGE("Initialized SearchTask, looting available");
		break;
	}
}

#if _DEBUG
int MyCrtReportHook(int, char*, int*)
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

void InitializeDiagnostics()
{
#if _DEBUG
	_CrtSetReportHook(MyCrtReportHook);
	// default Debug log level is TRACE
	spdlog::level::level_enum logLevel(spdlog::level::trace);
#else
	// default Release log level is ERROR
	spdlog::level::level_enum logLevel(spdlog::level::err);
#endif
	char* levelValue;
	size_t requiredSize;
	if (getenv_s(&requiredSize, NULL, 0, LogLevelVariable.c_str()) == 0 && requiredSize > 0)
	{
		levelValue = (char*)malloc((requiredSize + 1) * sizeof(char));
		if (levelValue)
		{
			levelValue[requiredSize] = 0;	// ensure null-terminated
			// Get the value of the LIB environment variable.
			if (getenv_s(&requiredSize, levelValue, requiredSize, LogLevelVariable.c_str()) == 0)
			{
				try
				{
					int envLevel = std::stoi(levelValue);
					if (envLevel >= SPDLOG_LEVEL_TRACE && envLevel <= SPDLOG_LEVEL_OFF)
					{
						logLevel = (spdlog::level::level_enum)envLevel;
					}
				}
				catch (const std::exception&)
				{
				}
			}
		}
	}

	std::filesystem::path logPath(SKSE::log::log_directory().value());
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
	}
	spdlog::set_level(logLevel); // Set global log level
	spdlog::flush_on(logLevel);	// always flush
#if 0
#if _DEBUG
	SKSE::add_papyrus_sink();	// TODO what goes in here now
#endif
#endif

	REL_MESSAGE("{} v{}", SHSE_NAME, VersionInfo::Instance().GetPluginVersionString().c_str());
}

extern "C"
{

#ifndef SKYRIM_AE
bool DLLEXPORT SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	InitializeDiagnostics();

	if (a_skse->IsEditor()) {
		REL_FATALERROR("Loaded in editor, marking as incompatible");
		return false;
	}

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = SHSE_NAME;
	a_info->version = VersionInfo::Instance().GetVersionMajor();

	REL::Version runtimeVer(a_skse->RuntimeVersion());
	if (runtimeVer < SKSE::RUNTIME_1_5_73 || runtimeVer > SKSE::RUNTIME_1_5_97)
	{
		REL_FATALERROR("Unsupported runtime version {}", runtimeVer.string());
		return false;
	}
	return true;
}
#endif

bool DLLEXPORT SKSEPlugin_Load(const SKSE::LoadInterface * skse)
{
#ifdef SKYRIM_AE
	InitializeDiagnostics();

	REL::Version runtimeVer(skse->RuntimeVersion());
	if (runtimeVer < SKSE::RUNTIME_1_6_317)
	{
		REL_FATALERROR("Unsupported runtime version {}", runtimeVer.string());
		return false;
	}
#endif

	REL_MESSAGE("{} plugin loaded", SHSE_NAME);
	SKSE::Init(skse);
	SKSE::GetMessagingInterface()->RegisterListener(SKSEMessageHandler);

	auto serialization = SKSE::GetSerializationInterface();
	serialization->SetUniqueID('SHSE');
	serialization->SetSaveCallback(SaveCallback);
	serialization->SetLoadCallback(LoadCallback);

	return true;
}

#ifdef SKYRIM_AE
// constinit essential because SKSE uses LoadLibraryEX(LOAD_LIBRARY_AS_IMAGE_RESOURCE) - only compile time values work
extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() { {
		SKSE::PluginVersionData v;

		// WET WET WET but less work than injecting Version in the build a la Quick Loot RE
		v.PluginVersion({ 4, 0, 0, 7 });
		v.PluginName(SHSE_NAME);
		v.AuthorName(MOD_AUTHOR);
		v.AuthorEmail(MOD_SUPPORT);

		v.UsesAddressLibrary(true);
		v.CompatibleVersions({ 
			SKSE::RUNTIME_1_5_97,
			SKSE::RUNTIME_1_6_317,
			SKSE::RUNTIME_1_6_318,
			SKSE::RUNTIME_1_6_323,
			SKSE::RUNTIME_1_6_342,
			SKSE::RUNTIME_LATEST });

		return v;
	}
}();
#endif
}
