#include "PrecompiledHeaders.h"

#include "sound.h"
#include "dataCase.h"

RE::BGSSoundDescriptorForm* _GetSoundDescriptorForm(RE::FormID formId)
{
	RE::BGSSoundDescriptorForm* result = nullptr;
	RE::TESForm* pForm = RE::TESForm::LookupByID(formId);
	if (pForm)
		result = skyrim_cast<RE::BGSSoundDescriptorForm*, RE::TESForm>(pForm);
	return result;
}

RE::TESSound* LookupSoundByID(RE::FormID formId)
{
	RE::TESSound* result = nullptr;
	RE::TESForm* pForm = RE::TESForm::LookupByID(formId);
	if (pForm)
    	result = skyrim_cast<RE::TESSound*, RE::TESForm>(pForm);
	return result;
}

RE::BGSSoundDescriptorForm* GetPickUpSoundDescriptor(const RE::TESForm* baseForm)
{
	if (!baseForm)
		return nullptr;

	RE::BGSSoundDescriptorForm * result = nullptr;

	if (baseForm->formType == RE::FormType::Ingredient)
	{
		RE::IngredientItem* item = skyrim_cast<RE::IngredientItem*, RE::TESForm>(baseForm);
		if (item)
		{
			RE::BGSPickupPutdownSounds* pSounds = skyrim_cast<RE::BGSPickupPutdownSounds*, RE::IngredientItem>(item);
			if (pSounds && pSounds->pickupSound)
				result = _GetSoundDescriptorForm(0x03C7C2);
		}
	}
	else if (baseForm->formType == RE::FormType::Activator)
	{
		RE::TESObjectACTI* item = skyrim_cast<RE::TESObjectACTI*, RE::TESForm>(baseForm);
		if (item)
		{
			RE::BGSPickupPutdownSounds* pSounds = skyrim_cast<RE::BGSPickupPutdownSounds*, RE::TESObjectACTI>(item);
			if (pSounds && pSounds->pickupSound)
				result = _GetSoundDescriptorForm(0x03C7C2);
		}
	}
	else if (baseForm->formType == RE::FormType::AlchemyItem)
	{
		RE::AlchemyItem* item = skyrim_cast<RE::AlchemyItem*, RE::TESForm>(baseForm);
		if (item)
		{
			RE::BGSPickupPutdownSounds* pSounds = skyrim_cast<RE::BGSPickupPutdownSounds*, RE::AlchemyItem>(item);
			if (pSounds && pSounds->pickupSound)
				result = _GetSoundDescriptorForm(0x03C7C2);
		}
	}
	else if (baseForm->formType == RE::FormType::Book)
	{
		RE::TESObjectBOOK* item = skyrim_cast<RE::TESObjectBOOK*, RE::TESForm>(baseForm);
		if (item)
		{
			RE::BGSPickupPutdownSounds* pSounds = skyrim_cast<RE::BGSPickupPutdownSounds*, RE::TESObjectBOOK>(item);
			if (pSounds && pSounds->pickupSound)
				result = _GetSoundDescriptorForm(0x03EDDE);
		}
	}
	else if (baseForm->formType == RE::FormType::Misc)
	{
		RE::TESObjectMISC* item = skyrim_cast<RE::TESObjectMISC*, RE::TESForm>(baseForm);
		if (item)
		{
			RE::BGSPickupPutdownSounds* pSounds = skyrim_cast<RE::BGSPickupPutdownSounds*, RE::TESObjectMISC>(item);
			if (pSounds && pSounds->pickupSound)
				result = _GetSoundDescriptorForm(0x03C7BA);
		}
	}
	else if (baseForm->formType == RE::FormType::Scroll)
	{
		RE::ScrollItem* item = skyrim_cast<RE::ScrollItem*, RE::TESForm>(baseForm);
		if (item)
		{
			RE::BGSPickupPutdownSounds* pSounds = skyrim_cast<RE::BGSPickupPutdownSounds*, RE::ScrollItem>(item);
			if (pSounds && pSounds->pickupSound)
				result = _GetSoundDescriptorForm(0x03EDDE);
		}
	}
	else if (baseForm->formType == RE::FormType::Note)
	{
		RE::BGSNote* item = skyrim_cast<RE::BGSNote*, RE::TESForm>(baseForm);
		if (item)
		{
			RE::BGSPickupPutdownSounds* pSounds = skyrim_cast<RE::BGSPickupPutdownSounds*, RE::BGSNote>(item);
			if (pSounds && pSounds->pickupSound)
				result = _GetSoundDescriptorForm(0x0C7A54);
		}
	}
	else if (baseForm->formType == RE::FormType::Ammo)
	{
		RE::TESAmmo* item = skyrim_cast<RE::TESAmmo*, RE::TESForm>(baseForm);
		if (item)
		{
			RE::BGSPickupPutdownSounds* pSounds = skyrim_cast<RE::BGSPickupPutdownSounds*, RE::TESAmmo>(item);
			if (pSounds && pSounds->pickupSound)
				result = _GetSoundDescriptorForm(0x03E7B7);
		}
	}
	else if (baseForm->formType == RE::FormType::KeyMaster)
	{
		result = _GetSoundDescriptorForm(0x03ED75);
	}
	else if (baseForm->formType == RE::FormType::Weapon)
	{
		result = _GetSoundDescriptorForm(0x03C7BE);
	}
	else if (baseForm->formType == RE::FormType::Armor)
	{
		RE::TESObjectARMO* item = skyrim_cast<RE::TESObjectARMO*, RE::TESForm>(baseForm);
		if (item)
		{
			RE::FormID formId = (item->HasKeyword(ClothKeyword)) ? 0x03C7BE : 0x03E609;
			result = _GetSoundDescriptorForm(formId);
		}
	}

	if (!result)
	{
		RE::BGSPickupPutdownSounds *pSounds = skyrim_cast<RE::BGSPickupPutdownSounds*, RE::TESForm>(baseForm);
		if (pSounds && pSounds->pickupSound)
			result = pSounds->pickupSound;
		else
			result = _GetSoundDescriptorForm(0x03C7BA);
	}

	return result;
}

