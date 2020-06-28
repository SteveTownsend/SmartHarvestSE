#include "PrecompiledHeaders.h"

#include <filesystem>
#include <regex>

#include "LogStackWalker.h"
#include "LocationTracker.h"
#include "EventPublisher.h"

namespace shse
{

std::unique_ptr<CollectionManager> CollectionManager::m_instance;

CollectionManager& CollectionManager::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<CollectionManager>();
	}
	return *m_instance;
}

CollectionManager::CollectionManager() : m_ready(false), m_enabled(false), m_gameTime(0.0)
{
}

// Generate Collection Definitions from JSON Config
void CollectionManager::ProcessDefinitions(void)
{
	// call only once
	if (IsActive())
		return;

	__try {
		if (!LoadData())
			return;

		// data validated and loaded
		m_ready = true;
	}
	__except (LogStackWalker::LogStack(GetExceptionInformation())) {
		REL_FATALERROR("JSON Collection Definitions threw structured exception");
	}
}

void CollectionManager::Refresh() const
{
	// request added items and game time to be pushed to us while we are sleeping
	if (IsActive())
		EventPublisher::Instance().TriggerFlushAddedItems();
}

void CollectionManager::UpdateGameTime(const float gameTime)
{
	RecursiveLockGuard guard(m_collectionLock);
	m_gameTime = gameTime;
}

void CollectionManager::CheckEnqueueAddedItem(const RE::FormID formID)
{
	if (!IsActive())
		return;
	RecursiveLockGuard guard(m_collectionLock);
	// only pass this along if it is in >= 1 collection
	if (m_collectionsByFormID.contains(formID))
	{
		EnqueueAddedItem(formID);
	}
}

void CollectionManager::EnqueueAddedItem(const RE::FormID formID)
{
	m_addedItemQueue.emplace_back(formID);
}

void CollectionManager::ProcessAddedItems()
{
	if (!IsActive())
		return;

#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("Collection checks");
#endif
	RecursiveLockGuard guard(m_collectionLock);
	constexpr std::chrono::milliseconds InventoryReconciliationIntervalMillis(3000LL);
	const auto nowTime(std::chrono::high_resolution_clock::now());
	if (nowTime - m_lastInventoryCheck >= InventoryReconciliationIntervalMillis)
	{
		DBG_MESSAGE("Inventory reconciliation required");
		m_lastInventoryCheck = nowTime;
		const auto inventoryAdds(ReconcileInventory());
		if (!inventoryAdds.empty())
		{
			m_addedItemQueue.insert(m_addedItemQueue.end(), inventoryAdds.cbegin(), inventoryAdds.cend());
		}
	}

	decltype(m_addedItemQueue) queuedItems;
	queuedItems.swap(m_addedItemQueue);
	for (const auto formID : queuedItems)
	{
		// only process items known to be a member of at least one collection
		if (m_collectionsByFormID.contains(formID))
		{
			DBG_VMESSAGE("Check collectability of added item 0x%08x", formID);
			AddToRelevantCollections(formID);
		}
		else if (m_nonCollectionForms.insert(formID).second)
		{
			DBG_VMESSAGE("Recorded 0x%08x as non-collectible", formID);
		}
	}
}

// bucket newly-received items in any matching collections
void CollectionManager::AddToRelevantCollections(const RE::FormID itemID)
{
	// resolve ID to Form
	RE::TESForm* form(RE::TESForm::LookupByID(itemID));
	if (!form)
		return;
	RecursiveLockGuard guard(m_collectionLock);
	const auto targets(m_collectionsByFormID.equal_range(form->GetFormID()));
	for (auto collection = targets.first; collection != targets.second; ++collection)
	{
		if (collection->second->IsMemberOf(form))
		{
			// record membership
			collection->second->RecordItem(itemID, form, m_gameTime, LocationTracker::Instance().CurrentPlayerPlace());
		}
	}
}

