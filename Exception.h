#pragma once

#include <exception>

class PluginError : public std::exception
{
public:
	PluginError(const char* pluginName);
};

class KeywordError : public std::exception
{
public:
	KeywordError(const char* keyword);
};
