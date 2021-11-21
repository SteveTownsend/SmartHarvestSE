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
#include "Looting/objects.h"
#include "WorldState/PlacedObjects.h"
#include "Utilities/utils.h"

namespace shse
{

LeveledListMembers::LeveledListMembers(const RE::TESLevItem* rootItem,
	std::unordered_set<RE::FormID>& members) : m_members(members), LeveledItemCategorizer(rootItem)
{
}

std::unordered_set<RE::FormID> LeveledListMembers::m_exclusions;

void LeveledListMembers::SetupExclusions()
{
	std::vector<std::tuple<std::string, RE::FormID>> excludedLVLI = {
		{"Skyrim.esm", 0x4e4f3},								// DA09DawnbreakerList
		{"Complete Alchemy & Cooking Overhaul.esp", 0xcb2be},	// CACO_ALLIngredients
		{"Complete Alchemy & Cooking Overhaul.esp", 0x6b0f3a}	// CACO_ALLIngredients_New
	};
	std::unordered_set<RE::TESLevItem*> excludedForms;
	for (const auto& lvliDef : excludedLVLI)
	{
		std::string espName(std::get<0>(lvliDef));
		RE::FormID formID(std::get<1>(lvliDef));
		RE::TESLevItem* lvliForm(DataCase::GetInstance()->FindExactMatch<RE::TESLevItem>(espName, formID));
		if (lvliForm)
		{
			REL_MESSAGE("LVLI {}:0x{:08x} found for Quest Target", espName, lvliForm->GetFormID());
			m_exclusions.insert(lvliForm->GetFormID());
		}
		else
		{
			REL_MESSAGE("LVLI {}/0x{:08x} not found for Load Order", espName, formID);
		}
	}
}

void LeveledListMembers::ProcessContentLeaf(RE::TESBoundObject* itemForm, ObjectType)
{
	if (!m_exclusions.contains(m_rootItem->GetFormID()))
	{
		if (m_members.insert(itemForm->GetFormID()).second)
		{
			REL_VMESSAGE("LVLI 0x{:08x} member {}/0x{:08x} not treated as Quest Target",
				m_rootItem->GetFormID(), itemForm->GetName(), itemForm->GetFormID());
		}
		else
		{
			DBG_VMESSAGE("LVLI 0x{:08x} member {}/0x{:08x} already not treated as Quest Target",
				m_rootItem->GetFormID(), itemForm->GetName(), itemForm->GetFormID());
		}
	}
	else if (m_members.contains(itemForm->GetFormID()))
	{
		REL_WARNING("Exclusion LVLI 0x{:08x} member {}/0x{:08x} already not treated as Quest Target",
			m_rootItem->GetFormID(), itemForm->GetName(), itemForm->GetFormID());
	}
	else
	{
		REL_VMESSAGE("Exclusion LVLI 0x{:08x} member {}/0x{:08x} can be treated as Quest Target",
			m_rootItem->GetFormID(), itemForm->GetName(), itemForm->GetFormID());
	}
}

std::unique_ptr<QuestTargets> QuestTargets::m_instance;

QuestTargets& QuestTargets::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<QuestTargets>();
		LeveledListMembers::SetupExclusions();
	}
	return *m_instance;
}

QuestTargets::QuestTargets()
{
}

bool QuestTargets::IsLootableInanimateReference(const RE::TESObjectREFR* refr) const
{
	if (refr->As<RE::Actor>())
		return false;
	return FormTypeIsLootableObject(refr->GetBaseObject()->GetFormType());
}

