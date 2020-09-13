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
#include "FormHelpers/IHasValueWeight.h"

ObjectType IHasValueWeight::GetObjectType() const
{
	return m_objectType;
}

std::string IHasValueWeight::GetTypeName() const
{
	return m_typeName;
}

constexpr const char* VW_Default = "valueWeightDefault";

uint32_t IHasValueWeight::GetWorth(void) const
{
	if (!m_worthSetup)
	{
		m_worth = CalculateWorth();
		m_worthSetup = true;
	}
	return m_worth;
}

bool IHasValueWeight::ValueWeightTooLowToLoot() const
{
	uint32_t worth(GetWorth());
	DBG_VMESSAGE("Checking value: {}", worth);

	// valuable objects overrides V/W checks
	if (IsValuable())
		return false;

	INIFile* settings(INIFile::GetInstance());
	// A specified default for value-weight supersedes a missing type-specific value-weight
	double valueWeight(settings->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::valueWeight, m_typeName.c_str()));
	if (valueWeight <= 0.)
	{
		valueWeight = settings->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, VW_Default);
	}

	if (valueWeight > 0.)
	{
		if (m_objectType == ObjectType::ammo)
		{
			// arrows use the value as an absolute threshold - in this case value represents damage done
			// allow small tolerance for floating point uncertainty
			DBG_VMESSAGE("{}/0x{:08x} ammo damage {} vs threshold {:0.2f}", GetName(), GetFormID(), worth, valueWeight);
			return worth < valueWeight - 0.01;
		}
		double weight = std::max(GetWeight(), 0.);
		if (worth > 0. && weight <= 0.)
		{
			DBG_VMESSAGE("{}/0x{:08x} has value {}, weightless", GetName(), GetFormID(), worth);
			return false;
		}

		if (worth <= 0.)
		{
			if (weight <= 0.)
			{
				// Harvest if non v/w criteria say we should do so.
				DBG_VMESSAGE("{}/0x{:08x} - cannot calculate v/w from weight {:0.2f} and value {}", GetName(), GetFormID(), weight, worth);
				return false;
			}
			else
			{
				// zero value object with strictly positive weight - do not auto-harvest
				DBG_VMESSAGE("{}/0x{:08x} - has weight {:0.2f}, no value", GetName(), GetFormID(), weight);
				return true;
			}
		}

		double vw = (worth > 0. && weight > 0.) ? worth / weight : 0.0;
		DBG_VMESSAGE("{}/0x{:08x} item VW {:0.2f} vs threshold VW {:0.2f}", GetName(), GetFormID(), vw, valueWeight);
		// allow small tolerance for floating point uncertainty
		if (vw < valueWeight - 0.01)
			return true;
	}
	return false;
}

bool IHasValueWeight::IsValuable() const
{
	uint32_t worth(GetWorth());
	if (worth > 0)
	{
		double minValue(INIFile::GetInstance()->GetSetting(INIFile::PrimaryType::harvest, INIFile::SecondaryType::config, "ValuableItemThreshold"));
		// allow small tolerance for floating point uncertainty
		if (minValue > 0. && double(worth) >= minValue - 0.01)
		{
			DBG_VMESSAGE("{}/{:08x} has value {} vs threshold {:0.2f}: Valuable", GetName(), GetFormID(), worth, minValue);
			return true;
		}
	}
	return false;
}
