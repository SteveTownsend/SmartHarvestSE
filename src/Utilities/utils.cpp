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

#include "Utilities/utils.h"

#include <shlobj.h>
#include <psapi.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm> //Trim
#include <math.h>	// pow

#include <brotli/decode.h>
#include <brotli/encode.h>

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
			DBG_MESSAGE("GetGamePath result: %s", s_runtimeDirectory.c_str());
		}
		return s_runtimeDirectory;
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
				REL_ERROR("GetModuleHandle failed, error = %d\n", ret);
			}
			else if (GetModuleFileName(hm, path, sizeof(path)) == 0)
			{
				int ret = GetLastError();
				REL_ERROR("GetModuleFileName failed, error = %d\n", ret);
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
				memset(drive, 0, MAX_PATH);
				char dir[MAX_PATH];
				memset(dir, 0, MAX_PATH);
				if (_splitpath_s(pluginFileName.c_str(), drive, MAX_PATH, dir, MAX_PATH, nullptr, 0, nullptr, 0) == 0)
				{
					char path[MAX_PATH];
					memset(path, 0, MAX_PATH);
					if (_makepath_s(path, drive, dir, nullptr, nullptr) == 0)
					{
					    s_skseDirectory = path;
						REL_MESSAGE("GetPluginPath result: %s", s_skseDirectory.c_str());
					}
				}
			}
		}
		return s_skseDirectory;
	}
}

namespace utils
{
	double GetGameSettingFloat(const RE::BSFixedString& name)
	{
		RE::Setting* setting(nullptr);
		RE::GameSettingCollection* settings(RE::GameSettingCollection::GetSingleton());
		if (settings)
		{
			setting = settings->GetSetting(name.c_str());
		}

		if (!setting || setting->GetType() != RE::Setting::Type::kFloat)
			return 0.0;

		double result(setting->GetFloat());
		DBG_MESSAGE("Game Setting(%s)=%.3f", name.c_str(), result);
		return result;
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
			return GetTickCount64() * 1000UL;
		}
	}

	void LogProcessWorkingSet()
	{
		PROCESS_MEMORY_COUNTERS pmc;
		if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
		{
			REL_MESSAGE("Working Set is %.3f MB", double(pmc.WorkingSetSize) / (1024.0 * 1024.0));
		}
	}

	ScopedTimer::ScopedTimer(const std::string& context) : m_startTime(microsecondsNow()), m_context(context) {}
	ScopedTimer::ScopedTimer(const std::string& context, RE::TESObjectREFR* refr) : m_startTime(microsecondsNow()), m_context(context)
	{
		std::ostringstream fullContext;
		fullContext << context << ' ' << (refr ? std::string(refr->GetBaseObject()->GetName()) : std::string()) <<
			"/0x" << std::hex << std::setw(8) << std::setfill('0') << (refr ? refr->GetBaseObject()->GetFormID() : InvalidForm);
		m_context = fullContext.str();
	}

	ScopedTimer::~ScopedTimer()
	{
		long long endTime(microsecondsNow());
		REL_MESSAGE("TIME(%s)=%d micros", m_context.c_str(), endTime - m_startTime);
	}

	std::unique_ptr<ScopedTimerFactory> ScopedTimerFactory::m_instance;

	ScopedTimerFactory& ScopedTimerFactory::Instance()
	{
		if (!m_instance)
		{
			m_instance = std::make_unique<ScopedTimerFactory>();
		}
		return *m_instance;
	}

	int ScopedTimerFactory::StartTimer(const std::string& context)
	{
#ifdef _PROFILING
		RecursiveLockGuard guard(m_timerLock);
		int handle(++m_nextHandle);
		m_timerByHandle.insert(std::make_pair(handle, std::make_unique<ScopedTimer>(context)));
		return handle;
#else
		return 0;
#endif
	}

	// fails silently if invalid, otherwise stops the timer and records result
	void ScopedTimerFactory::StopTimer(const int handle)
	{
#ifdef _PROFILING
		RecursiveLockGuard guard(m_timerLock);
		m_timerByHandle.erase(handle);
#endif
	}
}

namespace StringUtils
{
	void ToLower(std::string &str)
	{
		for (auto &c : str)
			c = tolower(c);
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

	// see https://stackoverflow.com/questions/215963/how-do-you-properly-use-widechartomultibyte
	std::string FromUnicode(const std::wstring& input) {
		if (input.empty()) return std::string();

		int size_needed = WideCharToMultiByte(CP_UTF8, 0, &input[0], static_cast<int>(input.size()), NULL, 0, NULL, NULL);
		if (size_needed == 0) return std::string();

		std::string output(size_needed, 0);
		int result(WideCharToMultiByte(CP_UTF8, 0, &input[0], static_cast<int>(input.size()), &output[0], size_needed, NULL, NULL));
		if (result == 0) return std::string();

		return output;
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

namespace CompressionUtils
{
	nlohmann::json DecodeBrotli(const std::string& compressed)
	{
		size_t inputSize(compressed.length());
		// TODO should not have to guess
		std::string inflated(inputSize * 15, 0);
		size_t outputSize;
		BrotliDecoderResult result(BrotliDecoderDecompress(inputSize, reinterpret_cast<const uint8_t*>(compressed.c_str()),
			&outputSize, reinterpret_cast<uint8_t*>(const_cast<char*>(inflated.c_str()))));
		if (result == BROTLI_DECODER_RESULT_SUCCESS)
		{
			REL_MESSAGE("Inflated %d bytes to %d", inputSize, outputSize);
			inflated.resize(outputSize);
			return nlohmann::json(inflated);
		}
		REL_ERROR("Inflating %d bytes failed, error %d", inputSize, result);
		return nlohmann::json();
	}

	std::string EncodeBrotli(const nlohmann::json& j)
	{
		std::string jsonStr(j.dump());
		size_t outputSize(BrotliEncoderMaxCompressedSize(jsonStr.length()));
		std::string encodedStr(outputSize, 0);
		static constexpr int BrotliQuality(1);	// favour fast speed over small size
		if (BrotliEncoderCompress(BrotliQuality, BROTLI_DEFAULT_WINDOW, BROTLI_MODE_TEXT,
			jsonStr.length(), reinterpret_cast<const uint8_t*>(jsonStr.c_str()), &outputSize,
			reinterpret_cast<uint8_t*>(const_cast<char*>(encodedStr.c_str()))) != BROTLI_FALSE)
		{
			REL_MESSAGE("Compressed %d bytes to %d, ratio %.3f", jsonStr.length(), outputSize, double(jsonStr.length()) / double(outputSize));
			encodedStr.resize(outputSize);
			return encodedStr;
		}
		REL_ERROR("Compressing %d bytes failed", jsonStr.length());
		return std::string();
	}
}
