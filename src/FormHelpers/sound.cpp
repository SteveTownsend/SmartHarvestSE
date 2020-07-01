#include "PrecompiledHeaders.h"

#include "FormHelpers/sound.h"
#include "Utilities/utils.h"

RE::BGSSoundDescriptorForm* _GetSoundDescriptorForm(RE::FormID formId)
{
	RE::BGSSoundDescriptorForm* result = nullptr;
	RE::TESForm* pForm = RE::TESForm::LookupByID(formId);
	if (pForm)
		result = pForm->As<RE::BGSSoundDescriptorForm>();
	return result;
}

RE::TESSound* LookupSoundByID(RE::FormID formId)
{
	RE::TESSound* result = nullptr;
	RE::TESForm* pForm = RE::TESForm::LookupByID(formId);
	if (pForm)
    	result = pForm->As<RE::TESSound>();
	return result;
}

RE::BGSSoundDescriptorForm* GetPickUpSoundDescriptor(RE::TESForm* baseForm)
{
	RE::BGSSoundDescriptorForm * result(nullptr);
	const RE::BGSPickupPutdownSounds* pSounds(baseForm->As<RE::BGSPickupPutdownSounds>());
	if (pSounds)
		return pSounds->pickupSound;

	if (baseForm->formType == RE::FormType::KeyMaster)
	{
		return _GetSoundDescriptorForm(0x03ED75);
	}
	else if (baseForm->formType == RE::FormType::Weapon)
	{
		return _GetSoundDescriptorForm(0x03C7BE);
	}
	else if (baseForm->formType == RE::FormType::Armor)
	{
		RE::TESObjectARMO* item = baseForm->As<RE::TESObjectARMO>();
		if (item)
		{
			RE::FormID formId = (item->HasKeyword(ClothKeyword)) ? 0x03C7BE : 0x03E609;
			return _GetSoundDescriptorForm(formId);
		}
	}
    return _GetSoundDescriptorForm(0x03C7BA);
}
