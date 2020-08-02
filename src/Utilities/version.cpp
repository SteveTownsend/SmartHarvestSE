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

#include "Utilities/version.h"
#include "Utilities/utils.h"

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

void VersionInfo::GetPluginVersionInfo()
{
	m_versionString = "unknown";
	m_majorVersion = -1;

	std::string moduleName = FileUtils::GetPluginFileName();
	DWORD zero = 0;		// handle bizarro Win API
	DWORD verInfoLen = 0;
	BYTE* verInfo = NULL;
	VS_FIXEDFILEINFO* fileInfo = NULL;
	UINT len = 0;

	/* Get the size of FileVersionInfo structure */
	verInfoLen = GetFileVersionInfoSize(moduleName.c_str(), &zero);
	if (verInfoLen == 0) {
		REL_WARNING("GetFileVersionInfoSize() Failed");
		return;
	}

	/* Get FileVersionInfo structure */
	verInfo = new BYTE[verInfoLen];
	if (!GetFileVersionInfo(moduleName.c_str(), 0, verInfoLen, verInfo)) {
		REL_WARNING("GetFileVersionInfo() Failed");
		return;
	}

	/* Query for File version details. */
	if (!VerQueryValue(verInfo, "\\", (LPVOID*)&fileInfo, &len)) {
		REL_WARNING("VerQueryValue() Failed");
		return;
	}
	m_majorVersion = HIWORD(fileInfo->dwFileVersionMS);
	std::ostringstream version;
	version << m_majorVersion << '.' << LOWORD(fileInfo->dwFileVersionMS) << '.' <<
		HIWORD(fileInfo->dwFileVersionLS) << '.' << LOWORD(fileInfo->dwFileVersionLS);
	m_majorVersion = HIWORD(fileInfo->dwFileVersionMS);
	m_versionString = version.str().c_str();
}
