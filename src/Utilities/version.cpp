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
#include "PrecompiledHeaders.h"

#include "Utilities/LogWrapper.h"
#include "Utilities/version.h"
#include "Utilities/utils.h"
#include <verrsrc.h>
#include <libloaderapi.h>

#undef GetEnvironmentVariable
#undef GetFileVersionInfo
#undef GetFileVersionInfoSize
#undef GetModuleFileName
#undef GetModuleHandle
#undef LoadLibrary
#undef MessageBox
#undef OutputDebugString
#undef RegQueryValueEx
#undef VerQueryValue

VersionInfo* VersionInfo::m_instance(nullptr);

VersionInfo& VersionInfo::Instance()
{
	if (!m_instance)
	{
		m_instance = new VersionInfo;
		m_instance->GetPluginVersionInfo();
	}
	return *m_instance;
}

std::string VersionInfo::GetPluginVersionString() const
{
	return m_versionString;
}

uint32_t VersionInfo::GetVersionMajor() const
{
	return m_majorVersion;
}

REL::Version VersionInfo::GetVersion() const
{
	return m_version;
}

void VersionInfo::SeedModuleVersionInfo(const std::wstring& moduleName) const
{
	std::uint32_t zero = 0;		// handle bizarro Win API
	DWORD verInfoLen = 0;
	/* Get the size of FileVersionInfo structure */
	verInfoLen = SKSE::WinAPI::GetFileVersionInfoSize(moduleName.c_str(), &zero);
	if (verInfoLen == 0) {
		REL_WARNING("GetFileVersionInfoSize() Failed");
		return;
	}

	/* Get FileVersionInfo structure - we leak this memory: once for this DLL and once for the process */
	m_verInfo.reset(new BYTE[verInfoLen]);
	if (!SKSE::WinAPI::GetFileVersionInfo(moduleName.c_str(), 0, verInfoLen, m_verInfo.get())) {
		REL_WARNING("GetFileVersionInfo() Failed");
		FreeVersionInfo();
	}
}

LPVOID VersionInfo::GetModuleFileInfo(const std::wstring& moduleName) const
{
	SeedModuleVersionInfo(moduleName);
	VS_FIXEDFILEINFO* fileInfo = nullptr;
	UINT len = 0;
	if (m_verInfo)
	{
		/* Query for File version details. */
		if (!SKSE::WinAPI::VerQueryValue(m_verInfo.get(), L"\\", (LPVOID*)&fileInfo, &len)) {
			REL_WARNING("VerQueryValue() Failed");
		}
	}
	return fileInfo;
}

std::string VersionInfo::GetExeVersionString() const
{
	wchar_t exePath[MAX_PATH+1];
	std::string versionString = "unknown";
	DWORD pathLength = SKSE::WinAPI::GetModuleFileName(NULL, exePath, DWORD(MAX_PATH));
	if (pathLength != 0)
	{
		versionString = StringUtils::FromUnicode(std::wstring(exePath, exePath + pathLength));
		SeedModuleVersionInfo(exePath);
		if (m_verInfo)
		{
			LPTSTR lpBuffer;
			UINT len = 0;
			if (!SKSE::WinAPI::VerQueryValue (m_verInfo.get(), TEXT("\\StringFileInfo\\040904B0\\ProductVersion"),
					(LPVOID*) &lpBuffer, &len)) {
				REL_WARNING("ProductVersion: not found");
			}
			else
			{
				std::string productVersion(StringUtils::FromUnicode(std::wstring(lpBuffer, lpBuffer + len - 1)));
				versionString.append(" v");
				versionString.append(productVersion);
			}
		}
		VersionInfo::Instance().FreeVersionInfo();
	}
	return versionString;
}

void VersionInfo::FreeVersionInfo() const
{
	m_verInfo.reset();
}

void VersionInfo::GetPluginVersionInfo()
{
	m_versionString = "unknown";
	m_majorVersion = 0;

	VS_FIXEDFILEINFO* fileInfo = reinterpret_cast<VS_FIXEDFILEINFO*>(GetModuleFileInfo(FileUtils::GetPluginFileName()));
	if (fileInfo)
	{
		m_majorVersion = HIWORD(fileInfo->dwFileVersionMS);
		std::ostringstream version;
		version << m_majorVersion << '.' << LOWORD(fileInfo->dwFileVersionMS) << '.' <<
			HIWORD(fileInfo->dwFileVersionLS) << '.' << LOWORD(fileInfo->dwFileVersionLS);
		m_versionString = version.str();
		m_version = {
			HIWORD(fileInfo->dwFileVersionMS),
			LOWORD(fileInfo->dwFileVersionMS),
			HIWORD(fileInfo->dwFileVersionLS),
			HIWORD(fileInfo->dwFileVersionLS)
		};
	}
	FreeVersionInfo();
}
