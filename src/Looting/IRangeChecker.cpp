#include "PrecompiledHeaders.h"
#include "IRangeChecker.h"

AbsoluteRange::AbsoluteRange(const RE::TESObjectREFR* source, const double radius) :
	m_sourceX(source->GetPositionX()), m_sourceY(source->GetPositionY()), m_sourceZ(source->GetPositionZ()), m_radius(radius)
{
}

bool AbsoluteRange::IsValid(const RE::TESObjectREFR* refr) const
{
	RE::FormID formID(refr->formID);
	double dx = fabs(refr->GetPositionX() - m_sourceX);
	double dy = fabs(refr->GetPositionY() - m_sourceY);
	double dz = fabs(refr->GetPositionZ() - m_sourceZ);

	// don't do Floating Point math if we can trivially see it's too far away
	if (dx > m_radius || dy > m_radius || dz > m_radius)
	{
		// very verbose
		DBG_DMESSAGE("REFR 0x%08x {%.2f,%.2f,%.2f} trivially too far from player {%.2f,%.2f,%.2f}",
			formID, refr->GetPositionX(), refr->GetPositionY(), refr->GetPositionZ(),
			RE::PlayerCharacter::GetSingleton()->GetPositionX(),
			RE::PlayerCharacter::GetSingleton()->GetPositionY(),
			RE::PlayerCharacter::GetSingleton()->GetPositionZ());
		return false;
	}
	double distance(sqrt((dx * dx) + (dy * dy) + (dz * dz)));
	DBG_VMESSAGE("REFR 0x%08x is %.2f units away, loot range %.2f units", formID, distance, m_radius);
	return distance <= m_radius;
}

BracketedRange::BracketedRange(const RE::TESObjectREFR* source, const double radius, const double delta) :
	m_innerLimit(source, radius), m_outerLimit(source, radius + delta)
{
}

bool BracketedRange::IsValid(const RE::TESObjectREFR* refr) const
{
	return !m_innerLimit.IsValid(refr) && m_outerLimit.IsValid(refr);
}

