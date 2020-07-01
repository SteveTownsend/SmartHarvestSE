#pragma once

class IRangeChecker {
public:
	virtual bool IsValid(const RE::TESObjectREFR* refr, const double distance = 0.) const = 0;
	virtual double Distance() const = 0;
protected:
	IRangeChecker() {}
	mutable double m_distance;
};

class AbsoluteRange : public IRangeChecker
{
public:
	AbsoluteRange(const RE::TESObjectREFR* source, const double radius, const double verticalFactor);
	virtual bool IsValid(const RE::TESObjectREFR* refr, const double distance = 0.) const override;
	virtual double Distance() const override;

private:
	double m_radius;
	double m_zLimit;
	double m_sourceX;
	double m_sourceY;
	double m_sourceZ;
};

class BracketedRange : public IRangeChecker
{
public:
	BracketedRange(const RE::TESObjectREFR* source, const double radius, const double m_delta);
	virtual bool IsValid(const RE::TESObjectREFR* refr, const double distance = 0.) const override;
	virtual double Distance() const override;

private:
	AbsoluteRange m_innerLimit;
	AbsoluteRange m_outerLimit;
};

typedef std::pair<double, RE::TESObjectREFR*> TargetREFR;
typedef std::vector<TargetREFR> DistanceToTarget;
