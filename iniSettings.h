#pragma once

#define INI_FILE "AutoHarvestSE.ini"
#include "SimpleIni.h"

class INIFile : public SimpleIni
{
public:
	enum PrimaryType
	{
		NONE = 0,
		common,
		autoharvest,
		spell,
		LAST
	};

	enum SecondaryType
	{
		NONE2 = 0,
		config,
		itemObjects,
		containers,
		deadbodies,
		valueWeight,
		maxItemCount,
		LAST2
	};

	inline bool IsType(PrimaryType type) { return (type > PrimaryType::NONE && type < PrimaryType::LAST); }
	inline bool IsType(SecondaryType type) { return (type > SecondaryType::NONE2 && type < SecondaryType::LAST2); }
	inline bool GetIsPrimaryTypeString(PrimaryType type, std::string& result)
	{
		switch (type)
		{
		case common:
			result += "common"; break;
		case autoharvest:
			result += "autoharvest"; break;
		case spell:
			result += "spell"; break;
		default:
			return false;
		}
		return true;
	}

	inline bool GetIsSecondaryTypeString(SecondaryType type, std::string& result)
	{
		switch (type)
		{
		case config:
			result += "config"; break;
		case itemObjects:
			result += "itemobjects"; break;
		case containers:
			result += "containers"; break;
		case deadbodies:
			result += "deadbodies"; break;
		case valueWeight:
			result += "valueWeight"; break;
		case maxItemCount:
			result += "maxItemCount"; break;
		default:
			return false;
		}
		return true;
	}

	static INIFile* GetInstance(void)
	{
		if (s_instance == nullptr)
		{
			s_instance = new INIFile();
		}
		return s_instance;
	}

	double GetRadius(PrimaryType first);
	double GetSetting(PrimaryType m_section_first, SecondaryType m_section_second, std::string m_key);
	void PutSetting(PrimaryType m_section_first, SecondaryType m_section_second, std::string m_key, double m_value);
	void SaveFile(void);
	bool LoadFile(void);

private:
	bool CreateSectionString(PrimaryType m_section_first, SecondaryType m_section_second, std::string& m_result);
	static INIFile* s_instance;
	const std::string GetFileName(void);
	std::string iniFilePath;
	INIFile(void);
};
