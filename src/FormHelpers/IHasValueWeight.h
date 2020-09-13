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

class INIFile;

class IHasValueWeight
{
public:
	ObjectType GetObjectType() const;
	std::string GetTypeName() const;
	bool ValueWeightTooLowToLoot() const;
	bool IsValuable() const;

	virtual double GetWeight(void) const = 0;
	uint32_t GetWorth(void) const;

	static constexpr float ValueWeightMaximum = 1000.0f;

protected:
	IHasValueWeight() : m_objectType(ObjectType::unknown), m_worth(0), m_worthSetup(false) {}
	virtual ~IHasValueWeight() {}

	virtual const char * GetName() const = 0;
	virtual uint32_t GetFormID() const = 0;
	virtual uint32_t CalculateWorth(void) const = 0;

	std::string m_typeName;
	ObjectType m_objectType;
	mutable uint32_t m_worth;
	mutable bool m_worthSetup;
};