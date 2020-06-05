#include "PrecompiledHeaders.h"

INIFile* INIFile::s_instance = nullptr;
namespace
{
	void ToLower(std::string& str)
	{
		for (auto& c : str)
			c = tolower(c);
	}
}

INIFile::INIFile()
{
}

bool INIFile::LoadFile()
{
	Free();

	bool result = Load(GetFileName());

	if (result)
	{
#if _DEBUG
		_MESSAGE("Loaded %s OK", GetFileName().c_str());
		SimpleIni::SectionIterator itSection;
		SimpleIni::KeyIterator itKey;
		for (itSection = beginSection(); itSection != endSection(); ++itSection)
		{
			for (itKey = beginKey(*itSection); itKey != endKey(*itSection); ++itKey)
				_MESSAGE("[%s] %s = %0.2f ", (*itSection).c_str(), (*itKey).c_str(), GetValue<double>(*itSection, *itKey, 0.0));
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
	if (!GetIsPrimaryTypeString(m_section_first, section[1]) || !GetIsSecondaryTypeString(m_section_second, section[2]))
		return false;
	m_result = section[1] + ":" + section[2];
	return true;
}

const std::string INIFile::GetFileName(void)
{
	if (iniFilePath.empty())
	{
		std::string RuntimeDir = FileUtils::GetGamePath();
		if (RuntimeDir.empty())
			return false;

		iniFilePath = RuntimeDir + "Data\\SKSE\\Plugins\\";
		iniFilePath += INI_FILE;
#if _DEBUG
		_MESSAGE("INI file at %s", iniFilePath.c_str());
#endif
	}
	return iniFilePath.c_str();
}

double INIFile::GetSetting(PrimaryType m_section_first, SecondaryType m_section_second, std::string m_key)
{
	double result = 0.0;
	std::string section;
	std::string key = m_key;

	if (!CreateSectionString(m_section_first, m_section_second, section))
		return 0.0;

	::ToLower(section);
	::ToLower(key);

	return GetValue<double>(section, key, 0.0);
}

void INIFile::PutSetting(PrimaryType m_section_first, SecondaryType m_section_second, std::string m_key, double m_value)
{
	std::string section;
	std::string key = m_key;;

	if (!CreateSectionString(m_section_first, m_section_second, section))
		return;

	::ToLower(section);
	::ToLower(key);

	SetValue<double>(section, key, m_value);
}

double INIFile::GetRadius(PrimaryType first)
{
	// Value for feet per unit from https://www.creationkit.com/index.php?title=Unit
	static const double FEET_PER_DISTANCE_UNIT(0.046875);
	const double setting(GetSetting(first, SecondaryType::config, "RadiusFeet"));
#if _DEBUG
	_MESSAGE("Search radius %.2f feet -> %.2f units", setting, setting / FEET_PER_DISTANCE_UNIT);
#endif
	return setting / FEET_PER_DISTANCE_UNIT;
}

double INIFile::GetIndoorsRadius(PrimaryType first)
{
	// Value for feet per unit from https://www.creationkit.com/index.php?title=Unit
	static const double FEET_PER_DISTANCE_UNIT(0.046875);
	const double setting(GetSetting(first, SecondaryType::config, "IndoorsRadiusFeet"));
#if _DEBUG
	_MESSAGE("Indoors search radius %.2f feet -> %.2f units", setting, setting / FEET_PER_DISTANCE_UNIT);
#endif
	return setting / FEET_PER_DISTANCE_UNIT;
}

void INIFile::SaveFile(void)
{
	SaveAs(GetFileName());
}
