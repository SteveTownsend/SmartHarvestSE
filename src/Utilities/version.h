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

constexpr const char* SHSE_NAME = "SmartHarvestSE";
constexpr const char* SHSE_PROXY = "SHSE_PluginProxy";
constexpr const wchar_t* L_SHSE_NAME = L"SmartHarvestSE";
constexpr const char* MODNAME = "SmartHarvestSE.esp";
constexpr const char* PRIORNAME = "AutoHarvestSE.esp";
constexpr const char* MOD_AUTHOR = "Steve Townsend";
constexpr const char* MOD_SUPPORT = "SteveTownsend0@gmail.com";

class VersionInfo
{
public:
	static VersionInfo& Instance();
	LPVOID GetModuleFileInfo(const std::wstring& moduleName) const;
	std::string GetExeVersionString() const;
	void FreeVersionInfo() const;
	std::string GetPluginVersionString() const;
	uint32_t GetVersionMajor() const;
	REL::Version GetVersion() const;

private:
	VersionInfo() : m_majorVersion(0) {}
	static VersionInfo* m_instance;
	void GetPluginVersionInfo();
	void SeedModuleVersionInfo(const std::wstring& moduleName) const;
	std::string m_versionString;
	uint32_t m_majorVersion;
	REL::Version m_version;
	mutable std::unique_ptr<BYTE[]> m_verInfo;
};