std::pair<bool, SpecialObjectHandling> CollectionManager::IsCollectible(const RE::TESForm* form)
{
	if (!IsActive() || !form)
		return NotCollectible;
	RecursiveLockGuard guard(m_collectionLock);
	if (m_nonCollectionForms.contains(form->GetFormID()))
		return NotCollectible;

	// find Collections that match this Form
	const auto targets(m_collectionsByFormID.equal_range(form->GetFormID()));
	if (targets.first == m_collectionsByFormID.cend())
	{
		DBG_VMESSAGE("Record %s/0x%08x as non-collectible", form->GetName(), form->GetFormID());
		m_nonCollectionForms.insert(form->GetFormID());
		return NotCollectible;
	}

	// It is in at least one collection. Find the most aggressive action.
	SpecialObjectHandling action(SpecialObjectHandling::DoNotLoot);
	bool collect(false);
	for (auto collection = targets.first; collection != targets.second; ++collection)
	{
		if (collection->second->IsCollectibleFor(form))
		{
			collect = true;
			action = UpdateSpecialObjectHandling(collection->second->Policy().Action(), action);
		}
	}
	return collect ? std::make_pair(true, action) : NotCollectible;
}

// Player inventory can get objects from Loot menus and other sources than our harvesting, we need to account for them
// We don't do this on every pass as it's a decent amount of work
std::vector<RE::FormID> CollectionManager::ReconcileInventory()
{
	RE::PlayerCharacter* player(RE::PlayerCharacter::GetSingleton());
	if (!player)
		return std::vector<RE::FormID>();

	// use delta vs last pass to speed this up (resets on game reload)
	decltype(m_lastInventoryItems) newInventoryItems;
	std::vector<RE::FormID> candidates;
	const auto inv = player->GetInventory([&](RE::TESBoundObject* candidate) -> bool {
		RE::FormID formID(candidate->GetFormID());
		newInventoryItems.insert(formID);
		if (!m_lastInventoryItems.contains(formID) && m_collectionsByFormID.contains(formID))
		{
			DBG_VMESSAGE("Collectible %s/0x%08x new in inventory", candidate->GetName(), formID);
			candidates.push_back(formID);
		}
		return false;
	});
	m_lastInventoryItems.swap(newInventoryItems);
	return candidates;
}

bool CollectionManager::LoadCollectionsFromFile(
	const std::filesystem::path& defFile, const std::string& groupName, nlohmann::json_schema::json_validator& validator)
{
	try {
		std::ifstream collectionFile(defFile);
		if (collectionFile.fail()) {
			throw FileNotFound(defFile.generic_string().c_str());
		}
		nlohmann::json collectionDefinitions(nlohmann::json::parse(collectionFile));
		validator.validate(collectionDefinitions);
		BuildDecisionTrees(collectionDefinitions, groupName);
		m_fileNamesByGroupName.insert(std::make_pair(groupName, defFile.string()));
		return true;
	}
	catch (const std::exception& e) {
		REL_ERROR("JSON Collection Definitions %s not loadable, error:\n%s", defFile.generic_string().c_str(), e.what());
		return false;
	}
}

bool CollectionManager::LoadData(void)
{
	// Validate the schema
	const std::string schemaFileName("SHSE.SchemaCollections.json");
	std::string filePath(FileUtils::GetPluginPath() + schemaFileName);
	nlohmann::json_schema::json_validator validator;
	try {
		std::ifstream schemaFile(filePath);
		if (schemaFile.fail()) {
			throw FileNotFound(filePath.c_str());
		}
		nlohmann::json schema(nlohmann::json::parse(schemaFile));
		validator.set_root_schema(schema); // insert root-schema
	}
	catch (const std::exception& e) {
		REL_ERROR("JSON Schema %s not loadable, error:\n%s", filePath.c_str(), e.what());
		return false;
	}

	REL_MESSAGE("JSON Schema %s parsed and validated", filePath.c_str());

	// Find and Load Collection Definitions using the validated schema
	const std::regex collectionsFilePattern("SHSE.Collections\\.(.*)\\.json");
	for (const auto& nextFile : std::filesystem::directory_iterator(FileUtils::GetPluginPath()))
	{
		if (!std::filesystem::is_regular_file(nextFile))
		{
			DBG_MESSAGE("Skip %s, not a regular file", nextFile.path().generic_string().c_str());
			continue;
		}
		std::string fileName(nextFile.path().filename().generic_string());
		std::smatch matches;
		if (!std::regex_search(fileName, matches, collectionsFilePattern))
		{
			DBG_MESSAGE("Skip %s, does not match Collections filename pattern", fileName.c_str());
				continue;
		}
		// capture string at index 1 is the Collection Name, always present after a regex match
		REL_MESSAGE("Load JSON Collection Definitions %s for Group %s", fileName.c_str(), matches[1].str().c_str());
		if (LoadCollectionsFromFile(nextFile, matches[1].str(), validator))
		{
			REL_MESSAGE("JSON Collection Definitions %s/%s parsed and validated", fileName.c_str(), matches[1].str().c_str());
		}
	}
	PrintDefinitions();
	ResolveMembership();
	return true;
}

