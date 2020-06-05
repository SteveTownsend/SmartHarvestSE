#pragma once

#include <shlobj.h>

constexpr RE::FormID ClothKeyword = 0x06BBE8;
constexpr RE::FormID CurrentFollowerFaction = 0x0005C84E;

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
		_MESSAGE("%p %s", func1, name);
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
	//UInt8 GetOrderIDByModName(std::string name);
	UInt8 GetLoadedModIndex(const char* espName);
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
}
