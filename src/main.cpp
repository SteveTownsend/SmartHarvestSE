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
#include "Collections/CollectionManager.h"
#include "WorldState/VisitedPlaces.h"
#include "WorldState/PartyMembers.h"
#include "WorldState/ActorTracker.h"

#include <shlobj.h>
#include <sstream>
#include <KnownFolders.h>
#include <filesystem>

#include <spdlog/sinks/basic_file_sink.h>

std::shared_ptr<spdlog::logger> SHSELogger;
const std::string LoggerName = "SHSE_Logger";

#if 0
constexpr const char* LORDFILE("LORD.compressed.json");
constexpr const char* COLLFILE("COLL.compressed.json");
constexpr const char* PLACFILE("PLAC.compressed.json");
constexpr const char* PRTYFILE("PRTY.compressed.json");
constexpr const char* VCTMFILE("VCTM.compressed.json");
#endif

void SaveCallback(SKSE::SerializationInterface* a_intfc)
{
	DBG_MESSAGE("Serialization Save hook called");
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Serialization Save hook");
#endif
	// Serialize JSON and compress per https://github.com/google/brotli
#if 0
	// output LoadOrder
	{
		nlohmann::json j(shse::LoadOrder::Instance());
		DBG_MESSAGE("Wrote {} :\n{}", LORDFILE, j.dump().c_str());
		std::string compressed(CompressionUtils::EncodeBrotli(j));
		std::ofstream saveData(LORDFILE, std::ios::out | std::ios::binary);
		saveData.write(compressed.c_str(), compressed.length());
		saveData.close();
	}
	// output Collection Groups - Definitions and Members
	{
		nlohmann::json j(shse::CollectionManager::Instance());
		DBG_MESSAGE("Wrote {} :\n{}", COLLFILE, j.dump().c_str());
		std::string compressed(CompressionUtils::EncodeBrotli(j));
		std::ofstream saveData(COLLFILE, std::ios::out | std::ios::binary);
		saveData.write(compressed.c_str(), compressed.length());
		saveData.close();
	}
	// output Location history
	{
		nlohmann::json j(shse::VisitedPlaces::Instance());
		DBG_MESSAGE("Wrote {} :\n{}", PLACFILE, j.dump().c_str());
		std::string compressed(CompressionUtils::EncodeBrotli(j));
		std::ofstream saveData(PLACFILE, std::ios::out | std::ios::binary);
		saveData.write(compressed.c_str(), compressed.length());
		saveData.close();
	}
	// output Followers-in-Party history
	{
		nlohmann::json j(shse::PartyMembers::Instance());
		DBG_MESSAGE("Wrote {} :\n{}", PRTYFILE, j.dump().c_str());
		std::string compressed(CompressionUtils::EncodeBrotli(j));
		std::ofstream saveData(PRTYFILE, std::ios::out | std::ios::binary);
		saveData.write(compressed.c_str(), compressed.length());
		saveData.close();
	}
	// output Party Kills history
	{
		nlohmann::json j(shse::ActorTracker::Instance());
		DBG_MESSAGE("Wrote {} :\n{}", VCTMFILE, j.dump().c_str());
		std::string compressed(CompressionUtils::EncodeBrotli(j));
		std::ofstream saveData(VCTMFILE, std::ios::out | std::ios::binary);
		saveData.write(compressed.c_str(), compressed.length());
		saveData.close();
	}
#else
	// output LoadOrder
	std::string lordRecord(CompressionUtils::EncodeBrotli(shse::LoadOrder::Instance()));
	if (!a_intfc->WriteRecord('LORD', 1, lordRecord.c_str(), static_cast<uint32_t>(lordRecord.length())))
	{
		REL_ERROR("Failed to serialize LORD");
	}
	// output Collection Groups - Definitions and Members
	std::string collRecord(CompressionUtils::EncodeBrotli(shse::CollectionManager::Instance()));
	if (!a_intfc->WriteRecord('COLL', 1, collRecord.c_str(), static_cast<uint32_t>(collRecord.length())))
	{
		REL_ERROR("Failed to serialize COLL");
	}
	// output Location history
	std::string placRecord(CompressionUtils::EncodeBrotli(shse::VisitedPlaces::Instance()));
	if (!a_intfc->WriteRecord('PLAC', 1, placRecord.c_str(), static_cast<uint32_t>(placRecord.length())))
	{
		REL_ERROR("Failed to serialize PLAC");
	}
	// output Followers-in-Party history
	std::string prtyRecord(CompressionUtils::EncodeBrotli(shse::PartyMembers::Instance()));
	if (!a_intfc->WriteRecord('PRTY', 1, prtyRecord.c_str(), static_cast<uint32_t>(prtyRecord.length())))
	{
		REL_ERROR("Failed to serialize PRTY");
	}
	std::string vctmRecord(CompressionUtils::EncodeBrotli(shse::ActorTracker::Instance()));
	if (!a_intfc->WriteRecord('VCTM', 1, vctmRecord.c_str(), static_cast<uint32_t>(vctmRecord.length())))
	{
		REL_ERROR("Failed to serialize VCTM");
	}
#endif
}

