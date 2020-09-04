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
	FileNotFound(const wchar_t* filename);
	FileNotFound(const char* filename);
};
