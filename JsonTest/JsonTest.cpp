#include "nlohmann/json-schema.hpp"
#include <iostream>
#include <fstream>

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
	try {
		nlohmann::json collections(nlohmann::json::parse(collectionFile));
		validator.validate(collections);
	}
	catch (const std::exception& e) {
		std::cerr << "JSON Collections " << collectionFileName << " validation error\n" << e.what() << '/n';
		return -1;
	}

	std::cout << "JSON Collections " << collectionFileName << " parsed and validated\n";
	return 0;
}