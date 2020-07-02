#pragma once

#include "FormHelpers/IHasValueWeight.h"
#include "Data/iniSettings.h"
#include "Collections/Condition.h"

class TESFormHelper : public IHasValueWeight
{
public:
	TESFormHelper(const RE::TESForm* form, const INIFile::SecondaryType scope);

	RE::BGSKeywordForm* GetKeywordForm(void) const;
	RE::EnchantmentItem* GetEnchantment(void);
	UInt32 GetGoldValue(void) const;
	std::pair<bool, SpecialObjectHandling> TreatAsCollectible(void) const;
	inline const RE::TESForm* Form() const { return m_form; }

	virtual double GetWeight(void) const override;

protected:
	const RE::TESForm* m_form;
	const shse::ConditionMatcher m_matcher;

	virtual const char* GetName() const override;
	virtual UInt32 GetFormID() const override;
	virtual double CalculateWorth(void) const override;
};

bool IsPlayable(const RE::TESForm* pForm);

template <typename FORM> RE::BGSKeywordForm* KeywordFormCast(const RE::TESForm* form)
{
	FORM* waypoint(form->As<FORM>());
	if (waypoint)
		return waypoint->As<RE::BGSKeywordForm>();
	return nullptr;
}
