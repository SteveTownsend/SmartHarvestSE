#include "PrecompiledHeaders.h"

#include "CommonLibSSE/include/RE/Misc.h"
#include "CommonLibSSE/include/RE/Setting.h"

#include "utils.h"

#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm> //Trim
#include <math.h>	// pow


namespace FileUtils
{
	std::string GetGamePath(void)
	{
		static std::string s_runtimeDirectory;
		if (s_runtimeDirectory.empty())
		{
			char	runtimePathBuf[MAX_PATH];
			UInt32	runtimePathLength = GetModuleFileName(GetModuleHandle(NULL), runtimePathBuf, sizeof(runtimePathBuf));
			if (runtimePathLength && (runtimePathLength < sizeof(runtimePathBuf)))
			{
				std::string	runtimePath(runtimePathBuf, runtimePathLength);
				std::string::size_type	lastSlash = runtimePath.rfind('\\');
				if (lastSlash != std::string::npos)
					s_runtimeDirectory = runtimePath.substr(0, lastSlash + 1);
			}
#if _DEBUG
			_MESSAGE("GetGamePath result: %s", s_runtimeDirectory.c_str());
#endif
		}
		return s_runtimeDirectory;
	}

	std::string GetDataPath()
	{
		static std::string s_dataDirectory;
		if (s_dataDirectory.empty())
		{
			s_dataDirectory = GetGamePath();
			s_dataDirectory += "data\\";
#if _DEBUG
			_MESSAGE("GetDataPath result: %s", s_dataDirectory.c_str());
#endif
		}
		return s_dataDirectory;
	}

	std::string GetPluginFileName()
	{
		static std::string s_pluginFileName;
		if (s_pluginFileName.empty())
		{
			HMODULE hm = NULL;
			char path[MAX_PATH];
			if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
				GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
				(LPCSTR)&GetPluginPath, &hm) == 0)
			{
				int ret = GetLastError();
#if _DEBUG
				_MESSAGE("GetModuleHandle failed, error = %d\n", ret);
#endif
			}
			else if (GetModuleFileName(hm, path, sizeof(path)) == 0)
			{
				int ret = GetLastError();
#if _DEBUG
				_MESSAGE("GetModuleFileName failed, error = %d\n", ret);
#endif
			}
			else
			{
				s_pluginFileName = path;
			}
		}
		return s_pluginFileName;
	}

	std::string GetPluginPath()
	{
		static std::string s_skseDirectory;
		if (s_skseDirectory.empty())
		{
			std::string pluginFileName(GetPluginFileName());
			if (!pluginFileName.empty())
			{
			    char drive[MAX_PATH];
			    char dir[MAX_PATH];
				if (_splitpath_s(pluginFileName.c_str(), drive, MAX_PATH, dir, MAX_PATH, nullptr, 0, nullptr, 0) == 0)
				{
					char path[MAX_PATH];
      			    if (_makepath_s(path, drive, dir, nullptr, nullptr) == 0)
					{
					    s_skseDirectory = path;
#if _DEBUG
						_MESSAGE("GetPluginPath result: %s", s_skseDirectory.c_str());
#endif
					}
				}
			}
		}
		return s_skseDirectory;
	}

	bool IsFoundFile(const char* fileName)
	{
		std::ifstream ifs(fileName);
		return (ifs.fail()) ? false : true;
	}

	bool WriteSectionKey(LPCTSTR section_name, LPCTSTR key_name, LPCTSTR key_data, LPCTSTR ini_file_path)
	{
		return (WritePrivateProfileString(section_name, key_name, key_data, ini_file_path)) != 0;
	}

	std::vector<std::string> GetSectionKeys(LPCTSTR section_name, LPCTSTR ini_file_path)
	{
		std::vector<std::string> result;
		std::string file_path(ini_file_path);
		if (IsFoundFile(ini_file_path))
		{
			TCHAR buf[32768] = {};
			GetPrivateProfileSection(section_name, buf, sizeof(buf), ini_file_path);
			for (LPTSTR seek = buf; *seek != '\0'; seek++)
			{
				std::string str(seek);
				result.push_back(str);
				while (*seek != '\0')
					seek++;
			}
		}
		return result;
	}

	std::vector<std::string> GetIniKeys(std::string section, std::string fileName)
	{
		std::vector<std::string> result;
		std::string filePath = GetPluginPath();
		filePath += fileName;
		if (IsFoundFile(filePath.c_str()))
		{
			std::vector<std::string> list = GetSectionKeys(section.c_str(), filePath.c_str());
			for (std::string str : list)
			{
				auto vec = StringUtils::Split(str, '=');
				if (!vec.empty())
				{
					std::string key = vec.at(0);
					UInt32 value = std::atoi(vec.at(1).c_str());
					if (!key.empty() && value >= 1)
						result.push_back(key.c_str());
				}
			}
		}
		return result;
	}
}

namespace utils
{
	void SetGoldValue(RE::TESForm* pForm, UInt32 value)
	{
		if (!pForm)
			return;
		RE::TESValueForm* pValue = pForm->As<RE::TESValueForm>();
		if (pValue)
			pValue->value = value;
	}
}

namespace WindowsUtils
{
	long long microsecondsNow() {
		static LARGE_INTEGER s_frequency;
		static BOOL s_use_qpc = QueryPerformanceFrequency(&s_frequency);
		if (s_use_qpc) {
			LARGE_INTEGER now;
			QueryPerformanceCounter(&now);
			// To guard against loss-of-precision, we convert
			// to microseconds *before* dividing by ticks-per-second.
			return (1000000LL * now.QuadPart) / s_frequency.QuadPart;
		}
		else {
			return GetTickCount() * 1000;
		}
	}

