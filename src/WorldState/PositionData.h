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
#include <array>

namespace shse
{

// x, y, z coordinates
typedef std::array<float, 3> Position;
typedef std::array<double, 3> AlglibPosition;

constexpr Position InvalidPosition = { 0.0, 0.0, 0.0 };

class RelativeLocationDescriptor
{
public:
	RelativeLocationDescriptor(const AlglibPosition startPoint, const AlglibPosition endPoint, const RE::FormID locationID, const double unitsAway) :
		m_startPoint(startPoint), m_endPoint(endPoint), m_locationID(locationID), m_unitsAway(unitsAway)
	{}
	inline AlglibPosition StartPoint() const { return m_startPoint; }
	inline AlglibPosition EndPoint() const { return m_endPoint; }
	inline RE::FormID LocationID() const { return m_locationID; }
	inline double UnitsAway() const { return m_unitsAway; }
	static RelativeLocationDescriptor Invalid() { return RelativeLocationDescriptor({ 0.,0.,0. }, { 0.,0.,0. }, 0, 0.0); }
	inline bool operator==(const RelativeLocationDescriptor& rhs) {
		return m_startPoint == rhs.m_startPoint && m_endPoint == rhs.m_endPoint;
	}

private:
	const AlglibPosition m_startPoint;
	const AlglibPosition m_endPoint;
	const RE::FormID m_locationID;	// represents end point
	const double m_unitsAway;
};

enum class MapMarkerType {
	Nearest = 0,
	AdventureTarget,
	MAX
};

}
