#pragma once

#include <string>

constexpr char* AHSE_NAME = "AutoHarvestSE";
constexpr wchar_t* L_AHSE_NAME = L"AutoHarvestSE";
constexpr char* MODNAME = "AutoHarvestSE.esp";

class VersionInfo
{
public:
	static VersionInfo& Instance();
	std::string GetPluginVersionString() const;
	UInt32 GetVersionMajor() const;

private:
	static VersionInfo* m_instance;
	void GetPluginVersionInfo();
	std::string m_versionString;
	UInt32 m_majorVersion;
};
