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

#include <shlobj.h>

constexpr RE::FormID ClothKeyword = 0x06BBE8;
constexpr RE::FormID CurrentFollowerFaction = 0x0005C84E;
constexpr RE::FormID InvalidForm = 0x0;
constexpr RE::FormID InvalidPlugin = 0xFFFFFFFF;
constexpr RE::FormID ESPMask = 0xFF000000;
constexpr RE::FormID FullRawMask = 0x00FFFFFF;
constexpr RE::FormID ESPFETypeMask = 0xFE000000;
constexpr RE::FormID ESPFEMask = 0xFEFFF000;
constexpr RE::FormID ESPFERawMask = 0x00000FFF;

constexpr double DistanceUnitInFeet = 0.046875;
constexpr double FeetPerMile = 5280.0;
constexpr double DistanceUnitInMiles = DistanceUnitInFeet / FeetPerMile;

namespace FileUtils
{
	std::string GetGamePath(void);
	std::string GetPluginFileName(void);
	std::string GetPluginPath(void);
	inline bool CanOpenFile(const char* fileName)
	{
		std::ifstream ifs(fileName);
		return ifs.fail() ? false : true;
	}
}

namespace utils
{
	double GetGameSettingFloat(const RE::BSFixedString& name);
}

namespace WindowsUtils
{
	long long microsecondsNow();

	class ScopedTimer {
	public:
		ScopedTimer(const std::string& context);
		ScopedTimer(const std::string& context, RE::TESObjectREFR* refr);
		~ScopedTimer();
	private:
		ScopedTimer();
		ScopedTimer(const ScopedTimer&);
		ScopedTimer& operator=(ScopedTimer&);

		long long m_startTime;
		std::string m_context;
	};

	class ScopedTimerFactory
	{
	public:
		static ScopedTimerFactory& Instance();
		ScopedTimerFactory() : m_nextHandle(0) {}
		int StartTimer(const std::string& context);
		void StopTimer(const int handle);

	private:
		static std::unique_ptr<ScopedTimerFactory> m_instance;
		RecursiveLock m_timerLock;
		std::unordered_map<int, std::unique_ptr<ScopedTimer>> m_timerByHandle;
		int m_nextHandle;
	};
}

namespace PluginUtils
{
	inline std::string GetBaseName(RE::TESForm* thisForm)
	{
		return thisForm ? thisForm->GetName() : std::string();
	}
	inline RE::FormID AsRaw(const RE::FormID rawID)
	{
		if ((rawID & ESPFETypeMask) == ESPFETypeMask)
			return rawID & ESPFERawMask;
		return rawID & FullRawMask;
	}
}

namespace FormUtils
{
	// This can be missing, e.g. "Elementary Destruction.esp" FormID 0x31617, Github issue #28
	// Certain forms such as CONT do not load this at runtime, see 
	// https://github.com/Ryan-rsm-McKenzie/CommonLibSSE/issues/20
	inline std::string SafeGetFormEditorID(const RE::TESForm* form)
	{
		const char* edid(form->GetFormEditorID());
		return edid ? std::string(edid) : std::string();
	}
}

namespace StringUtils
{
	void ToLower(std::string &str);
	bool Replace(std::string &str, const std::string target, const std::string replacement);
	std::string FromUnicode(const std::wstring& input);
}
