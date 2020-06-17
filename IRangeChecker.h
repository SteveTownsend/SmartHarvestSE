#pragma once

class IRangeChecker {
public:
	virtual bool IsValid(const RE::TESObjectREFR* refr) const = 0;
protected:
	IRangeChecker() {}
};

class AbsoluteRange : public IRangeChecker
{
public:
	AbsoluteRange(const RE::TESObjectREFR* source, const double radius);
	virtual bool IsValid(const RE::TESObjectREFR* refr) const override;

private:
	double m_radius;
	double m_sourceX;
	double m_sourceY;
	double m_sourceZ;
};

class BracketedRange : public IRangeChecker
{
public:
	BracketedRange(const RE::TESObjectREFR* source, const double radius, const double m_delta);
	virtual bool IsValid(const RE::TESObjectREFR* refr) const override;
private:
	AbsoluteRange m_innerLimit;
	AbsoluteRange m_outerLimit;
};
