#include "PrecompiledHeaders.h"
#include "PrecompiledHeaders.h"
#include "LogStackWalker.h"

std::unique_ptr<CollectionManager> CollectionManager::m_instance;

CollectionManager& CollectionManager::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<CollectionManager>();
	}
	return *m_instance;
}

// Generate Collection Definitions from JSON Config
void CollectionManager::ProcessDefinitions(void)
{
	if (!LoadData())
		return;

	__try {
		BuildDecisionTrees();
	}
	__except (LogStackWalker::LogStack(GetExceptionInformation())) {
	}
}

void CollectionManager::EnqueueAddedItems(const std::vector<std::pair<RE::FormID, ObjectType>>& looted)
{
	RecursiveLockGuard guard(m_collectionLock);
	m_addedItemQueue.reserve(m_addedItemQueue.size() + looted.size());
	for (const auto& item : looted)
	{
		EnqueueAddedItem(item.first, item.second);
	}
}

void CollectionManager::EnqueueAddedItem(const RE::FormID formID, const ObjectType objectType)
{
	RecursiveLockGuard guard(m_collectionLock);
	// only pass this along if not already checked
	if (m_checkedItems.insert(formID).second)
	{
		m_addedItemQueue.emplace_back(formID, objectType);
	}
}

void CollectionManager::ProcessAddedItems()
{
	constexpr std::chrono::milliseconds InventoryReconciliationIntervalMillis(5000LL);

	WindowsUtils::ScopedTimer elapsed("Collection checks");
	RecursiveLockGuard guard(m_collectionLock);
	decltype(m_addedItemQueue) queuedItems;
	queuedItems.swap(m_addedItemQueue);
	for (const auto& looted : queuedItems)
	{
#if _DEBUG
		_DMESSAGE("Check collectability of added item 0x%08x", looted.first);
#endif
		AddToRelevantCollections(looted.first, looted.second);
	}

	const auto nowTime(std::chrono::high_resolution_clock::now());
	if (nowTime - m_lastInventoryCheck >= InventoryReconciliationIntervalMillis)
	{
#if _DEBUG
		_DMESSAGE("Inventory reconcilation required");
#endif
		m_lastInventoryCheck = nowTime;
		ReconcileInventory();
	}
}

// bucket newly-received items in any matching collections
void CollectionManager::AddToRelevantCollections(const RE::FormID itemID, const ObjectType objectType)
{
	// resolve ID to Form
	RE::TESForm* form(RE::TESForm::LookupByID(itemID));
	if (!form)
		return;
	for (auto& collection : m_collectionByName)
	{
		if (collection.second->IsMemberOf(form, GetBaseFormObjectType(form, true)))
		{
			// record membership
			collection.second->RecordNewMember(itemID, form);
		}
	}
}

// Player inventory can get objects from Loot menus and other sources than our harvesting, we need to account for them
// We don't do this on every pass as it's a decent amount of work
void CollectionManager::ReconcileInventory()
{
	RE::PlayerCharacter* player(RE::PlayerCharacter::GetSingleton());
	if (!player)
		return;

	auto inv = player->GetInventory();
	if (inv.empty())
		return;
	std::vector<std::pair<RE::FormID, ObjectType>> candidates;
	candidates.reserve(inv.size());
	std::transform(inv.cbegin(), inv.cend(), std::back_inserter(candidates),
		[&](const auto& objectData) { return std::make_pair(objectData.first->GetFormID(), GetBaseFormObjectType(objectData.first, true)); });
	CollectionManager::Instance().EnqueueAddedItems(candidates);
}

bool CollectionManager::LoadData(void)
{
	// Validate the schema
	const std::string schemaFileName("Schema.json");
	std::string filePath(FileUtils::GetPluginPath() + schemaFileName);
	std::ifstream schemaFile(filePath);
	nlohmann::json_schema::json_validator validator;
	try {
		nlohmann::json schema(nlohmann::json::parse(schemaFile));
		validator.set_root_schema(schema); // insert root-schema
	}
	catch (const std::exception& e) {
		_ERROR("JSON Schema %s not loadable, error:\n%s", filePath.c_str(), e.what());
		return false;
	}

#if _DEBUG
	_MESSAGE("JSON Schema %s parsed and validated", filePath.c_str());
#endif

	// Load the Collection Definitions using the validated schema
	const std::string collectionFileName("CollectionDefinition.json");
	filePath = FileUtils::GetPluginPath() + collectionFileName;
	std::ifstream collectionFile(filePath);
	try {
		m_collectionDefinitions = nlohmann::json::parse(collectionFile);
		validator.validate(m_collectionDefinitions);
	}
	catch (const std::exception& e) {
		_ERROR("JSON Collection Definitions %s not loadable, error:\n%s", filePath.c_str(), e.what());
		return false;
	}

#if _DEBUG
	_MESSAGE("JSON Collection Definitions %s parsed and validated", filePath.c_str());
#endif
	return true;
}

#if _DEBUG
void CollectionManager::PrintCollections(void)
{
	for (const auto& collection : m_collectionByName)
	{
		std::ostringstream collectionStr;
		collectionStr << *collection.second;
		_MESSAGE("Collection %s:\n%s", collection.first.c_str(), collectionStr.str().c_str());
	}
}
#endif

void CollectionManager::BuildDecisionTrees(void)
{
	for (const auto& definition : m_collectionDefinitions["collections"])
	{
		try {
			std::unique_ptr<Collection> filter(CollectionFactory::Instance().ParseCollection(definition));
			std::string name(definition["name"].get<std::string>());
			if (m_collectionByName.insert(std::make_pair(name, std::move(filter))).second)
			{
#if _DEBUG
				_MESSAGE("Decision Tree built for Collection %s", name.c_str());
#endif
			}
			else
			{
#if _DEBUG
				_MESSAGE("Discarded Decision Tree for duplicate Collection %s", name.c_str());
#endif
			}
		}
		catch (const std::exception& exc) {
			_ERROR("Error %s loading Collection\n%s", exc.what(), definition.dump(2).c_str());
		}
	}
#if _DEBUG
	PrintCollections();
#endif
}