void CollectionManager::PrintDefinitions(void) const
{
	for (const auto& collection : m_allCollectionsByLabel)
	{
		REL_MESSAGE("Collection %s:\n%s", collection.first.c_str(), collection.second->PrintDefinition().c_str());
	}
}

void CollectionManager::PrintMembership(void) const
{
	for (const auto& collection : m_allCollectionsByLabel)
	{
		REL_MESSAGE("Collection %s:\n%s", collection.first.c_str(), collection.second->PrintMembers().c_str());
	}
}

int CollectionManager::NumberOfFiles(void) const
{
	RecursiveLockGuard guard(m_collectionLock);
	return static_cast<int>(m_fileNamesByGroupName.size());
}

std::string CollectionManager::GroupNameByIndex(const int fileIndex) const
{
	RecursiveLockGuard guard(m_collectionLock);
	size_t index(0);
	for (const auto& group : m_fileNamesByGroupName)
	{
		if (index == fileIndex)
			return group.first;
		++index;
	}
	return std::string();
}

std::string CollectionManager::GroupFileByIndex(const int fileIndex) const
{
	RecursiveLockGuard guard(m_collectionLock);
	size_t index(0);
	for (const auto& group : m_fileNamesByGroupName)
	{
		if (index == fileIndex)
			return group.second;
		++index;
	}
	return std::string();
}

int CollectionManager::NumberOfCollections(const std::string& groupName) const
{
	RecursiveLockGuard guard(m_collectionLock);
	return static_cast<int>(m_collectionsByGroupName.count(groupName));
}

std::string CollectionManager::NameByGroupIndex(const std::string& groupName, const int collectionIndex) const
{
	RecursiveLockGuard guard(m_collectionLock);
	const auto matches(m_collectionsByGroupName.equal_range(groupName));
	size_t index(0);
	for (auto group = matches.first; group != matches.second; ++group)
	{
		if (index == collectionIndex)
		{
			// strip the group name and separator
			return group->second.substr(groupName.length() + 1);
		}
		++index;
	}
	return std::string();
}

std::string CollectionManager::MakeLabel(const std::string& groupName, const std::string& collectionName)
{
	std::ostringstream labelStream;
	labelStream << groupName << '/' << collectionName;
	return labelStream.str();
}

bool CollectionManager::PolicyRepeat(const std::string& groupName, const std::string& collectionName) const
{
	const std::string label(MakeLabel(groupName, collectionName));
	RecursiveLockGuard guard(m_collectionLock);
	const auto matched(m_allCollectionsByLabel.find(label));
	if (matched != m_allCollectionsByLabel.cend())
	{
		return matched->second->Policy().Repeat();
	}
	return false;
}

bool CollectionManager::PolicyNotify(const std::string& groupName, const std::string& collectionName) const
{
	const std::string label(MakeLabel(groupName, collectionName));
	RecursiveLockGuard guard(m_collectionLock);
	const auto matched(m_allCollectionsByLabel.find(label));
	if (matched != m_allCollectionsByLabel.cend())
	{
		return matched->second->Policy().Notify();
	}
	return false;
}

SpecialObjectHandling CollectionManager::PolicyAction(const std::string& groupName, const std::string& collectionName) const
{
	const std::string label(MakeLabel(groupName, collectionName));
	RecursiveLockGuard guard(m_collectionLock);
	const auto matched(m_allCollectionsByLabel.find(label));
	if (matched != m_allCollectionsByLabel.cend())
	{
		return matched->second->Policy().Action();
	}
	return SpecialObjectHandling::DoNotLoot;
}

void CollectionManager::PolicySetRepeat(const std::string& groupName, const std::string& collectionName, const bool allowRepeats)
{
	const std::string label(MakeLabel(groupName, collectionName));
	RecursiveLockGuard guard(m_collectionLock);
	auto matched(m_allCollectionsByLabel.find(label));
	if (matched != m_allCollectionsByLabel.cend())
	{
		matched->second->Policy().SetRepeat(allowRepeats);
	}
}

