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

constexpr char* SHSE_NAME = "SmartHarvestSE";
constexpr char* SHSE_PROXY = "SHSE_PluginProxy";
constexpr wchar_t* L_SHSE_NAME = L"SmartHarvestSE";
constexpr char* MODNAME = "SmartHarvestSE.esp";
constexpr char* PRIORNAME = "AutoHarvestSE.esp";

class VersionInfo
{
public:
	static VersionInfo& Instance();
	std::string GetPluginVersionString() const;
	uint32_t GetVersionMajor() const;

private:
	VersionInfo() : m_majorVersion(0) {}
	static VersionInfo* m_instance;
	void GetPluginVersionInfo();
	std::string m_versionString;
	uint32_t m_majorVersion;
};
