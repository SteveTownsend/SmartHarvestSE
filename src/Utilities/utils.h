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

#include "Data/LoadOrder.h"

constexpr RE::FormID ClothKeyword = 0x06BBE8;
constexpr RE::FormID CurrentFollowerFaction = 0x0005C84E;

constexpr double DistanceUnitInFeet = 0.046875;
constexpr double FeetPerMile = 5280.0;
constexpr double DistanceUnitInMiles = DistanceUnitInFeet / FeetPerMile;

namespace FileUtils
{
	std::wstring GetGamePath(void);
	std::wstring GetPluginFileName(void) noexcept;
	std::wstring GetPluginPath(void) noexcept;
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
	unsigned long long microsecondsNow();
	void LogProcessWorkingSet();
	void TakeNap(const double delaySeconds);

	class ScopedTimer {
	public:
		ScopedTimer(const std::string& context);
		ScopedTimer(const std::string& context, RE::TESObjectREFR* refr);
		~ScopedTimer();
	private:
		ScopedTimer();
		ScopedTimer(const ScopedTimer&);
		ScopedTimer& operator=(ScopedTimer&);

		unsigned long long m_startTime;
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

namespace FormUtils
{
	// This can be missing, e.g. "Elementary Destruction.esp" FormID 0x31617, Github issue #28
	// Certain forms such as CONT do not load this at runtime, see 
	// https://github.com/Ryan-rsm-McKenzie/CommonLibSSE/issues/20
	inline std::string SafeGetFormEditorID(const RE::TESForm* form)
	{
		const char* edid(form ? form->GetFormEditorID() : nullptr);
		return edid ? std::string(edid) : std::string();
	}

	inline bool IsConcrete(const RE::TESForm* form)
	{
		return form && form->GetPlayable() && !std::string(form->GetName()).empty();
	}
}

namespace StringUtils
{
	void ToLower(std::string &str);
	void ToUpper(std::string& str);
	bool Replace(std::string &str, const std::string target, const std::string replacement);
	std::string FromUnicode(const std::wstring& input);
	inline RE::FormID ToFormID(const std::string& formStr)
	{
		RE::FormID formID;
		std::stringstream ss;
		ss << std::hex << formStr;
		ss >> formID;
		return ss.fail() ? InvalidForm : formID;
	}
	constexpr const char * InvalidFormString = "00000000";
	inline std::string FromFormID(const RE::FormID formID)
	{
		std::ostringstream ss;
		ss << std::hex << std::setw(8) << std::setfill('0') << formID;
		return ss.fail() ? InvalidFormString : ss.str();

	}
	std::string FormIDString(const RE::FormID formID);
}

namespace CompressionUtils
{
	bool DecodeBrotli(const std::string& compressed, nlohmann::json& output);
	bool EncodeBrotli(const nlohmann::json& plainText, std::string& encoded);
}

namespace JSONUtils
{
	std::vector<std::pair<std::string, std::vector<std::string>>> ParseFormsType(const nlohmann::json& formsType);

	template <typename FORMTYPE> std::unordered_set<const FORMTYPE*> ToForms(const nlohmann::json& formsType)
	{
		std::unordered_set<const FORMTYPE*> forms;
		const auto formNamesByPlugin(ParseFormsType(formsType));
		for (const auto& entry : formNamesByPlugin)
		{
			for (const auto nextID : entry.second)
			{
				// schema enforces 8-char HEX format
				RE::FormID formID(StringUtils::ToFormID(nextID));
				const FORMTYPE* form(shse::LoadOrder::Instance().LookupForm<FORMTYPE>(shse::LoadOrder::Instance().AsRaw(formID), entry.first));
				if (!form)
				{
					REL_WARNING("FormsCondition requires valid Forms, got {}/0x{:08x}", entry.first.c_str(), formID);
					continue;
				}
				DBG_VMESSAGE("Resolved Form 0x{:08x}", formID);
				forms.insert(form);
			}
		}
		return forms;
	}
	std::vector<std::pair<std::string, std::vector<std::string>>> ParseFormsType(const nlohmann::json& formsType);
}
