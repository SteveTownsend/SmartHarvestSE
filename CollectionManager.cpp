#include "PrecompiledHeaders.h"

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

}
#endif

void CollectionManager::BuildDecisionGraph(void)
{
	for (const auto& definition : m_collectionDefinitions["collections"])
	{
		std::unique_ptr<Condition> filter(ConditionTreeFactory::Instance().ParseTree(definition, 0));
		std::string name(definition["name"]);
		if (m_filterByName.insert(std::make_pair(name, std::move(filter))).second)
		{
#if _DEBUG
			_MESSAGE("Decision Graph built for Collection %s", name.c_str());
#endif
		}
		else
		{
#if _DEBUG
			_MESSAGE("Discarded Decision Graph for duplicate Collection %s", name.c_str());
#endif
		}
	}
#if _DEBUG
	PrintCollections();
#endif
}


