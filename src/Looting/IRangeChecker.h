#pragma once

typedef std::pair<double, RE::TESObjectREFR*> TargetREFR;
typedef std::vector<TargetREFR> DistanceToTarget;

class IRangeChecker {
public:
	virtual bool IsValid(const RE::TESObjectREFR* refr, const double distance = 0.) const = 0;
	inline double Distance() const { return m_distance; }
protected:
	IRangeChecker() {}
	mutable double m_distance;
};

class AbsoluteRange : public IRangeChecker
{
public:
	AbsoluteRange(const RE::TESObjectREFR* source, const double radius, const double zFactor);
	virtual bool IsValid(const RE::TESObjectREFR* refr, const double distance = 0.) const override;

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
	virtual bool IsValid(const RE::TESObjectREFR* refr, const double distance = 0.) const override;
private:
	AbsoluteRange m_innerLimit;
	AbsoluteRange m_outerLimit;
};
