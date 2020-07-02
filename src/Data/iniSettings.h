#pragma once

#define INI_FILE "SmartHarvestSE.ini"
#include "SimpleIni.h"

class INIFile : public SimpleIni
{
public:
	enum class PrimaryType
	{
		NONE = 0,
		common,
		harvest,
		LAST
	};

	enum class SecondaryType
	{
		NONE2 = 0,
		config,
		itemObjects,
		containers,
		deadbodies,
		valueWeight,
		LAST2
	};

	inline bool IsType(PrimaryType type) { return (type > PrimaryType::NONE && type < PrimaryType::LAST); }
	inline bool IsType(SecondaryType type) { return (type > SecondaryType::NONE2 && type < SecondaryType::LAST2); }
	inline std::string PrimaryTypeString(PrimaryType type)
	{
		switch (type)
		{
		case PrimaryType::common:
			return "common";
		case PrimaryType::harvest:
			return "smartHarvest";;
		default:
			break;
		}
		return "";
	}

	inline std::string SecondaryTypeString(SecondaryType type)
	{
		switch (type)
		{
		case SecondaryType::config:
			return "config";
		case SecondaryType::itemObjects:
			return "itemobjects";
		case SecondaryType::containers:
			return "containers";
		case SecondaryType::deadbodies:
			return "deadbodies";
		case SecondaryType::valueWeight:
			return "valueWeight";
		default:
			break;
		}
		return "";
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
	double GetIndoorsRadius(PrimaryType first);
	double GetVerticalFactor();
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
