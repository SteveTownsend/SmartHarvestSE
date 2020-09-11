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

#include "WorldState/QuestTargets.h"
#include "WorldState/PlacedObjects.h"
#include "Utilities/utils.h"

namespace shse
{

std::unique_ptr<QuestTargets> QuestTargets::m_instance;

QuestTargets& QuestTargets::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<QuestTargets>();
	}
	return *m_instance;
}

QuestTargets::QuestTargets()
{
}

bool QuestTargets::ReferenceIsLootable(const RE::TESObjectREFR* refr) const
{
	if (refr->As<RE::Actor>())
		return false;
	const auto formType(refr->GetBaseObject()->GetFormType());
	return
		formType == RE::FormType::AlchemyItem ||
		formType == RE::FormType::Ammo ||
		formType == RE::FormType::Armor ||
		formType == RE::FormType::Book ||
		formType == RE::FormType::Container ||
		formType == RE::FormType::Flora ||
		formType == RE::FormType::Ingredient ||
		formType == RE::FormType::KeyMaster ||
		formType == RE::FormType::Misc ||
		formType == RE::FormType::Note ||
		formType == RE::FormType::Projectile ||
		formType == RE::FormType::Scroll ||
		formType == RE::FormType::SoulGem ||
		formType == RE::FormType::Tree ||
		formType == RE::FormType::Weapon;
}

void QuestTargets::Analyze()
{
	for (const auto quest : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESQuest>())
	{
		DBG_VMESSAGE("Quest Targets for {}/0x{:08x}", quest->GetName(), quest->GetFormID());
		std::unordered_map<uint32_t, const RE::BGSBaseAlias*> aliasByID;
		for (const auto alias : quest->aliases)
		{
			aliasByID.insert({ alias->aliasID, alias });
		}
		for (const auto alias : quest->aliases)
		{
			// Blacklist item if it is a quest ref-alias object
			if (alias->GetVMTypeID() == RE::BGSRefAlias::VMTYPEID)
			{
				bool isQuest(alias->IsQuestObject());
				RE::BGSRefAlias* refAlias(static_cast<RE::BGSRefAlias*>(alias));
				if (refAlias->fillType == RE::BGSBaseAlias::FILL_TYPE::kCreated && refAlias->fillData.created.object)
				{
					// Check for specific instance of item created-in another alias
					uint16_t createdIn(refAlias->fillData.created.alias.alias);
					if (refAlias->fillData.created.alias.create == RE::BGSRefAlias::CreatedFillData::Alias::Create::kIn)
					{
						const auto target(aliasByID.find(createdIn));
						if (target != aliasByID.cend())
						{
							const RE::BGSRefAlias* targetAlias(static_cast<const RE::BGSRefAlias*>(target->second));
							if (targetAlias->fillType == RE::BGSBaseAlias::FILL_TYPE::kForced)
							{
								// created in ALFR
								if (targetAlias->fillData.forced.forcedRef)
								{
									RE::TESObjectREFR* refr(targetAlias->fillData.forced.forcedRef.get().get());
									if (refr)
									{
										// this object in this specific REFR is a Quest Target
										if (BlacklistQuestTargetReferencedItem(refAlias->fillData.created.object, refr))
										{
											REL_VMESSAGE("Blacklist Specific REFR {}/0x{:08x} to ALCO as Quest Target Item {}/0x{:08x}",
												refr->GetName(), refr->GetFormID(),
												refAlias->fillData.created.object->GetName(), refAlias->fillData.created.object->GetFormID());
											continue;
										}
										else
										{
											DBG_VMESSAGE("Failed to Blacklist Specific REFR {}/0x{:08x} to ALCO as Quest Target Item {}/0x{:08x}",
												refr->GetName(), refr->GetFormID(),
												refAlias->fillData.created.object->GetName(), refAlias->fillData.created.object->GetFormID());
										}
									}
								}
							}
							else if (targetAlias->fillType == RE::BGSBaseAlias::FILL_TYPE::kUniqueActor)
							{
								// created in ALUA
								if (targetAlias->fillData.uniqueActor.uniqueActor)
								{
									// do not blacklist e.g. Torygg's War Horn 0xE77BB
									DBG_VMESSAGE("RefAlias ALCO {}/0x{:08x} is created-in ALUA {}/0x{:08x}",
										refAlias->fillData.created.object->GetName(), refAlias->fillData.created.object->GetFormID(),
										targetAlias->fillData.uniqueActor.uniqueActor->GetName(), targetAlias->fillData.uniqueActor.uniqueActor->GetFormID());
								}
							}
						}
					}
					else
					{
						DBG_VMESSAGE("Created RefAlias ALCO as Quest Target Item {}/0x{:08x} has type kAt",
							refAlias->fillData.created.object->GetName(), refAlias->fillData.created.object->GetFormID());
					}
					size_t itemCount(PlacedObjects::Instance().NumberOfInstances(refAlias->fillData.created.object));
					if (itemCount >= BoringQuestTargetThreshold)
					{
						DBG_VMESSAGE("RefAlias ALCO as Quest Target Item {}/0x{:08x} ignored, too many ({} placed vs threshold {})",
							refAlias->fillData.created.object->GetName(), refAlias->fillData.created.object->GetFormID(), itemCount, BoringQuestTargetThreshold);
					}
					// record if unique or Quest Object flag set
					else if ((isQuest || (!refAlias->fillData.created.object->As<RE::TESNPC>() && itemCount <= RareQuestTargetThreshold)) &&
						BlacklistQuestTargetItem(refAlias->fillData.created.object))
					{
						REL_VMESSAGE("Blacklist Created RefAlias ALCO as Quest Target Item {}/0x{:08x} ({} placed)",
							refAlias->fillData.created.object->GetName(), refAlias->fillData.created.object->GetFormID(), itemCount);
					}
					else
					{
						DBG_VMESSAGE("Skip Created RefAlias ALCO {}/0x{:08x}",
							refAlias->fillData.created.object->GetName(), refAlias->fillData.created.object->GetFormID());
					}
				}
				else if (refAlias->fillType == RE::BGSBaseAlias::FILL_TYPE::kForced)
				{
					if (refAlias->fillData.forced.forcedRef)
					{
						RE::TESObjectREFR* refr(refAlias->fillData.forced.forcedRef.get().get());
						if (refr && refr->GetBaseObject())
						{
							size_t itemCount(PlacedObjects::Instance().NumberOfInstances(refr->GetBaseObject()));
							// record this specific REFR as the QUST target
							if ((isQuest || (ReferenceIsLootable(refr) && itemCount <= RareQuestTargetThreshold)) && BlacklistQuestTargetREFR(refr))
							{
								REL_VMESSAGE("Blacklist Forced RefAlias ALFR as Quest Target Item 0x{:08x} to Base {}/0x{:08x} ({} placed)",
									refr->GetFormID(), refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID(), itemCount);
							}
							else
							{
								DBG_VMESSAGE("Skip Forced RefAlias ALFR 0x{:08x} to Base {}/0x{:08x}",
									refr->GetFormID(), refr->GetBaseObject()->GetName(), refr->GetBaseObject()->GetFormID());
							}
						}
					}
				}
				else if (isQuest && refAlias->fillType == RE::BGSBaseAlias::FILL_TYPE::kUniqueActor)
				{
					// Quest NPC should not be looted
					if (refAlias->fillData.uniqueActor.uniqueActor)
					{
						if (BlacklistQuestTargetNPC(refAlias->fillData.uniqueActor.uniqueActor))
						{
							REL_VMESSAGE("Blacklist UniqueActor RefAlias ALUA as Quest Target NPC {}/0x{:08x}",
								refAlias->fillData.uniqueActor.uniqueActor->GetName(), refAlias->fillData.uniqueActor.uniqueActor->GetFormID());
						}
						else
						{
							DBG_VMESSAGE("Skip UniqueActor RefAlias ALUA {}/0x{:08x}",
								refAlias->fillData.uniqueActor.uniqueActor->GetName(), refAlias->fillData.uniqueActor.uniqueActor->GetFormID());
						}
					}
				}
				else
				{
					DBG_VMESSAGE("RefAlias skipped for Quest: {}/0x{:08x} - unsupported RefAlias fill-type {}",
						quest->GetName(), quest->GetFormID(), refAlias->fillType.underlying());
				}
			}
		}
	}
}