	ScopedTimer::ScopedTimer(const std::string& context) : m_startTime(microsecondsNow()), m_context(context) {}
	ScopedTimer::ScopedTimer(const std::string& context, RE::TESObjectREFR* refr) : m_startTime(microsecondsNow()), m_context(context)
	{
		if (refr)
		{
			std::ostringstream fullContext;
			fullContext << context << ' ' << refr->data.objectReference->GetName() <<
				"/0x" << std::hex << std::setw(8) << std::setfill('0') << refr->data.objectReference->GetFormID();
			m_context = fullContext.str();
		}
	}

	ScopedTimer::~ScopedTimer()
	{
		long long endTime(microsecondsNow());
		_MESSAGE("TIME(%s)=%d micros", m_context.c_str(), endTime - m_startTime);
	}
}

namespace PluginUtils
{
	std::string GetPluginName(RE::TESForm* thisForm)
	{
		std::string result;
		UInt8 loadOrder = (thisForm->formID) >> 24;
		if (loadOrder < 0xFF)
		{
			RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
			const RE::TESFile* modInfo = dhnd->LookupLoadedModByIndex(loadOrder);
			if (modInfo)
				result = std::string(&modInfo->fileName[0]);
		}
		return result;
	}
	std::string GetPluginName(UInt8 modIndex)
	{
		std::string unknown("Unknown");
		const RE::TESFile* info = nullptr;
		RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
		if (dhnd && modIndex < 0xFF)
			info = dhnd->LookupLoadedModByIndex(modIndex);
		return (info) ? std::string(&info->fileName[0]) : unknown;
	}

	void SetBaseName(RE::TESForm* pForm, const char* str)
	{
		if (!pForm)
			return;
		RE::TESFullName* pFullName = pForm->As<RE::TESFullName>();
		if (pFullName)
			pFullName->fullName = str;
	}

	UInt8 GetModIndex(RE::TESForm* thisForm)
	{
		return (thisForm) ? thisForm->formID >> 24 : 0xFF;
	}
}

namespace PluginUtils
{
	UInt8 GetLoadedModIndex(const char* espName)
	{
		const RE::TESFile* info = RE::TESDataHandler::GetSingleton()->LookupModByName(espName);
		if (!info)
		{
#if _DEBUG
			_DMESSAGE("Mod lookup failed for %s", espName);
#endif
			return 0xFF;
		}

		if ((info->recordFlags & RE::TESFile::RecordFlag::kSmallFile) == RE::TESFile::RecordFlag::kSmallFile)
		{
#if _DEBUG
			_DMESSAGE("Found Creation Club Mod %s", espName);
#endif
			//いつか対応する
			return 0xFF;
		}
		// a full mod, not light file
#if _DEBUG
		_DMESSAGE("Found index 0x02%d Mod %s", static_cast<int>(info->compileIndex), espName);
#endif
		return info->compileIndex;
	}
}
namespace StringUtils
{
	std::string ToStringID(RE::FormID id)
	{
		std::stringstream ss;
		ss << std::hex << std::setfill('0') << std::setw(8) << std::uppercase << id;
		//std::string result("0x");
		//result += ss.str();
		//return result;
		return ss.str();
	}

	std::string ToString_0f(double num, UInt8 setp)
	{
		std::stringstream ss;
		ss << std::fixed << std::setprecision(setp) << num;
		return ss.str();
	}

	void ToUpper(std::string &str)
	{
		for (auto &c : str)
			c = toupper(c);
	}

	void ToLower(std::string &str)
	{
		for (auto &c : str)
			c = tolower(c);
	}

	std::string Trim(const std::string& str, const char* trimCharacterList = " \t\v\r\n")
	{
		std::string result;
		std::string::size_type left = str.find_first_not_of(trimCharacterList);
		if (left != std::string::npos)
		{
			std::string::size_type right = str.find_last_not_of(trimCharacterList);
			result = str.substr(left, right - left + 1);
		}
		return result;
	}

	void DeleteNl(std::string &str)
	{
		const char CR = '\r';
		const char LF = '\n';
		std::string destStr;
		for (const auto c : str) {
			if (c != CR && c != LF) {
				destStr += c;
			}
		}
		str = std::move(destStr);
	}

	std::vector<std::string> Split(const std::string &str, char sep)
	{
		std::vector<std::string> result;

		auto first = str.begin();
		while (first != str.end())
		{
			auto last = first;
			while (last != str.end() && *last != sep)
				++last;
			result.push_back(std::string(first, last));
			if (last != str.end())
				++last;
			first = last;
		}
		return result;
	}

	bool Replace(std::string &str, const std::string target, const std::string replacement)
	{
		if (str.empty() || target.empty())
			return false;

		bool result = false;
		std::string::size_type pos = 0;
		while ((pos = str.find(target, pos)) != std::string::npos)
		{
			if (!result)
				result = true;
			str.replace(pos, target.length(), replacement);
			pos += replacement.length();
		}
		return result;
	}
}

namespace GameSettingUtils
{
	std::string GetGameLanguage()
	{
		RE::Setting	* setting = RE::GetINISetting("sLanguage:General");
		std::string sLanguage = (setting && setting->GetType() == RE::Setting::Type::kString) ? setting->GetString() : "ENGLISH";
		return sLanguage;
	}
}