void CollectionManager::PolicySetNotify(const std::string& groupName, const std::string& collectionName, const bool notify)
{
	const std::string label(MakeLabel(groupName, collectionName));
	RecursiveLockGuard guard(m_collectionLock);
	auto matched(m_allCollectionsByLabel.find(label));
	if (matched != m_allCollectionsByLabel.cend())
	{
		return matched->second->Policy().SetNotify(notify);
	}
}

void CollectionManager::PolicySetAction(const std::string& groupName, const std::string& collectionName, const SpecialObjectHandling action)
{
	const std::string label(MakeLabel(groupName, collectionName));
	RecursiveLockGuard guard(m_collectionLock);
	auto matched(m_allCollectionsByLabel.find(label));
	if (matched != m_allCollectionsByLabel.cend())
	{
		matched->second->Policy().SetAction(action);
	}
}

size_t CollectionManager::TotalItems(const std::string& groupName, const std::string& collectionName) const
{
	const std::string label(MakeLabel(groupName, collectionName));
	RecursiveLockGuard guard(m_collectionLock);
	const auto matched(m_allCollectionsByLabel.find(label));
	if (matched != m_allCollectionsByLabel.cend())
	{
		return matched->second->Count();
	}
	return 0;
}

size_t CollectionManager::ItemsObtained(const std::string& groupName, const std::string& collectionName) const
{
	const std::string label(MakeLabel(groupName, collectionName));
	RecursiveLockGuard guard(m_collectionLock);
	const auto matched(m_allCollectionsByLabel.find(label));
	if (matched != m_allCollectionsByLabel.cend())
	{
		return matched->second->Observed();
	}
	return 0;
}

void CollectionManager::BuildDecisionTrees(nlohmann::json& collectionDefinitions, const std::string& groupName)
{
	for (const auto& definition : collectionDefinitions["collections"])
	{
		try {
			std::shared_ptr<Collection> filter(CollectionFactory::Instance().ParseCollection(definition));
			const std::string label(MakeLabel(groupName, definition["name"].get<std::string>()));
			if (m_allCollectionsByLabel.insert(std::make_pair(label, filter)).second)
			{
				REL_MESSAGE("Decision Tree built for Collection %s", label.c_str());
				m_collectionsByGroupName.insert(std::make_pair(groupName, label));
			}
			else
			{
				REL_WARNING("Discarded Decision Tree for duplicate Collection %s", label.c_str());
			}
		}
		catch (const std::exception& exc) {
			REL_ERROR("Error %s building Decision Tree for Collection\n%s", exc.what(), definition.dump(2).c_str());
		}
	}
}

void CollectionManager::ResolveMembership(void)
{
	for (const auto& signature : SignatureCondition::ValidSignatures())
	{
		for (const auto form : RE::TESDataHandler::GetSingleton()->GetFormArray(signature.second))
		{
			for (const auto& collection : m_allCollectionsByLabel)
			{
				// record collection membership for any that match this object - ignore whitelist
				if (collection.second->MatchesFilter(form))
				{
					DBG_VMESSAGE("Record %s/0x%08x as collectible", form->GetName(), form->GetFormID());
					m_collectionsByFormID.insert(std::make_pair(form->GetFormID(), collection.second));
					collection.second->AddMemberID(form);
				}
			}
		}
	}
	PrintMembership();
}

// for game reload, we reset the checked items
// TODO process SKSE co-save data
void CollectionManager::OnGameReload()
{
	RecursiveLockGuard guard(m_collectionLock);
	/// reset player inventory last-known-good
	m_lastInventoryItems.clear();

	// logic depends on prior and new state
	bool wasEnabled(m_enabled);
	m_enabled = INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::common, INIFile::SecondaryType::config, "CollectionsEnabled") != 0.;
	REL_MESSAGE("Collections are %s", m_enabled ? "enabled" : "disabled");
	if (m_enabled)
	{
		// TODO load Collections data from saved game
		// Flush membership state to allow testing
		for (auto collection : m_allCollectionsByLabel)
		{
			collection.second->Reset();
		}
	}
	else
	{
		// TODO maybe more state to clean out
	}
}

}