void LoadCallback(SKSE::SerializationInterface* a_intfc)
{
	DBG_MESSAGE("Serialization Load hook called");
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Serialization Load hook");
#endif
#if 0
	try {
		// decompress per https://github.com/google/brotli and rehydrate to JSON
		size_t fileSize(std::filesystem::file_size(LORDFILE));
		std::ifstream readData(LORDFILE, std::ios::in | std::ios::binary);
		std::string roundTrip(fileSize, 0);
		readData.read(const_cast<char*>(roundTrip.c_str()), roundTrip.length());
		nlohmann::json jRead(CompressionUtils::DecodeBrotli(roundTrip));
		DBG_MESSAGE("Read {}:\n{}", LORDFILE, jRead.dump().c_str());
	}
	catch (const std::exception& exc)
	{
		DBG_ERROR("Load error on {}: {}", LORDFILE, exc.what());
	}
	try {
		// decompress per https://github.com/google/brotli and rehydrate to JSON
		size_t fileSize(std::filesystem::file_size(COLLFILE));
		std::ifstream readData(COLLFILE, std::ios::in | std::ios::binary);
		std::string roundTrip(fileSize, 0);
		readData.read(const_cast<char*>(roundTrip.c_str()), roundTrip.length());
		nlohmann::json jRead(CompressionUtils::DecodeBrotli(roundTrip));
		DBG_MESSAGE("Read {}:\n{}", COLLFILE, jRead.dump().c_str());
	}
	catch (const std::exception& exc)
	{
		DBG_ERROR("Load error on {}: {}", COLLFILE, exc.what());
	}
	try {
		// decompress per https://github.com/google/brotli and rehydrate to JSON
		size_t fileSize(std::filesystem::file_size(PLACFILE));
		std::ifstream readData(PLACFILE, std::ios::in | std::ios::binary);
		std::string roundTrip(fileSize, 0);
		readData.read(const_cast<char*>(roundTrip.c_str()), roundTrip.length());
		nlohmann::json jRead(CompressionUtils::DecodeBrotli(roundTrip));
		DBG_MESSAGE("Read {}:\n{}", PLACFILE, jRead.dump().c_str());
	}
	catch (const std::exception& exc)
	{
		DBG_ERROR("Load error on {}: {}", PLACFILE, exc.what());
	}
	try {
		// decompress per https://github.com/google/brotli and rehydrate to JSON
		size_t fileSize(std::filesystem::file_size(PRTYFILE));
		std::ifstream readData(PRTYFILE, std::ios::in | std::ios::binary);
		std::string roundTrip(fileSize, 0);
		readData.read(const_cast<char*>(roundTrip.c_str()), roundTrip.length());
		nlohmann::json jRead(CompressionUtils::DecodeBrotli(roundTrip));
		DBG_MESSAGE("Read {}:\n{}", PRTYFILE, jRead.dump().c_str());
	}
	catch (const std::exception& exc)
	{
		DBG_ERROR("Load error on {}: {}", PRTYFILE, exc.what());
	}
	try {
		// decompress per https://github.com/google/brotli and rehydrate to JSON
		size_t fileSize(std::filesystem::file_size(VCTMFILE));
		std::ifstream readData(VCTMFILE, std::ios::in | std::ios::binary);
		std::string roundTrip(fileSize, 0);
		readData.read(const_cast<char*>(roundTrip.c_str()), roundTrip.length());
		nlohmann::json jRead(CompressionUtils::DecodeBrotli(roundTrip));
		DBG_MESSAGE("Read {}:\n{}", VCTMFILE, jRead.dump().c_str());
	}
	catch (const std::exception& exc)
	{
		DBG_ERROR("Load error on {}: {}", VCTMFILE, exc.what());
	}
#else
	uint32_t readType;
	uint32_t version;
	uint32_t length;
	std::unordered_map<shse::SerializationRecordType, nlohmann::json> records;
	std::string saveData;
	while (a_intfc->GetNextRecordInfo(readType, version, length)) {
		saveData.resize(length);
		if (!a_intfc->ReadRecordData(const_cast<char*>(saveData.c_str()), length))
		{
			REL_ERROR("Failed to load record {}", readType);
		}
		shse::SerializationRecordType recordType(shse::SerializationRecordType::MAX);
		switch (readType) {
		case 'LORD':
			// Load Order
			recordType = shse::SerializationRecordType::LoadOrder;
			break;
		case 'COLL':
			// Collection Groups - Definitions and Members
			recordType = shse::SerializationRecordType::Collections;
			break;
		case 'PLAC':
			// Visited Places
			recordType = shse::SerializationRecordType::PlacesVisited;
			break;
		case 'PRTY':
			// Party Membership
			recordType = shse::SerializationRecordType::PartyUpdates;
			break;
		case 'VCTM':
			// Party Victims
			recordType = shse::SerializationRecordType::Victims;
			break;
		default:
			REL_ERROR("Unrecognized signature type {}", readType);
			break;
		}
		if (recordType != shse::SerializationRecordType::MAX)
		{
			records.insert({ recordType, CompressionUtils::DecodeBrotli(saveData) });
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
	std::filesystem::path logPath(SKSE::log::log_directory());
	try
	{
		std::string fileName(logPath.generic_string());
		fileName.append("/");
		fileName.append(SHSE_NAME);
		fileName.append(".log");
		SHSELogger = spdlog::basic_logger_mt(LoggerName, fileName, true);
		SHSELogger->set_pattern("%Y-%m-%d %T.%e %8l %6t %v");
	}
	catch (const spdlog::spdlog_ex&)
	{
		return false;
	}	
	spdlog::set_level(spdlog::level::trace); // Set global log level
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
