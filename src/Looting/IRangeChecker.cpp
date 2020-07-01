#include "PrecompiledHeaders.h"
#include "IRangeChecker.h"

AbsoluteRange::AbsoluteRange(const RE::TESObjectREFR* source, const double radius, const double zFactor) :
	m_sourceX(source->GetPositionX()), m_sourceY(source->GetPositionY()), m_sourceZ(source->GetPositionZ()),
	m_radius(radius), m_zLimit(radius * zFactor)
{
}

bool AbsoluteRange::IsValid(const RE::TESObjectREFR* refr, const double distance) const
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
			RE::PlayerCharacter::GetSingleton()->GetPositionX(),
			RE::PlayerCharacter::GetSingleton()->GetPositionY(),
			RE::PlayerCharacter::GetSingleton()->GetPositionZ());
		return false;
	}
	m_distance = distance > 0. ? distance : sqrt((dx * dx) + (dy * dy) + (dz * dz));
	DBG_VMESSAGE("REFR 0x%08x is %.2f units away, loot range %.2f XY-units, %.2f Z-units", formID, m_distance, m_radius, m_zLimit);
	return m_distance <= m_radius;
}

BracketedRange::BracketedRange(const RE::TESObjectREFR* source, const double radius, const double delta) :
	m_innerLimit(source, radius, 1.0), m_outerLimit(source, radius + delta, 1.0)
{
}

bool BracketedRange::IsValid(const RE::TESObjectREFR* refr, const double distance) const
{
	return !m_innerLimit.IsValid(refr) && m_outerLimit.IsValid(refr, m_innerLimit.Distance());
}

