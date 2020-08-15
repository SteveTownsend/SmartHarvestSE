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

#define INI_FILE "SmartHarvestSE.ini"
#define INI_FILE_DEFAULTS "SmartHarvestSE.Defaults.ini"
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
		glow,
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
		case SecondaryType::glow:
			return "glow";
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
	bool LoadFile(const bool useDefaults);

private:
	bool CreateSectionString(PrimaryType m_section_first, SecondaryType m_section_second, std::string& m_result);
	static INIFile* s_instance;
	const std::string GetFileName(const bool useDefaults);
	INIFile(void);
};
