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

namespace FileUtils
{
	std::string ReadFileToString(const std::string& filePath);
	std::string GetGamePath(void);
	std::string GetDataPath(void);
	std::string GetPluginFileName(void);
	std::string GetPluginPath(void);
	bool WriteSectionKey(LPCTSTR section_name, LPCTSTR key_name, LPCTSTR key_data, LPCTSTR ini_file_path);
	std::vector<std::string> GetSectionKeys(LPCTSTR section_name, LPCTSTR ini_file_path);
	std::vector<std::string> GetIniKeys(std::string section, std::string fileName);
	inline bool CanOpenFile(const char* fileName)
	{
		std::ifstream ifs(fileName);
		return ifs.fail() ? false : true;
	}

}

namespace utils
{
	void SetGoldValue(RE::TESForm* pForm, UInt32 value);
	template <typename T> void LogFunctionAddress(T func, const char * name)
	{
		decltype(func) func1(func);
		DBG_MESSAGE("%p %s", func1, name);
	}
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
}

namespace PluginUtils
{
	std::string GetPluginName(RE::TESForm* thisForm);
	inline std::string GetBaseName(RE::TESForm* thisForm)
	{
		return thisForm ? thisForm->GetName() : std::string();
	}
	void SetBaseName(RE::TESForm* pForm, const char* str);
	std::string GetPluginName(UInt8 modIndex);
	UInt8 GetModIndex(RE::TESForm* thisForm);
	UInt8 GetLoadedModIndex(const char* espName);
	inline bool FormIDsAppearsEqual(const RE::FormID rawID, const RE::FormID target)
	{
		return (rawID & FullRawMask) == (target & FullRawMask);
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
	std::string ToStringID(RE::FormID id);
	std::string ToString_0f(double num, UInt8 set);
	void ToUpper(std::string &str);
	void ToLower(std::string &str);
	std::vector<std::string> Split(const std::string &str, char sep);
	bool Replace(std::string &str, const std::string target, const std::string replacement);
	std::string Trim(const std::string& str, const char* trimCharacterList);
	void DeleteNl(std::string &str);
	std::string FromUnicode(const std::wstring& input);
}
