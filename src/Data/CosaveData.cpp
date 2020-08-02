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
#include "Collections/CollectionManager.h"
#include "Data/CosaveData.h"
#include "Data/LoadOrder.h"
#include "Utilities/utils.h"
#include "WorldState/ActorTracker.h"
#include "WorldState/PartyMembers.h"
#include "WorldState/VisitedPlaces.h"

namespace shse
{

#if 0
constexpr const char* LORDFILE("LORD.compressed.json");
constexpr const char* COLLFILE("COLL.compressed.json");
constexpr const char* PLACFILE("PLAC.compressed.json");
constexpr const char* PRTYFILE("PRTY.compressed.json");
constexpr const char* VCTMFILE("VCTM.compressed.json");
#endif

std::unique_ptr<CosaveData> CosaveData::m_instance;

CosaveData& CosaveData::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<CosaveData>();
	}
	return *m_instance;
}

CosaveData::CosaveData()
{
}

void CosaveData::Clear()
{
	// Clear state that we will reseed. LoadOrder preserves current state and compares to cosave version.
	CollectionManager::Instance().Clear();
	VisitedPlaces::Instance().Reset();
	PartyMembers::Instance().Reset();
	ActorTracker::Instance().ClearVictims();

	RecursiveLockGuard guard(m_cosaveLock);
	m_records.clear();
}

void CosaveData::SeedState()
{
	// iterate records
	// TODO make this atomic if any portion fails
	for (const auto& record : m_records)
	{
		REL_MESSAGE("Seed state from cosave data {}", SerializationRecordName(record.first));
		try {
			switch (record.first) {
			case SerializationRecordType::LoadOrder:
				LoadOrder::Instance().UpdateFrom(record.second);
				break;
			case SerializationRecordType::Collections:
				CollectionManager::Instance().UpdateFrom(record.second);
				break;
			case SerializationRecordType::PlacesVisited:
				VisitedPlaces::Instance().UpdateFrom(record.second);
				break;
			case SerializationRecordType::PartyUpdates:
				PartyMembers::Instance().UpdateFrom(record.second);
				break;
			case SerializationRecordType::Victims:
				ActorTracker::Instance().UpdateFrom(record.second);
				break;
			default:
				break;
			}
		}
		catch (const std::exception& exc) {
			REL_ERROR("Failed to parse cosave {} record:\n{}", SerializationRecordName(record.first), exc.what());
			break;
		}
	}
}

bool CosaveData::Serialize(SKSE::SerializationInterface* intf)
{
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
	std::string record;
	if (!CompressionUtils::EncodeBrotli(shse::LoadOrder::Instance(), record))
	{
		return false;
	}
	if (!intf->WriteRecord('LORD', 1, record.c_str(), static_cast<uint32_t>(record.length())))
	{
		REL_ERROR("Failed to serialize LORD");
		return false;
	}
	else
	{
		REL_MESSAGE("Wrote LORD record {} bytes", record.length());
	}
	// output Collection Groups - Definitions and Members
	if (!CompressionUtils::EncodeBrotli(shse::CollectionManager::Instance(), record))
	{
		return false;
	}
	if (!intf->WriteRecord('COLL', 1, record.c_str(), static_cast<uint32_t>(record.length())))
	{
		REL_ERROR("Failed to serialize COLL");
		return false;
	}
	else
	{
		REL_MESSAGE("Wrote COLL record {} bytes", record.length());
	}
	// output Location history
	if (!CompressionUtils::EncodeBrotli(shse::VisitedPlaces::Instance(), record))
	{
		return false;
	}
	if (!intf->WriteRecord('PLAC', 1, record.c_str(), static_cast<uint32_t>(record.length())))
	{
		REL_ERROR("Failed to serialize PLAC");
		return false;
	}
	else
	{
		REL_MESSAGE("Wrote PLAC record {} bytes", record.length());
	}
	// output Followers-in-Party history
	if (!CompressionUtils::EncodeBrotli(shse::PartyMembers::Instance(), record))
	{
		return false;
	}
	if (!intf->WriteRecord('PRTY', 1, record.c_str(), static_cast<uint32_t>(record.length())))
	{
		REL_ERROR("Failed to serialize PRTY");
		return false;
	}
	else
	{
		REL_MESSAGE("Wrote PRTY record {} bytes", record.length());
	}
	if (!CompressionUtils::EncodeBrotli(shse::ActorTracker::Instance(), record))
	{
		return false;
	}
	if (!intf->WriteRecord('VCTM', 1, record.c_str(), static_cast<uint32_t>(record.length())))
	{
		REL_ERROR("Failed to serialize VCTM");
		return false;
	}
	else
	{
		REL_MESSAGE("Wrote VCTM record {} bytes", record.length());
	}
	return true;
#endif
}

bool CosaveData::Deserialize(SKSE::SerializationInterface* intf)
{
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
	std::string saveData;
	while (intf->GetNextRecordInfo(readType, version, length)) {
		saveData.resize(length);
		if (!intf->ReadRecordData(const_cast<char*>(saveData.c_str()), length))
		{
			REL_ERROR("Failed to load record {}", readType);
			return false;
		}
		shse::SerializationRecordType recordType(shse::SerializationRecordType::MAX);
		switch (readType) {
		case 'LORD':
			// Load Order
			REL_MESSAGE("Read LORD record {} bytes", length);
			recordType = shse::SerializationRecordType::LoadOrder;
			break;
		case 'COLL':
			// Collection Groups - Definitions and Members
			REL_MESSAGE("Read COLL record {} bytes", length);
			recordType = shse::SerializationRecordType::Collections;
			break;
		case 'PLAC':
			// Visited Places
			REL_MESSAGE("Read PLAC record {} bytes", length);
			recordType = shse::SerializationRecordType::PlacesVisited;
			break;
		case 'PRTY':
			// Party Membership
			REL_MESSAGE("Read PRTY record {} bytes", length);
			recordType = shse::SerializationRecordType::PartyUpdates;
			break;
		case 'VCTM':
			// Party Victims
			REL_MESSAGE("Read VCTM record {} bytes", length);
			recordType = shse::SerializationRecordType::Victims;
			break;
		default:
			REL_ERROR("Unrecognized signature type {}", readType);
			break;
		}
		if (recordType != shse::SerializationRecordType::MAX)
		{
			nlohmann::json record;
			if (CompressionUtils::DecodeBrotli(saveData, record))
			{
				m_records.insert({ recordType, record });
			}
			else
			{
				return false;
			}
		}
	}
	return true;
#endif
}

}