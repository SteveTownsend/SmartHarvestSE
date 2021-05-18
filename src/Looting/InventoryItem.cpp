/*************************************************************************
SmartHarvest SE
Copyright (c) Steve Townsend 2020

>>> SOURCE LICENSE >>>
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation (www.fsf.org); either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License is available at
http://www.fsf.org/licensing/licenses
>>> END OF LICENSE >>>
*************************************************************************/
#include "PrecompiledHeaders.h"

#include "Looting/objects.h"
#include "Looting/InventoryItem.h"
#include "Looting/ScanGovernor.h"
#include "Collections/CollectionManager.h"
#include "FormHelpers/FormHelper.h"

namespace shse
{

InventoryItem::InventoryItem(
	std::unique_ptr<RE::InventoryEntryData> a_entry, std::ptrdiff_t a_count, const EnchantedObjectHandling enchantedObjectHandling) :
	m_inlineTransfer(false), m_entry(std::move(a_entry)), m_count(a_count),
	m_objectType(GetBaseFormObjectType(m_entry->GetObject()))
{
	// Decorate objectType for player-created enchantments: promote vanilla object if it has enchantment.
	DBG_VMESSAGE("{}/0x{:08x} has type {}", m_entry->GetObject()->GetName(), m_entry->GetObject()->GetFormID(), GetObjectTypeName(m_objectType));
	if (m_objectType == ObjectType::weapon || m_objectType == ObjectType::armor || m_objectType == ObjectType::jewelry)
	{
		// player-created enchantments are always known, so treat as unenchanted unless we collect Known enchangements
		if (IncludeEnchantedObjectIfKnown(enchantedObjectHandling) &&
			TESFormHelper::ConfirmEnchanted(GetEnchantmentFromExtraLists(m_entry->extraLists), enchantedObjectHandling))
		{
			DBG_VMESSAGE("{}/0x{:08x} has player-created enchantment", m_entry->GetObject()->GetName(), m_entry->GetObject()->GetFormID());
			switch (m_objectType)
			{
			case ObjectType::weapon:
				m_objectType = ObjectType::enchantedWeapon;
				break;
			case ObjectType::armor:
				m_objectType = ObjectType::enchantedArmor;
				break;
			case ObjectType::jewelry:
				m_objectType = ObjectType::enchantedJewelry;
				break;
			default:
				break;
			}
		}
	}
	// we may need to treat an item with vanilla enchantment as unenchanted
	if (TypeIsEnchanted(m_objectType))
	{
		ObjectType newType = TESFormHelper::EnchantedItemEffectiveType(m_entry->GetObject(), m_objectType, enchantedObjectHandling);
		if (newType != m_objectType)
		{
			m_objectType = newType;
		}
	}
	DBG_VMESSAGE("{}/0x{:08x} final type {}", m_entry->GetObject()->GetName(), m_entry->GetObject()->GetFormID(), GetObjectTypeName(m_objectType));
}
InventoryItem::InventoryItem(const InventoryItem& rhs) :
	m_inlineTransfer(rhs.m_inlineTransfer), m_entry(std::move(rhs.m_entry)), m_count(rhs.m_count), m_objectType(rhs.m_objectType) {}

// returns number of objects added
size_t InventoryItem::TakeAll(RE::TESObjectREFR* container, RE::TESObjectREFR* target, const bool collectible, const bool inlineTransfer)
{
	m_inlineTransfer = inlineTransfer;
	auto toRemove = m_count;
	if (toRemove <= 0) {
		return 0;
	}

	DBG_VMESSAGE("get {}/0x{:08x} ({})", BoundObject()->GetName(), BoundObject()->GetFormID(), toRemove);
	std::vector<std::pair<RE::ExtraDataList*, std::ptrdiff_t>> queued;
	if (m_entry->extraLists) {
		for (auto& xList : *m_entry->extraLists) {
			if (xList) {
				auto xCount = std::min<std::ptrdiff_t>(xList->GetCount(), toRemove);
				DBG_VMESSAGE("Handle extra list {} ({})", xList->GetDisplayName(BoundObject()), xCount);

				toRemove -= xCount;
				queued.push_back(std::make_pair(xList, xCount));

				if (toRemove <= 0) {
					break;
				}
			}
		}
	}

	// Removing items from NPCs here seems to be impossible to make stable, possibly because of thread safety issues with
	// the game's unequip-item handling. Give up trying, and script this.
	// RemoveItem inline soon after Actor death is problematic, I speculate that the game is sorting out the equipment state.
	// In any case these RemoveItem calls can cause a crash during or after the death of an NPC during my play-testing.
	// We wait briefly before looting bodies so we don't glow them during a kill animation, and script the inventory
	// shuffling.
	/*
		Possible relevant objects (15)
		{
		  [   1]    NiNode(Name: `WEAPON`)
		  [   1]    NiNode(Name: `WeaponDagger`)
		  [  37]    TESNPC(Name: `Bandit Hidden`, FormId: 5A000969, File: `know_your_armor_patch.esp <- OBIS SE.esp`)
		  [  37]    Character(FormId: FF001DEF, BaseForm: TESNPC(Name: `Bandit Hidden`, FormId: 5A000969, File: `know_your_armor_patch.esp <- OBIS SE.esp`))
		  [  45]    BSFadeNode(Name: `Weapon  (4A00953A)`)
		  [  60]    BSFlattenedBoneTree(Name: `NPC Root [Root]`)
		  [  64]    TESObjectWEAP(Name: `Cyrodiilic Silver Dagger`, FormId: 4A00953A, File: `Audio Weather and Misc Merged.esp <- Lore Weapon Expansion.esp`)
		  [  93]    BSFadeNode(Name: `skeletonbeast.nif`)
		  [ 164]    TESNPC(Name: `Aerlyn`, FormId: 00000007, File: `know_your_armor_patch.esp <- Skyrim.esm`)
		  [ 164]    PlayerCharacter(FormId: 00000014, BaseForm: TESNPC(Name: `Aerlyn`, FormId: 00000007, File: `know_your_armor_patch.esp <- Skyrim.esm`))
		  [ 172]    BGSEquipSlot(FormId: 00013F42, File: `Skyrim.esm`)
		  [ 185]    BGSEquipSlot(FormId: 00013F43, File: `Skyrim.esm`)
		  [ 406]    TESObjectMISC(Name: `Septims`, FormId: 0000000F, File: `Immersive Jewelry.esp <- Weapons Armor Clothing & Clutter Fixes.esp <- Skyrim.esm`)
		  [ 417]    TESObjectARMO(Name: `Leather Boots`, FormId: 00013920, File: `CACO CCOR Omega and Qwinn Merged.esp <- Weapons Armor Clothing & Clutter Fixes.esp <- Skyrim.esm`)
		  [ 422]    TESObjectARMO(Name: `Ranger Bracers`, FormId: 41005A70, File: `CACO CCOR Omega and Qwinn Merged.esp <- MLU - Immersive Armors.esp <- Hothtrooper44_ArmorCompilation.esp`)
		}

		[28]  0x7FF7A1476BC1     (SkyrimSE.exe + 106BC1)          BSExtraDataList::unk_106B50 + 71
		[29]  0x7FF7A147755A     (SkyrimSE.exe + 10755A)          BSExtraDataList::GetExtraDataWithoutLocking_107480 + DA
		[30]  0x7FF7A1483E66     (SkyrimSE.exe + 113E66)          BSExtraDataList::GetContainerChanges_113E20 + 46
		[31]  0x7FF7A15FDAAA     (SkyrimSE.exe + 28DAAA)          TESObjectREFR::RemoveItem_28D9E0 + CA
		[32]  0x7FF7A196F7B9     (SkyrimSE.exe + 5FF7B9)          Actor::RemoveItem_5FF750 + 69
		[33]  0x17AC0056C3A      (SmartHarvestSE.dll + 6C3A)
	*/
	for (auto& elem : queued) {
		DBG_VMESSAGE("Move extra list {} ({})", elem.first->GetDisplayName(BoundObject()), elem.second);
		Remove(container, target, elem.first, elem.second, collectible);
	}
	if (toRemove > 0) {
		DBG_VMESSAGE("Move item {} ({})", BoundObject()->GetName(), toRemove);
		Remove(container, target, nullptr, toRemove, collectible);
	}
	return static_cast<size_t>(toRemove + queued.size());
}

void InventoryItem::Remove(RE::TESObjectREFR* container, RE::TESObjectREFR* target, RE::ExtraDataList* extraDataList,
	ptrdiff_t count, const bool collectible)
{
	if (m_inlineTransfer)
	{
		// safe to handle here - record the item for Collection correlation before moving
		shse::CollectionManager::Instance().CheckEnqueueAddedItem(BoundObject(), INIFile::SecondaryType::containers, m_objectType);
		container->RemoveItem(BoundObject(), static_cast<int32_t>(count), RE::ITEM_REMOVE_REASON::kRemove, extraDataList, target);
	}
	else
	{
		// apparent thread safety issues for NPC item transfer - use Script event dispatch
		EventPublisher::Instance().TriggerLootFromNPC(container, BoundObject(), static_cast<int>(count), m_objectType, collectible);
	}
}

void InventoryItem::MakeCopies(RE::TESObjectREFR* target, size_t count)
{
	target->AddObjectToContainer(BoundObject(), nullptr, static_cast<int32_t>(count), nullptr);
}

}