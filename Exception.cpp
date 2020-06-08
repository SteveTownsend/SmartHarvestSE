#include "PrecompiledHeaders.h"

PluginError::PluginError(const char* pluginName) : std::exception(pluginName)
{
}

KeywordError::KeywordError(const char* keyword) : std::exception(keyword)
{
}
