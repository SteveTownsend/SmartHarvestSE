#include "PrecompiledHeaders.h"

ObjectType IHasValueWeight::GetObjectType() const
{
	return m_objectType;
}

std::string IHasValueWeight::GetTypeName() const
{
	return m_typeName;
}

constexpr const char* VW_Default = "valueWeightDefault";

bool IHasValueWeight::ValueWeightTooLowToLoot(INIFile* settings) const
{
#if 1
	// A specified default for value-weight supersedes a missing type-specific value-weight
	double defaultVW(settings->GetSetting(INIFile::autoharvest, INIFile::SecondaryType::config, VW_Default));
	double valueVW(settings->GetSetting(INIFile::autoharvest, INIFile::SecondaryType::valueWeight, m_typeName.c_str()));
	double effectiveVW(std::max(valueVW, defaultVW));
	if (effectiveVW > 0)
	{
		double worth = GetWorth();
		double weight = std::max(GetWeight(), 0.);
		double vw = (worth > 0. && weight > 0.) ? worth / weight : 0.0;

#if _DEBUG
		const char* name = GetName();
		double worthB = GetWorth();
		double weightB = GetWeight();

		_MESSAGE("* %s(%08x)", name, GetFormID());
		_MESSAGE("VW=%0.2f effectiveVW=%0.2f", vw, effectiveVW);
#endif

		if (vw < effectiveVW)
		{
			return true;
		}
	}
	return false;
#else
	double valueVW = settings->GetSetting(INIFile::autoharvest, INIFile::SecondaryType::valueWeight, m_typeName.c_str());

	if (valueVW > 0)
	{
		UInt32 worth = static_cast<UInt32>(GetWorth());
		UInt32 weight = static_cast<UInt32>(GetWeight());
		weight = (weight > 0) ? weight : 1;
		double vw = (worth > 0 && weight > 0) ? worth / weight : 0.0;

		if (vw < valueVW)
		{
			return true;
		}
	}
	return false;
#endif
}
