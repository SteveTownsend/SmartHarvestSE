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
	double valueWeight(settings->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::valueWeight, m_typeName.c_str()));
	if (valueWeight <= 0.)
	{
		valueWeight = settings->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, VW_Default);
	}
	if (valueWeight > 0.)
	{
		double worth = GetWorth();
		double weight = std::max(GetWeight(), 0.);
		if (worth > 0. && weight <= 0.)
		{
			DBG_VMESSAGE("* %s(%08x) has value %0.2f, weightless", GetName(), GetFormID(), worth);
			return false;
		}

		if (worth <= 0.)
		{
			if (weight <= 0.)
			{
				// this may be a scripted activator without special-case handling - one example is Poison Bloom (xx007cda).
				// Harvest if non v/w criteria say we should do so.
				DBG_VMESSAGE("* %s(%08x) - cannot calculate v/w from weight %0.2f and worth %0.2f", GetName(), GetFormID(), weight, worth);
				return false;
			}
			else
			{
				// zero value object with strictly positive weight - do not auto-harvest
				DBG_VMESSAGE("* %s(%08x) - has weight %0.2f, no value", GetName(), GetFormID(), weight);
				return true;
			}
		}

		double vw = (worth > 0. && weight > 0.) ? worth / weight : 0.0;
		DBG_VMESSAGE("* %s(%08x) item VW %0.2f vs threshold VW %0.2f", GetName(), GetFormID(), vw, valueWeight);
		// allow small tolerance for floating point math
		if (vw < valueWeight && fabs(valueWeight - vw) > 0.01)
			return true;
	}
	return false;
}
