#pragma once

typedef std::pair<double, RE::TESObjectREFR*> TargetREFR;
typedef std::vector<TargetREFR> DistanceToTarget;

class IRangeChecker {
public:
	virtual bool IsValid(const RE::TESObjectREFR* refr) const = 0;
	virtual double Radius() const = 0;
	virtual void SetRadius(const double newRadius) = 0;
	virtual double Distance() const = 0;

protected:
	IRangeChecker() : m_distance(0.) {}
	mutable double m_distance;
};

class AbsoluteRange : public IRangeChecker
{
public:
	AbsoluteRange(const RE::TESObjectREFR* source, const double radius, const double zFactor);
	virtual bool IsValid(const RE::TESObjectREFR* refr) const override;
	virtual double Radius() const override;
	virtual void SetRadius(const double newRadius) override;
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
	virtual void SetRadius(const double newRadius) override;
	virtual double Distance() const override;

private:
	AbsoluteRange m_innerLimit;
	AbsoluteRange m_outerLimit;
};
