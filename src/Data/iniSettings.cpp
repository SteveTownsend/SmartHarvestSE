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

#include "Data/iniSettings.h"
#include "Utilities/utils.h"

INIFile* INIFile::s_instance = nullptr;

std::string INIFile::PrimaryTypeString(PrimaryType type)
{
	switch (type)
	{
	case PrimaryType::common:
		return "common";
	case PrimaryType::harvest:
		return "smartHarvest";
	default:
		break;
	}
	return "";
}

std::string INIFile::SecondaryTypeString(SecondaryType type)
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
	case SecondaryType::excessHandling:
		return "excessHandling";
	case SecondaryType::maxItems:
		return "maxItems";
	case SecondaryType::maxWeight:
		return "maxWeight";
	default:
		break;
	}
	return "";
}
INIFile::INIFile()
{
}

bool INIFile::LoadFile(const bool useDefaults)
{
	Free();

	std::string fileName(GetFileName(useDefaults));
	bool result = Load(fileName);

	if (result)
	{
		REL_MESSAGE("Loaded {} OK", fileName.c_str());
#if _DEBUG
		SimpleIni::SectionIterator itSection;
		SimpleIni::KeyIterator itKey;
		for (itSection = beginSection(); itSection != endSection(); ++itSection)
		{
			for (itKey = beginKey(*itSection); itKey != endKey(*itSection); ++itKey)
				DBG_VMESSAGE("[{}] {} = {:0.2f} ", (*itSection).c_str(), (*itKey).c_str(), GetValue<double>(*itSection, *itKey, 0.0));
		}
#endif
	}
	return result;
}


bool INIFile::CreateSectionString(PrimaryType m_section_first, SecondaryType m_section_second, std::string& m_result)
{
	std::string section[3];
	if (!IsType(m_section_first) || !IsType(m_section_second))

		return false;
	if ((section[1] = PrimaryTypeString(m_section_first)).empty() || (section[2] = SecondaryTypeString(m_section_second)).empty())
		return false;
	m_result = section[1] + ":" + section[2];
	return true;
}

const std::string INIFile::GetFileName(const bool useDefaults)
{
	std::string iniFilePath;
	std::string RuntimeDir = StringUtils::FromUnicode(FileUtils::GetGamePath());
	if (RuntimeDir.empty())
		return "";

	iniFilePath = RuntimeDir + "Data\\SKSE\\Plugins\\";
	iniFilePath += (useDefaults ? INI_FILE_DEFAULTS : INI_FILE);
	DBG_MESSAGE("INI file at {}", iniFilePath.c_str());
	return iniFilePath.c_str();
}

double INIFile::GetSetting(PrimaryType m_section_first, SecondaryType m_section_second, std::string m_key)
{
	std::string section;
	std::string key = m_key;

	if (!CreateSectionString(m_section_first, m_section_second, section))
		return 0.0;

	StringUtils::ToLower(section);
	StringUtils::ToLower(key);

	double setting(GetValue<double>(section, key, 0.0));
	DBG_DMESSAGE("Get config setting {}/{}/{} = {}", INIFile::PrimaryTypeString(m_section_first),
		INIFile::SecondaryTypeString(m_section_second), key.c_str(), setting);
	return setting;
}

void INIFile::PutSetting(PrimaryType m_section_first, SecondaryType m_section_second, std::string m_key, double m_value)
{
	std::string section;
	std::string key = m_key;;

	if (!CreateSectionString(m_section_first, m_section_second, section))
		return;

	StringUtils::ToLower(section);
	StringUtils::ToLower(key);

	SetValue<double>(section, key, m_value);
}

void INIFile::SaveFile(void)
{
	static const bool useDefaults(false);
	SaveAs(GetFileName(useDefaults));
}
