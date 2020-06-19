#pragma once

namespace papyrus
{
	RE::BSFixedString GetTranslation(RE::StaticFunctionTag* base, RE::BSFixedString key);
	void LoadIniFile(RE::StaticFunctionTag* base);

	void SetIngredientForCritter(RE::StaticFunctionTag* base, RE::TESForm* critter, RE::TESForm* ingredient);

	bool RegisterFuncs(RE::BSScript::Internal::VirtualMachine* a_vm);
}


