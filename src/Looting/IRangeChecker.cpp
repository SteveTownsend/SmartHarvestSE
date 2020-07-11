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
#include "IRangeChecker.h"

AbsoluteRange::AbsoluteRange(const RE::TESObjectREFR* source, const double radius, const double zFactor) :
	m_sourceX(source->GetPositionX()), m_sourceY(source->GetPositionY()), m_sourceZ(source->GetPositionZ()),
	m_radius(radius), m_zLimit(radius * zFactor)
{
}

bool AbsoluteRange::IsValid(const RE::TESObjectREFR* refr) const
{
	RE::FormID formID(refr->formID);
	double dx = fabs(refr->GetPositionX() - m_sourceX);
	double dy = fabs(refr->GetPositionY() - m_sourceY);
	double dz = fabs(refr->GetPositionZ() - m_sourceZ);

	// don't do Floating Point math if we can trivially see it's too far away
	if (dx > m_radius || dy > m_radius || dz > m_zLimit)
	{
		// very verbose
		DBG_DMESSAGE("REFR 0x%08x {%.2f,%.2f,%.2f} trivially too far from player {%.2f,%.2f,%.2f}",
			formID, refr->GetPositionX(), refr->GetPositionY(), refr->GetPositionZ(),
			m_sourceX, m_sourceY, m_sourceZ);
		m_distance = std::max({ dx, dy, dz });
		return false;
	}
	m_distance = sqrt((dx * dx) + (dy * dy) + (dz * dz));
	DBG_VMESSAGE("REFR 0x%08x is %.2f units away, loot range %.2f XY-units, %.2f Z-units", formID, m_distance, m_radius, m_zLimit);
	return m_distance <= m_radius;
}

double AbsoluteRange::Radius() const
{
	return m_radius;
}

void AbsoluteRange::SetRadius(const double newRadius)
{
	m_radius = newRadius;
}

double AbsoluteRange::Distance() const
{
	return m_distance;
}

BracketedRange::BracketedRange(const RE::TESObjectREFR* source, const double radius, const double delta) :
	m_innerLimit(source, std::max(radius, 0.0), 1.0), m_outerLimit(source, radius + delta, 1.0)
{
}

bool BracketedRange::IsValid(const RE::TESObjectREFR* refr) const
{
	return !m_innerLimit.IsValid(refr) && m_outerLimit.IsValid(refr);
}

// TODO is this used/correct?
double BracketedRange::Radius() const
{
	return m_outerLimit.Radius();
}

// TODO is this used/correct?
void BracketedRange::SetRadius(const double newRadius)
{
	m_outerLimit.SetRadius(newRadius);
}

double BracketedRange::Distance() const
{
	return m_innerLimit.Distance();
}
