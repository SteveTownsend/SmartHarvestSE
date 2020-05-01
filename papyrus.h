#pragma once

bool Replace(std::string& str, const std::string& target, const std::string& replacement);

namespace papyrus
{
	RE::BSFixedString GetTranslation(RE::StaticFunctionTag* base, RE::BSFixedString key);
	void UnblockEverything(RE::StaticFunctionTag* base);
	void LoadIniFile(RE::StaticFunctionTag* base);

	void SetIngredientForCritter(RE::StaticFunctionTag* base, RE::TESForm* critter, RE::TESForm* ingredient);

	bool RegisterFuncs(RE::BSScript::Internal::VirtualMachine* a_vm);
}


