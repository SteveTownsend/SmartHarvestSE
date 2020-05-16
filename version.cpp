#include "PrecompiledHeaders.h"
#include "version.h"

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

UInt32 VersionInfo::GetVersionMajor() const
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
#if _DEBUG
		_MESSAGE("GetFileVersionInfoSize() Failed");
#endif
		return;
	}

	/* Get FileVersionInfo structure */
	verInfo = new BYTE[verInfoLen];
	if (!GetFileVersionInfo(moduleName.c_str(), zero, verInfoLen, verInfo)) {
#if _DEBUG
		_MESSAGE("GetFileVersionInfo() Failed");
#endif
		return;
	}

	/* Query for File version details. */
	if (!VerQueryValue(verInfo, "\\", (LPVOID*)&fileInfo, &len)) {
#if _DEBUG
		_MESSAGE("VerQueryValue() Failed");
#endif
		return;
	}
	m_majorVersion = HIWORD(fileInfo->dwFileVersionMS);
	std::ostringstream version;
	version << m_majorVersion << '.' << LOWORD(fileInfo->dwFileVersionMS) << '.' <<
		HIWORD(fileInfo->dwFileVersionLS) << '.' << LOWORD(fileInfo->dwFileVersionLS);
	m_majorVersion = HIWORD(fileInfo->dwFileVersionMS);
	m_versionString = version.str().c_str();
}

UInt32 GetVersionMajor()
{
}
