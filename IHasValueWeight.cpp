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
	// A specified default for value-weight supersedes a missing type-specific value-weight
	double valueWeight(settings->GetSetting(INIFile::autoharvest, INIFile::SecondaryType::valueWeight, m_typeName.c_str()));
	if (valueWeight <= 0.)
	{
		valueWeight = settings->GetSetting(INIFile::autoharvest, INIFile::SecondaryType::config, VW_Default);
	}
	if (valueWeight > 0.)
	{
		double worth = GetWorth();
		double weight = std::max(GetWeight(), 0.);
		if (worth > 0. && weight <= 0.)
		{
#if _DEBUG
			_MESSAGE("* %s(%08x) has value %0.2f, weightless", GetName(), GetFormID(), worth);
#endif
			return false;
		}

		if (worth <= 0. && weight <= 0.)
		{
			// this may be a scripted activator without special-case handling - one example is Poison Bloom (xx007cda).
			// Harvest if non v/w criteria say we should do so.
#if _DEBUG
			_MESSAGE("* %s(%08x) - cannot calculate v/w from weight %0.2f and worth %0.2f", GetName(), GetFormID(), weight, worth);
#endif
			return false;
		}

		double vw = (worth > 0. && weight > 0.) ? worth / weight : 0.0;
#if _DEBUG
		_MESSAGE("* %s(%08x) item VW %0.2f vs threshold VW %0.2f", GetName(), GetFormID(), vw, valueWeight);
#endif
		// allow small tolerance for floating point math
		if (vw < valueWeight && fabs(valueWeight - vw) > 0.01)
			return true;
	}
	return false;
}