// used for Quest Target Items with specific REFR. Blocks autoloot of the item for this REFR (or any if REFR blank),
// to preserve immersion and avoid breaking Quests.
bool QuestTargets::BlacklistQuestTargetReferencedItem(const RE::TESBoundObject* item, const RE::TESObjectREFR* refr)
{
	if (!FormUtils::IsConcrete(item))
		return false;
	// dynamic forms must never be recorded as their FormID may be reused - this may never fire, since this is startup logic
	if (item->IsDynamicForm())
		return false;
	return (m_questTargetItems.insert({ item, refr })).second;
}

// used for Quest Target Items. Blocks autoloot of the item, to preserve immersion and avoid breaking Quests.
bool QuestTargets::BlacklistQuestTargetREFR(const RE::TESObjectREFR* refr)
{
	if (!refr->GetBaseObject() || !FormUtils::IsConcrete(refr->GetBaseObject()))
		return false;
	// dynamic forms must never be recorded as their FormID may be reused - this may never fire, since this is startup logic
	if (refr->IsDynamicForm() || refr->GetBaseObject()->IsDynamicForm())
		return false;
	return m_questTargetREFRs.insert(refr).second;
}

// used for Quest Target NPCs. Blocks autoloot of the NPC, to preserve immersion and avoid breaking Quests.
bool QuestTargets::BlacklistQuestTargetNPC(const RE::TESNPC* npc)
{
	if (!npc)
		return false;
	// dynamic forms must never be recorded as their FormID may be reused - this may never fire, since this is startup logic
	if (npc->IsDynamicForm())
		return false;
	std::string name(npc->GetName());
	if (name.empty())
		return false;
	RecursiveLockGuard guard(m_questLock);
	return (m_questTargetItems.insert({ npc, nullptr })).second;
}

Lootability QuestTargets::ReferencedQuestTargetLootability(const RE::TESObjectREFR* refr) const
{
	if (!refr)
		return Lootability::NullReference;
	if (m_questTargetREFRs.contains(refr))
	{
		return Lootability::CannotLootQuestTarget;
	}
	return QuestTargetLootability(refr->GetBaseObject(), refr);
}

Lootability QuestTargets::QuestTargetLootability(const RE::TESForm* form, const RE::TESObjectREFR* refr) const
{
	if (!form)
		return Lootability::NoBaseObject;
	// dynamic forms must never be recorded as their FormID may be reused - this may never fire, since list was built in startup logic
	if (form->IsDynamicForm())
		return Lootability::Lootable;
	RecursiveLockGuard guard(m_questLock);
	const auto matchedItem(m_questTargetItems.find(form));
	// check for specific REFR match, or item match with no stored explicit  REFR
	if (matchedItem != m_questTargetItems.cend() && (!matchedItem->second || refr == matchedItem->second))
	{
		return Lootability::CannotLootQuestTarget;
	}
	return Lootability::Lootable;
}

}