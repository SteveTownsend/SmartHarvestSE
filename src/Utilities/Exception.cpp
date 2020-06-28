#include "PrecompiledHeaders.h"

#include "Utilities/Exception.h"

PluginError::PluginError(const char* pluginName) : std::runtime_error(std::string(PluginError::ErrorName) + pluginName)
{
}

KeywordError::KeywordError(const char* keyword) : std::runtime_error(std::string(KeywordError::ErrorName) + keyword)
{
}

FileNotFound::FileNotFound(const char* filename) : std::runtime_error(std::string(FileNotFound::ErrorName) + filename)
{
}
