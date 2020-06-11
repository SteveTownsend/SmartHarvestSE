#include "nlohmann/json-schema.hpp"
#include <iostream>
#include <fstream>

void ParsePlugin(const nlohmann::json& pluginRule)
{
	std::string plugin(pluginRule.get<std::string>());
}

void ParseKeywords(const nlohmann::json& keywordRule)
{
	std::vector<std::string> keywords;
	keywords.reserve(keywordRule.size());
	std::transform(keywordRule.begin(), keywordRule.end(), std::back_inserter(keywords),
		[&](const nlohmann::json& next) { return next.get<std::string>(); });
}

void ParseSignatures(const nlohmann::json& signatureRule)
{
	std::vector<std::string> signatures;
	signatures.reserve(signatureRule.size());
	std::transform(signatureRule.begin(), signatureRule.end(), std::back_inserter(signatures),
		[&](const nlohmann::json& next) { return next.get<std::string>(); });
}

void ParseLootCategories(const nlohmann::json& lootCategoryRule)
{
	std::vector<std::string> lootCategories;
	lootCategories.reserve(lootCategoryRule.size());
	std::transform(lootCategoryRule.begin(), lootCategoryRule.end(), std::back_inserter(lootCategories),
		[&](const nlohmann::json& next) { return next.get<std::string>(); });
}

void ParseFilter(const nlohmann::json& filter)
{
	std::string op(filter["operator"].get<std::string>());
	for (const auto& condition : filter["conditions"].items())
	{
		if (condition.key() == "subFilters")
		{
			for (const auto& subFilter : condition.value())
			{
				ParseFilter(subFilter);
			}
		}
		else if (condition.key() == "plugin")
		{
			ParsePlugin(condition.value());
		}
		else if (condition.key() == "keywords")
		{
			ParseKeywords(condition.value());
		}
		else if (condition.key() == std::string("signatures"))
		{
			ParseSignatures(condition.value());
		}
		else if (condition.key() == std::string("lootCategories"))
		{
			ParseLootCategories(condition.value());
		}
	}
}

void ParseCollection(const nlohmann::json& collection)
{
	ParseFilter(collection["rootFilter"]);
}

int main(int argc, const char** argv)
{
	// Validate the schema
	const std::string schemaFileName("Schema.json");
    std::ifstream schemaFile(schemaFileName);
	nlohmann::json_schema::json_validator validator;
	try {
		nlohmann::json schema(nlohmann::json::parse(schemaFile));
		validator.set_root_schema(schema); // insert root-schema
	}
	catch (const std::exception& e) {
		std::cerr << "JSON Schema " << schemaFileName << " validation error\n" << e.what() << '/n';
		return -1;
	}
	std::cout << "JSON Schema " << schemaFileName << " parsed and validated\n";

	// Load the data with the validated schema
	const std::string collectionFileName("CollectionDefinition.json");
	std::ifstream collectionFile(collectionFileName);
	nlohmann::json collections;
	try {
		collections = nlohmann::json::parse(collectionFile);
		validator.validate(collections);
	}
	catch (const std::exception& e) {
		std::cerr << "JSON Collections " << collectionFileName << " validation error\n" << e.what() << '/n';
		return -1;
	}

	std::cout << "JSON Collections " << collectionFileName << " parsed and validated\n";

	// walk the tree
	for (const auto& collection : collections["collections"])
	{
		ParseCollection(collection);
	}
	std::cout << "JSON Collections walked OK\n";
	return 0;
}