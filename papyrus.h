#pragma once

bool Replace(std::string& str, const std::string& target, const std::string& replacement);

namespace papyrus
{
	RE::BSFixedString GetTranslation(RE::StaticFunctionTag* base, RE::BSFixedString key);
	void UnblockEverything(RE::StaticFunctionTag* base);
	void LoadIniFile(RE::StaticFunctionTag* base);

	void SetIngredientForCritter(RE::StaticFunctionTag* base, RE::TESForm* critter, RE::TESForm* ingredient);

	bool RegisterFuncs(RE::BSScript::Internal::VirtualMachine* a_vm);
	template <class F>
	void RegisterFunction(RE::BSScript::Internal::VirtualMachine* a_vm, const char* a_fnName, const char* a_className, F* a_callback, bool a_callableFromTasklets = false)
	{
		a_vm->BindNativeMethod(RE::MakeNativeFunction(a_fnName, a_className, a_callback));
		if (a_callableFromTasklets) {
			a_vm->SetCallableFromTasklets(a_className, a_fnName, a_callableFromTasklets);
		}
	}
}