void QuestTargets::Analyze()
{
	// any items that is in a Leveled List is not blacklisted as a Quest Target
	std::unordered_set<RE::FormID> lvliMembers;
	for (const auto leveledItem : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESLevItem>())
	{
		LeveledListMembers(leveledItem, lvliMembers).CategorizeContents();
	}

	for (const auto quest : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESQuest>())
	{
		REL_VMESSAGE("Quest Targets for {}/0x{:08x}", quest->GetName(), quest->GetFormID());
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
					const RE::TESBoundObject* targetItem(refAlias->fillData.created.object);
					m_questTargetAllItems.insert(targetItem->GetFormID());
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
										if (BlacklistQuestTargetReferencedItem(targetItem, refr))
										{
											REL_VMESSAGE("Blacklist Specific REFR {}/0x{:08x} to ALCO as Quest Target Item {}/0x{:08x}",
												refr->GetName(), refr->GetFormID(), targetItem->GetName(), targetItem->GetFormID());
											continue;
										}
										else
										{
											DBG_VMESSAGE("Failed to Blacklist Specific REFR {}/0x{:08x} to ALCO as Quest Target Item {}/0x{:08x}",
												refr->GetName(), refr->GetFormID(), targetItem->GetName(), targetItem->GetFormID());
										}
									}
								}
							}
							else if (targetAlias->fillType == RE::BGSBaseAlias::FILL_TYPE::kUniqueActor)
							{
								// created in ALUA
								RE::TESNPC* npc(targetAlias->fillData.uniqueActor.uniqueActor);
								if (npc)
								{
									// do not blacklist e.g. Torygg's War Horn 0xE77BB, but blacklist the hosting NPC
									if (BlacklistQuestTargetNPC(npc))
									{
										REL_VMESSAGE("Blacklist created-in ALUA {}/0x{:08x} for RefAlias ALCO {}/0x{:08x}",
											npc->GetName(), npc->GetFormID(), targetItem->GetName(), targetItem->GetFormID());
									}
									else
									{
										DBG_VMESSAGE("Using created-in ALUA {}/0x{:08x} to blacklist RefAlias ALCO {}/0x{:08x}",
											npc->GetName(), npc->GetFormID(), targetItem->GetName(), targetItem->GetFormID());
									}
									continue;
								}
							}
						}
					}
					DBG_VMESSAGE("Created RefAlias ALCO as Quest Target Item {}/0x{:08x} has type kAt",
						targetItem->GetName(), targetItem->GetFormID());
					// if item is in permitted LVLI, do not blacklist it
					size_t itemCount(PlacedObjects::Instance().NumberOfInstances(targetItem));
					if (itemCount >= BoringQuestTargetThreshold)
					{
						DBG_VMESSAGE("RefAlias ALCO as Quest Target Item {}/0x{:08x} ignored, too many ({} placed vs threshold {})",
							targetItem->GetName(), targetItem->GetFormID(), itemCount, BoringQuestTargetThreshold);
					}
					else if (lvliMembers.contains(targetItem->GetFormID()))
					{
						DBG_VMESSAGE("RefAlias ALCO excluded as Quest Target Item {}/0x{:08x}, member of LVLI",
							targetItem->GetName(), targetItem->GetFormID());
					}
					// record if unique or Quest Object flag set
					else if ((isQuest || (!targetItem->As<RE::TESNPC>() && itemCount <= RareQuestTargetThreshold)) &&
						BlacklistQuestTargetItem(targetItem))
					{
						REL_VMESSAGE("Blacklist Created RefAlias ALCO as Quest Target Item {}/0x{:08x} ({} placed)",
							targetItem->GetName(), targetItem->GetFormID(), itemCount);
					}
					else
					{
						DBG_VMESSAGE("Skip Created RefAlias ALCO {}/0x{:08x}", targetItem->GetName(), targetItem->GetFormID());
					}
				}
				else if (refAlias->fillType == RE::BGSBaseAlias::FILL_TYPE::kForced)
				{
					if (refAlias->fillData.forced.forcedRef)
					{
						RE::TESObjectREFR* refr(refAlias->fillData.forced.forcedRef.get().get());
						if (refr && refr->GetBaseObject())
						{
							m_questTargetAllItems.insert(refr->GetBaseObject()->GetFormID());
							size_t itemCount(PlacedObjects::Instance().NumberOfInstances(refr->GetBaseObject()));
							// record this specific REFR as the QUST target
							if ((isQuest || (IsLootableInanimateReference(refr) && itemCount <= RareQuestTargetThreshold)) && BlacklistQuestTargetREFR(refr))
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
	BlacklistOutliers();
}

void QuestTargets::BlacklistOutliers()
{
	// https://github.com/SteveTownsend/SmartHarvestSE/issues/322
	// MQ106DragonMapRef [REFR:000FF228] (places MQ106DragonParchment "Map of Dragon Burials" [MISC:000BBCD5]
	//   in GRUP Cell Persistent Children of RiverwoodSleepingGiantInn "Sleeping Giant Inn" [CELL:000133C6])
	const RE::TESObjectREFR* refr(RE::TESDataHandler::GetSingleton()->LookupForm<RE::TESObjectREFR>(0xff228, "Skyrim.esm"));
	const RE::TESObjectMISC* item(RE::TESDataHandler::GetSingleton()->LookupForm<RE::TESObjectMISC>(0xbbcd5, "Skyrim.esm"));
	if (item && refr)
	{
		REL_VMESSAGE("Blacklist REFR {}/0x{:08x} to outlier Quest Target Item {}/0x{:08x}",
			refr->GetName(), refr->GetFormID(), item->GetName(), item->GetFormID());
		BlacklistQuestTargetReferencedItem(item, refr);
	}
	const std::vector<std::pair<std::string, RE::FormID>> offLimitsNPCs = {
		// Ysgramor's Tomb Companion Ghosts
		{"Skyrim.esm", 0xab102},
		{"Skyrim.esm", 0xab104},
		{"Skyrim.esm", 0xde516},
		{"Skyrim.esm", 0xde517},
		{"Skyrim.esm", 0xde518},
		{"Skyrim.esm", 0xde519},
		// Ysgramor's Tomb Wolf Spirits
		{"Skyrim.esm", 0x58303},	// Kodlak
		{"Skyrim.esm", 0xf6087},	// Farkas
		{"Skyrim.esm", 0xf6089}		// Vilkas
	};
	for (const auto barredNPC : offLimitsNPCs)
	{
		const RE::TESNPC* npc(RE::TESDataHandler::GetSingleton()->LookupForm<RE::TESNPC>(barredNPC.second, barredNPC.first));
		if (npc)
		{
			REL_VMESSAGE("Blacklist outlier Quest Target NPC {}/0x{:08x}", npc->GetName(), npc->GetFormID());
			BlacklistQuestTargetNPC(npc);
		}
	}
	const RE::IngredientItem* ingredient(RE::TESDataHandler::GetSingleton()->LookupForm<RE::IngredientItem>(0x3ad61, "Skyrim.esm"));
	RE::BGSPerk* perk(RE::TESDataHandler::GetSingleton()->LookupForm<RE::BGSPerk>(0x1c05b, "Dragonborn.esm"));
	auto player(RE::PlayerCharacter::GetSingleton());
	QuestTargetPredicate predicate([=]() -> bool {return player->HasPerk(perk); });
	// Conditionally blacklisted items
	if (ingredient && perk && player)
	{
		if (BlacklistConditionalQuestTargetItem(ingredient, predicate))
		{
			REL_VMESSAGE("Blacklist Quest Target {}/0x{:08x} conditional on Perk {}/0x{:08x}",
				ingredient->GetName(), ingredient->GetFormID(), perk->GetName(), perk->GetFormID());
		}
	}
}

// used for Quest Target Items with no specific REFR. Blocks autoloot of the item everywhere,
// to preserve immersion and avoid breaking Quests.
bool QuestTargets::BlacklistQuestTargetItem(const RE::TESBoundObject* item)
{
	if (!FormUtils::IsConcrete(item))
		return false;
	if (m_questTargetItems.insert(item->GetFormID()).second)
	{
		m_userCannotPermission.insert(item->GetFormID());
		return true;
	}
	return false;
}

bool QuestTargets::BlacklistConditionalQuestTargetItem(const RE::TESBoundObject* item, QuestTargetPredicate predicate)
{
	if (!FormUtils::IsConcrete(item))
		return false;
	return m_conditionalQuestTargetItems.insert({ item->GetFormID(), predicate }).second;
}


// used for Quest Target Items with specific REFR. Blocks autoloot of the item for this REFR (or any if REFR blank),
// to preserve immersion and avoid breaking Quests.
bool QuestTargets::BlacklistQuestTargetReferencedItem(const RE::TESBoundObject* item, const RE::TESObjectREFR* refr)
{
	if (!FormUtils::IsConcrete(item))
		return false;
	return m_questTargetReferenced[item->GetFormID()].insert(refr->GetFormID()).second;
}

// used for Quest Target Items. Blocks autoloot of the item, to preserve immersion and avoid breaking Quests.
bool QuestTargets::BlacklistQuestTargetREFR(const RE::TESObjectREFR* refr)
{
	if (!refr->GetBaseObject() || !FormUtils::IsConcrete(refr->GetBaseObject()))
		return false;
	// record the base object
	if (m_questTargetREFRs.insert(refr->GetFormID()).second)
	{
		m_userCannotPermission.insert(refr->GetBaseObject()->GetFormID());
		return true;
	}
	return false;
}

// used for Quest Target NPCs. Blocks autoloot of the NPC, to preserve immersion and avoid breaking Quests.
bool QuestTargets::BlacklistQuestTargetNPC(const RE::TESNPC* npc)
{
	if (!npc)
		return false;
	std::string name(npc->GetName());
	if (name.empty())
		return false;
	RecursiveLockGuard guard(m_questLock);
	return m_questTargetItems.insert(npc->GetFormID()).second;
}

Lootability QuestTargets::ReferencedQuestTargetLootability(const RE::TESObjectREFR* refr) const
{
	if (!refr)
		return Lootability::NullReference;
	if (m_questTargetREFRs.contains(refr->GetFormID()))
	{
		return Lootability::CannotLootQuestTarget;
	}
	return QuestTargetLootability(refr->GetBaseObject(), refr);
}

Lootability QuestTargets::QuestTargetLootability(const RE::TESForm* form, const RE::TESObjectREFR* refr) const
{
	if (!form)
		return Lootability::NoBaseObject;
	// Dynamic forms must never be recorded as their FormID may be reused - this may never fire, since list was built in startup logic.
	// User-created ALCH may trigger this though.
	if (form->IsDynamicForm())
		return Lootability::Lootable;
	RecursiveLockGuard guard(m_questLock);
	// check for universal item match with no stored explicit  REFR
	if (m_questTargetItems.contains(form->GetFormID()))
	{
		return Lootability::CannotLootQuestTarget;
	}
	// check for specific reference to base
	const auto referenced(m_questTargetReferenced.find(form->GetFormID()));
	if (referenced != m_questTargetReferenced.cend() && referenced->second.find(refr->GetFormID()) != referenced->second.cend())
	{
		return Lootability::CannotLootQuestTarget;
	}
	// check for items that can be conditionally excluded
	const auto condition(m_conditionalQuestTargetItems.find(form->GetFormID()));
	if (condition != m_conditionalQuestTargetItems.cend() && (condition->second)())
	{
		return Lootability::CannotLootQuestTarget;
	}
	return Lootability::Lootable;
}

// we need to be extremely conservative in excluding possible Quest Targets from excess inventory handling
bool QuestTargets::AllowsExcessHandling(const RE::TESForm* form) const
{
	if (!form)
		return false;
	// Dynamic forms must never be recorded as their FormID may be reused - this may never fire, since list was built in startup logic.
	// User-created ALCH may trigger this though.
	if (form->IsDynamicForm())
		return true;
	RecursiveLockGuard guard(m_questLock);
	// check for universal item match
	return !m_questTargetAllItems.contains(form->GetFormID());
}

bool QuestTargets::UserCannotPermission(const RE::TESForm* form) const
{
	RecursiveLockGuard guard(m_questLock);
	return m_userCannotPermission.contains(form->GetFormID());
}

}