#pragma once

#include "FormHelpers/IHasValueWeight.h"

class TESFormHelper : public IHasValueWeight
{
public:
	TESFormHelper(const RE::TESForm* form);

	RE::BGSKeywordForm* GetKeywordForm(void) const;
	RE::EnchantmentItem* GetEnchantment(void);
	UInt32 GetGoldValue(void) const;
	std::pair<bool, SpecialObjectHandling> IsCollectible(void) const;

	virtual double GetWeight(void) const override;

	const RE::TESForm* m_form;

protected:
	virtual const char* GetName() const;
	virtual UInt32 GetFormID() const;
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
