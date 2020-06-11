#pragma once

#include <exception>

class PluginError : public std::runtime_error
{
	static constexpr std::string_view ErrorName = "PluginError: ";
public:
	PluginError(const char* pluginName);
};

class KeywordError : public std::runtime_error
{
	static constexpr std::string_view ErrorName = "KeywordError: ";
public:
	KeywordError(const char* keyword);
};

class FileNotFound : public std::runtime_error
{
	static constexpr std::string_view ErrorName = "FileNotFound: ";
public:
	FileNotFound(const char* filename);
};