size_t soundIdx = 0;
RE::TESSound* CreateSound(RE::BGSSoundDescriptorForm* soundDesc)
{
	if (!soundDesc)
		return nullptr;

	DataCase* data = DataCase::GetInstance();
	static size_t max = static_cast<int>(data->lists.sound.size());
	if (max == 0)
		return nullptr;

	if (soundIdx >= max - 1)
		soundIdx = 0;

	RE::TESSound* sound = data->lists.sound.at(soundIdx++);
	if (!sound)
		return nullptr;

	sound->descriptor = soundDesc;
	return sound;
}
#if 0
RE::TESSound* CreatePickupSound(const RE::TESForm* baseForm)
{
	if (!baseForm)
		return nullptr;

	RE::TESSound * result = nullptr;

	if (baseForm->formID == 0xF)
	{
		result = LookupSoundByID(0x333AA);
	}
	else if (baseForm->formType == RE::FormType::Ingredient)
	{
		RE::IngredientItem* item = skyrim_cast<RE::IngredientItem*, RE::TESForm>(baseForm);
		if (item)
		{
			RE::BGSPickupPutdownSounds* pSounds = skyrim_cast<RE::BGSPickupPutdownSounds*, RE::IngredientItem>(item);
			if (pSounds && pSounds->pickupSound)
				result = CreateSound(pSounds->pickupSound);
		}
	}
	else if (baseForm->formType == RE::FormType::AlchemyItem)
	{
		result = LookupSoundByID(0x35227);
	}
	else if (baseForm->formType == RE::FormType::Book || baseForm->formType == RE::FormType::Scroll || baseForm->formType == RE::FormType::Note)
	{
		result = LookupSoundByID(0x036C0F);
	}
	else if (baseForm->formType == RE::FormType::Ammo)
	{
		result = LookupSoundByID(0x0334A6);
	}
	else if (baseForm->formType == RE::FormType::KeyMaster)
	{
		result = LookupSoundByID(0x035225);
	}
	else if (baseForm->formType == RE::FormType::Weapon)
	{
		result = LookupSoundByID(0x014116);
	}
	else if (baseForm->formType == RE::FormType::Armor)
	{
		RE::TESObjectARMO* item = skyrim_cast<RE::TESObjectARMO*, RE::TESForm>(baseForm);
		if (item)
			result = LookupSoundByID((!item->HasKeyword(ClothKeyword)) ? 0x32876 : 0x32878);
	}

	if (!result)
		result = LookupSoundByID(0x014115);

	return result;
}
#endif