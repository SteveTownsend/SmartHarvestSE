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

typedef std::pair<double, RE::TESObjectREFR*> TargetREFR;
typedef std::vector<TargetREFR> DistanceToTarget;

class IRangeChecker {
public:
	virtual bool IsValid(const RE::TESObjectREFR* refr) const = 0;
	virtual double Radius() const = 0;
	virtual double Distance() const = 0;

protected:
	IRangeChecker() : m_distance(0.) {}
	virtual ~IRangeChecker() {}
	mutable double m_distance;
};

class AlwaysInRange : public IRangeChecker
{
public:
	virtual bool IsValid(const RE::TESObjectREFR* refr) const override;
	virtual double Radius() const override;
	virtual double Distance() const override;
};

class AbsoluteRange : public IRangeChecker
{
public:
	AbsoluteRange(const RE::TESObjectREFR* source, const double radius, const double zFactor);
	virtual bool IsValid(const RE::TESObjectREFR* refr) const override;
	virtual double Radius() const override;
	virtual double Distance() const override;

private:
	double m_radius;
	double m_sourceX;
	double m_sourceY;
	double m_sourceZ;
	double m_zLimit;
};

class BracketedRange : public IRangeChecker
{
public:
	BracketedRange(const RE::TESObjectREFR* source, const double radius, const double m_delta);
	virtual bool IsValid(const RE::TESObjectREFR* refr) const override;
	virtual double Radius() const override;
	virtual double Distance() const override;

private:
	AbsoluteRange m_innerLimit;
	AbsoluteRange m_outerLimit;
};
