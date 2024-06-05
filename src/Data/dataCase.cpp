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

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

#include "Data/dataCase.h"
#include "Data/LoadOrder.h"
#include "Collections/CollectionManager.h"
#include "Utilities/utils.h"
#include "Utilities/version.h"
#include "WorldState/CraftingItems.h"
#include "WorldState/PlayerHouses.h"
#include "WorldState/PlayerState.h"
#include "Looting/ScanGovernor.h"
#include "Looting/objects.h"
#include "Looting/NPCFilter.h"

const std::string MissivesGroup("BlackList");
const std::string MissivesCollection("SHSE-Missives");

namespace
{
	std::vector<std::string> Split(const std::string &str, char sep)
	{
		std::vector<std::string> result;

		auto first = str.begin();
		while (first != str.end())
		{
			auto last = first;
			while (last != str.end() && *last != sep)
				++last;
			result.push_back(std::string(first, last));
			if (last != str.end())
				++last;
			first = last;
		}
		return result;
	}
};

namespace shse
{

DataCase* DataCase::s_pInstance = nullptr;

DataCase::DataCase() :
m_spellTomeKeyword(nullptr), m_underwear(nullptr), m_missivesBoards(nullptr)
{
}

void DataCase::GetTranslationData()
{
	RE::Setting	* setting = RE::GetINISetting("sLanguage:General");

	std::string path = "Interface\\Translations\\";
	path += std::string(SHSE_NAME);
	path += std::string("_");
	path += std::string((setting && setting->GetType() == RE::Setting::Type::kString) ? setting->data.s : "ENGLISH");
	path += std::string(".txt");

	DBG_MESSAGE("Reading translations from {}", path.c_str());
	RE::BSResourceNiBinaryStream fs(path.c_str());
	if (!fs.good())
		return;

	uint16_t bom = 0;
	bool	ret = fs.read(&bom, sizeof(uint16_t) / sizeof(wchar_t));
	if (!ret)
	{
		REL_ERROR("Empty translation file.");
		return;
	}

	if (bom != 0xFEFF)
	{
		REL_ERROR("BOM Error, file must be encoded in UCS-2 LE.");
		return;
	}

	while (true)
	{
		std::wstring buf;

		bool readOK(std::getline(fs, buf, L'\n'));
		if (!readOK) // End of file
		{
			break;
		}
		auto nextChar = buf.c_str();
		size_t offset(0);
		while (!iswspace(*nextChar) && offset < buf.length())
		{
			++nextChar;
			++offset;
		}
		if (offset <= 0)
			continue;

		// Save key and consume whitespace preceding value
		std::wstring key(buf.c_str(), buf.c_str() + offset);
		while (iswspace(*nextChar) && offset < buf.length())
		{
			++nextChar;
			++offset;
		}


		// use the rest of the line as a value, even if it's empty - omit trailing whitespace
		size_t whitespace(0);
		if (offset < buf.length())
		{
			auto endString(buf.crbegin());
			while (iswspace(*endString))
			{
				++whitespace;
				++endString;
			}
		}
		std::wstring translation(buf.c_str() + offset, buf.c_str() + buf.length() - whitespace);

		// convert Unicode to UTF8 for UI usage
		std::string keyS = StringUtils::FromUnicode(key);
		std::string translationS = StringUtils::FromUnicode(translation);

		m_translations[keyS] = translationS;
		DBG_VMESSAGE("Translation entry: {} -> {}", keyS.c_str(), translationS.c_str());

	}
	DBG_MESSAGE("* TranslationData({})", m_translations.size());

	return;
}

// process comma-separated list of allowed ACTI verbs, to make localization INI-based
void DataCase::ActivationVerbsByType(const char* activationVerbKey, const ObjectType objectType)
{
	RE::BSString iniVerbs(GetTranslation(activationVerbKey));
	std::istringstream verbStream(iniVerbs.c_str());
	std::string nextVerb;
	while (std::getline(verbStream, nextVerb, ',')) {
		auto inserted(m_objectTypeByActivationVerb.insert(std::make_pair(nextVerb, objectType)));
		if (inserted.second)
		{
			REL_MESSAGE("Activation Verb {}/{} registered as ObjectType {}",
				activationVerbKey, nextVerb.c_str(), GetObjectTypeName(objectType).c_str());
		}
		else
		{
			// dup verb in Translation file
			REL_WARNING("Ignoring Activation verb {}/{} already registered as ObjectType {}",
				activationVerbKey, nextVerb.c_str(), GetObjectTypeName(inserted.first->second).c_str());
		}
	}
}

// Some activation verbs are used to handle referenced forms as a catch-all, though we prefer other rules
void DataCase::StoreActivationVerbs()
{
	// https://github.com/SteveTownsend/SmartHarvestSE/issues/56
	// Clutter categorization here is not correct - typically these are quest items that we need the player to activate
	// maybe reinstate with a glow function later
	// ActivationVerbsByType("$SHSE_ACTIVATE_VERBS_CLUTTER", ObjectType::clutter);
	ActivationVerbsByType("$SHSE_ACTIVATE_VERBS_CRITTER", ObjectType::critter);
	ActivationVerbsByType("$SHSE_ACTIVATE_VERBS_FLORA", ObjectType::flora);
	ActivationVerbsByType("$SHSE_ACTIVATE_VERBS_OREVEIN", ObjectType::oreVein);
	// https://github.com/SteveTownsend/SmartHarvestSE/issues/133
	// retired in favour of Collections-based solution
	//ActivationVerbsByType("$SHSE_ACTIVATE_VERBS_MANUAL", ObjectType::manualLoot);
}

ObjectType DataCase::GetObjectTypeForActivationText(const std::string& verb) const
{
	// #436 support Oblivion Interaction Icons alt text by extending logic to match as Posix Basic RE
	// for use with std::basic_regex
	auto verbMatched(std::find_if(m_objectTypeByActivationVerb.cbegin(),
								   m_objectTypeByActivationVerb.cend(),
								   [&](const decltype(m_objectTypeByActivationVerb)::value_type &vt) -> bool
								   {
									   return std::regex_match(verb,
															   std::basic_regex(vt.first));
								   }));
	if (verbMatched != m_objectTypeByActivationVerb.end())
	{
		return verbMatched->second;
	}
	else
	{
		m_unhandledActivationVerbs.insert(verb);
		return ObjectType::unknown;
	}
}
void DataCase::CategorizeByActivationVerb()
{
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	if (!dhnd)
		return;

	for (RE::TESObjectACTI* activator : dhnd->GetFormArray<RE::TESObjectACTI>())
	{
		if (!activator->GetFullNameLength())
			continue;
		const char* formName(activator->GetFullName());

		RE::BSString activationText;
		if (activator->GetActivateText(RE::PlayerCharacter::GetSingleton(), activationText))
		{
			// read Activation Text up to newline - the text we see is the data in the ESP file, plus a newline, plus the target
			std::istringstream input;
			input.str(std::string(activationText));
			std::string verb;
			std::getline(input, verb);
			DBG_VMESSAGE("Categorizing {}/0x{:08x} by activation verb {}", formName, activator->GetFormID(), verb);
			
			ObjectType activatorType(GetObjectTypeForActivationText(verb));
			if (activatorType != ObjectType::unknown)
			{
				if (SetObjectTypeForForm(activator, activatorType))
				{
					// set resourceType for oreVein
					if (activatorType == ObjectType::oreVein)
					{
						// Deposit -> volcanic
						ResourceType resourceType;
						if (std::string(formName).find(std::string("Heart Stone Deposit")) != std::string::npos ||	// Dragonborn
							std::string(formName).find(std::string("Sulfur Deposit")) != std::string::npos)			// CACO
						{
							resourceType = ResourceType::volcanic;
						}
						else if (std::string(formName).find(std::string("Geode")) != std::string::npos)
						{
							resourceType = ResourceType::geode;
						}
						else
						{
							resourceType = ResourceType::ore;
						}
						m_resourceTypeByOreVein.insert(std::make_pair(activator, resourceType));
						REL_VMESSAGE("{}/0x{:08x} has ResourceType {}", formName, activator->GetFormID(), PrintResourceType(resourceType));
					}
					// Creation Club and others use Harvest for ACTI. We bucket as flora but need custom harvesting logic, like critter.
					else if (activatorType == ObjectType::flora)
					{
						m_syntheticFlora.insert(activator);
						REL_VMESSAGE("ACTI {}/0x{:08x} must be treated as flora", formName, activator->GetFormID());
					}
				}
				continue;
			}
		}
		DBG_MESSAGE("{}/0x{:08x} not mappable, uses verb '{}'", formName, activator->GetFormID(), std::string(activationText));
	}
}

void DataCase::FindCraftingItems(void)
{
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	if (!dhnd)
		return;

	for (RE::BGSConstructibleObject* cobj : dhnd->GetFormArray<RE::BGSConstructibleObject>())
	{
		unsigned int total(0);
		unsigned int added(0);
		cobj->requiredItems.ForEachContainerObject([&] (RE::ContainerObject& item) -> RE::BSContainer::ForEachResult {
			// Do not add Gold to crafting item list
			if (item.obj->GetFormID() != Gold)
			{
				if (CraftingItems::Instance().AddIfNew(item.obj))
					++added;
				++total;
			}
			return RE::BSContainer::ForEachResult::kContinue;
		});
		DBG_MESSAGE("Added {} of {} items for COBJ 0x{:08x}", added, total, cobj->GetFormID());
	}
}

void DataCase::AnalyzePerks(void)
{
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	if (!dhnd)
		return;

	for (const RE::BGSPerk* perk : dhnd->GetFormArray<RE::BGSPerk>())
	{
		DBG_MESSAGE("Perk {}/0x{:08x} being checked", perk->GetName(), perk->GetFormID());
		for (const RE::BGSPerkEntry* perkEntry : perk->perkEntries)
		{
			if (perkEntry->GetType() != RE::PERK_ENTRY_TYPE::kEntryPoint)
				continue;

			const RE::BGSEntryPointPerkEntry* entryPoint(static_cast<const RE::BGSEntryPointPerkEntry*>(perkEntry));
			if (entryPoint->entryData.entryPoint == RE::BGSEntryPoint::ENTRY_POINT::kAddLeveledListOnDeath &&
				entryPoint->entryData.function == RE::BGSEntryPointPerkEntry::EntryData::Function::kAddLeveledList)
			{
				REL_MESSAGE("Leveled items added on death by perk {}/0x{:08x}", perk->GetName(), perk->GetFormID());
				m_leveledItemOnDeathPerks.insert(perk);
			}
			if (entryPoint->entryData.entryPoint == RE::BGSEntryPoint::ENTRY_POINT::kModIngredientsHarvested)
			{
				if (entryPoint->entryData.function == RE::BGSEntryPointPerkEntry::EntryData::Function::kSetValue && 
					entryPoint->functionData && entryPoint->functionData->GetType() == RE::BGSEntryPointFunctionData::ENTRY_POINT_FUNCTION_DATA::kOneValue)
				{
					const RE::BGSEntryPointFunctionDataOneValue* oneValued(static_cast<const RE::BGSEntryPointFunctionDataOneValue*>(entryPoint->functionData));
					REL_MESSAGE("Modify Harvested Ingredients factor {:0.2f} from perk {}/0x{:08x}", oneValued->data, perk->GetName(), perk->GetFormID());
					m_modifyHarvestedPerkMultipliers.insert(std::make_pair(perk, oneValued->data));
				}
				else
				{
					REL_WARNING("Modify Harvested Ingredients unsupported for perk {}/0x{:08x}, function {}, type {}", perk->GetName(), perk->GetFormID(),
						static_cast<std::uint8_t>(entryPoint->entryData.function.get()), entryPoint->functionData ? int(entryPoint->functionData->GetType()) : -1);
				}
			}
		}
	}
	// List MGEF with Slow Time archetype, save the ones with Value Modifier on Bow Speed Bonus
	for (RE::EffectSetting* effect : dhnd->GetFormArray<RE::EffectSetting>())
	{
		if (effect->HasArchetype(RE::EffectSetting::Archetype::kSlowTime))
		{
			REL_VMESSAGE("SlowTime Magic Effect : {}({:08x})", effect->GetName(), effect->GetFormID());
		}
		else if (effect->HasArchetype(RE::EffectSetting::Archetype::kValueModifier) &&
			effect->data.primaryAV == RE::ActorValue::kBowSpeedBonus)
		{
			REL_VMESSAGE("ValueModifier-BowSpeedBonus Magic Effect : {}({:08x})", effect->GetName(), effect->GetFormID());
			m_slowTimeEffects.insert(effect);
		}
	}
}

void DataCase::ExcludeFactionContainers()
{
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	if (!dhnd)
		return;

	for (RE::TESFaction* faction : dhnd->GetFormArray<RE::TESFaction>())
	{
		const RE::TESObjectREFR* containerRef(nullptr);
		if ((faction->data.flags & RE::FACTION_DATA::Flag::kVendor) == RE::FACTION_DATA::Flag::kVendor)
		{
			containerRef = faction->vendorData.merchantContainer;
			if (containerRef)
			{
				REL_VMESSAGE("Blocked faction/vendor container : {}({:08x})", containerRef->GetName(), containerRef->GetFormID());
				m_offLimitsContainers.insert(containerRef->GetFormID());
			}
		}

		containerRef = faction->crimeData.factionStolenContainer;
		if (containerRef)
		{
			REL_VMESSAGE("Blocked stolenGoodsContainer : {}({:08x})", containerRef->GetName(), containerRef->GetFormID());
			m_offLimitsContainers.insert(containerRef->GetFormID());
		}

		containerRef = faction->crimeData.factionPlayerInventoryContainer;
		if (containerRef)
		{
			REL_VMESSAGE("Blocked playerInventoryContainer : {}({:08x})", containerRef->GetName(), containerRef->GetFormID());
			m_offLimitsContainers.insert(containerRef->GetFormID());
		}
	}
}

bool DataCase::ReferencesBlacklistedContainer(const RE::TESObjectREFR* refr) const
{
	RecursiveLockGuard guard(m_blockListLock);
	if (m_containerBlackList.contains(refr->GetContainer()))
	{
		return true;
	}
	const RE::TESObjectCONT* container(refr->GetBaseObject()->As<RE::TESObjectCONT>());
	if (container && m_missivesBoards)
	{
		DBG_VMESSAGE("Is Container {}/{:08x} member of Missive Boards? {}",
			container->GetName(), container->GetFormID(), m_missivesBoards->IsMemberOf(container));
		return m_missivesBoards->IsEnabled() && m_missivesBoards->IsMemberOf(container);
	}
	else if (!container)
	{
		REL_WARNING("Base Object for {:08x} is not TESObjectCONT", refr->GetFormID());
	}
	return false;
}

void DataCase::ExcludeVendorContainers()
{
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	if (!dhnd)
		return;

	// Vendor chests contain LVLI with substring VendorGold - there's no way to check that on the fly
	// because for LVLI records, EDID is not loaded
	// Check for exact match in Load Order using {plugin file, plugin-relative Form ID} tuple
	// TODO assumes no merge - core game, probably OK
	std::vector<std::tuple<std::string, RE::FormID>> vendorItemLVLI = {
		{"Skyrim.esm", 0x17102},	// VendorGoldBlacksmithTown
		{"Skyrim.esm", 0x72ae7},	// VendorGoldMisc
		{"Skyrim.esm", 0x72ae8},	// VendorGoldApothecary
		{"Skyrim.esm", 0x72ae9},	// VendorGoldBlacksmith
		{"Skyrim.esm", 0x72aea},	// VendorGoldInn
		{"Skyrim.esm", 0x72aeb},	// VendorGoldStreetVendor
		{"Skyrim.esm", 0x72aec},	// VendorGoldSpells
		{"Skyrim.esm", 0x72aed},	// VendorGoldBlackSmithOrc
		{"Skyrim.esm", 0xd54bf},	// VendorGoldFenceStage00
		{"Skyrim.esm", 0xd54c0},	// VendorGoldFenceStage01
		{"Skyrim.esm", 0xd54c1},	// VendorGoldFenceStage02
		{"Skyrim.esm", 0xd54c2},	// VendorGoldFenceStage03
		{"Skyrim.esm", 0xd54c3},	// VendorGoldFenceStage04
		{"Skyrim.esm", 0x9af0f}		// LItemMiscVendorLockpicks75 - catches a lot of mod-added vendor/merchant CONT
	};
	std::unordered_set<RE::TESLevItem*> vendorSentinels;
	for (const auto& lvliDef : vendorItemLVLI)
	{
		std::string espName(std::get<0>(lvliDef));
		RE::FormID formID(std::get<1>(lvliDef));
		RE::TESLevItem* lvliForm(FindExactMatch<RE::TESLevItem>(espName, formID));
		if (lvliForm)
		{
			REL_MESSAGE("LVLI {}:0x{:08x} found for Vendor Container contents", espName, lvliForm->GetFormID());
			vendorSentinels.insert(lvliForm);
		}
		else
		{
			REL_ERROR("LVLI {}/0x{:08x} not found, should be Vendor Container contents", espName, formID);
		}
	}
	if (vendorSentinels.size() != vendorItemLVLI.size())
	{
		REL_ERROR("LVLI count {} (base game) for Vendor Sentinels inconsistent with expected {}",
			vendorItemLVLI.size(), vendorSentinels.size());
	}

	// check mod-specific LVLI
	// TODO assumes no merge - mods, could be a problem
	// Trade & Barter.esp is well-behaved, using only core forms
	std::vector<std::tuple<std::string, RE::FormID>> modVendorSentinelLVLI = {
		{"Midwood Isle.esp", 0x142430},	// VendorGoldHermitMidwoodIsle
		{"Midwood Isle.esp", 0x19b10a},	// VendorGoldHunterMidwoodIsle
		{"AAX_Arweden.esp", 0x041dd1},	// AAX_VendorGold
		{"Complete Alchemy & Cooking Overhaul.esp", 0x97afe1},	// VendorGoldFarmer
		{"ccbgssse001-fish.esm", 0x1223de},	// ccBGSSSE001_LItemVendorFishingClothes50
		{"summersetisles.esp", 0x219171},	// VendorSSICoin
		{"summersetisles.esp", 0x3ffc4f},	// VendorGoldApothecarySSI
		{"summersetisles.esp", 0x3ffc50},	// VendorGoldBlacksmithSSI
		{"summersetisles.esp", 0x3ffc51},	// VendorGoldInnSSI
		{"summersetisles.esp", 0x3ffc52},	// VendorGoldMiscSSI
		{"summersetisles.esp", 0x3ffc53},	// VendorGoldSpellsSSI
		{"MiyapsDungeon.esm", 0x45155b},	// aaaaMDMVendorGoldMisc
		{"JerallMountainsCitadelPart1.esm", 0x26bef0}	// _JMCVendorGold
	};

	for (const auto& lvliDef : modVendorSentinelLVLI)
	{
		std::string espName(std::get<0>(lvliDef));
		RE::FormID formID(std::get<1>(lvliDef));
		RE::TESLevItem* lvliForm(FindExactMatch<RE::TESLevItem>(espName, formID));
		if (lvliForm)
		{
			REL_MESSAGE("LVLI {}:0x{:08x} found for Vendor Container contents", espName, lvliForm->GetFormID());
			vendorSentinels.insert(lvliForm);
		}
		else
		{
			REL_ERROR("LVLI {}/0x{:08x} not found, should be Vendor Container contents", espName, formID);
		}
	}

	// special and mod-added Containers to avoid looting
	std::vector<std::tuple<std::string, RE::FormID>> oneOffContainers = {
		// WEMerchantChest
		{"Skyrim.esm", 0xbbcd0},
		// DLC1dunRedwaterDenMerchantChest
		{"Dawnguard.esm", 0x13941},
		// DLC2dunFrostmoonVendorChest
		{"Dragonborn.esm", 0x1dc65},
		// DLC2MerchantDremoraChest
		{"Dragonborn.esm", 0x1eec0},
		// LoTD Museum Shipments
		{"LegacyoftheDragonborn.esm", 0x1772a6},	// Incoming
		{"LegacyoftheDragonborn.esm", 0x1772a7},	// Outgoing
		// HLIORemielVendorChest
		{"HLIORemi.esp", 0x4c786},
		//SL00MerchantGenericAmuletChest
		{"SL01AmuletsSkyrim.esp", 0x1de80},
		// SL00MerchantHoldNecklaceChest
		{"SL01AmuletsSkyrim.esp", 0x420c1},
		// SL00MerchantSolitudeGenericAmuletChest
		{"SL01AmuletsSkyrim.esp", 0x471e0},
		// ccVSVSSE001_MerchantPack
		{"ccvsvsse001-winter.esl", 0x809},
		// CYRMerchantBrumaMuseumChest
		{"BSHeartland.esm", 0xd24d1},
		// CYRMerchantBrumaLeoNewspaperChest
		{"BSHeartland.esm", 0xd3743},
		// CYRMerchantKvatchTheRightfulRemedyChest
		{"BSHeartland.esm", 0xf9704},
		// CYRMerchantKvatchTheRightfulRemedyChestExtra
		{"BSHeartland.esm", 0xf970f},
		// CYRMerchantKvatchFireheartHotelBarChest
		{"BSHeartland.esm", 0xf973f},
		// CH_IssalMerchantChest
		{"Coldhaven.esm", 0x3a84dc},
		// FSMerchantAudmund
		{"Falskaar.esm", 0x15c536},
		// Yadamerchantchest
		{"Jehanna.esp", 0x1ca84},
		// MerchantFlorinIarraChest
		{"Midwood Isle.esp", 0x56971},
		// MidwoodIsleMerchantTradesmanChest
		{"Midwood Isle.esp", 0x7f166e},
		// NyhusMerchantAreathchest
		{"Nyhus.esp", 0x7341a9},
		// NyhusMerchantTG04EECDockworker03
		{"Nyhus.esp", 0x8fdb65},
		// SB_SamMerchantChest
		{"SkyrimBounties.esp", 0x6b808},
		// SB_RedLanternMerchantChest
		{"SkyrimBounties.esp", 0x18acf4},
		// <N78HelheimGanglotMerchantChest>
		{"TrueHel.esp", 0x162e75},
		// MerchantWhiterunQuintus
		{"SurWR.esp", 0x2ecda0},
		// RiftenExtMerchantChestAnjiir
		{"RiftenExtension.esp", 0x105530},
		// 000FCLukiMerchantContainer
		{"ForgottenCity.esp", 0x2cc87},
		// 000FCStoreroomMerchantContainer
		{"ForgottenCity.esp", 0x2f3c8},
		// 000FCTailorMerchantContainer
		{"ForgottenCity.esp", 0x2f950},
		// LAMIOzirianaMerchantChest
		{"BetalillesHammerfellQuestBundle.esp", 0x1298},
		// ASHGMerchantDarkElfChest01
		{"BetalillesHammerfellQuestBundle.esp", 0x1733},
		// ASHGMerchantDarkElfChest02
		{"BetalillesHammerfellQuestBundle.esp", 0x1734},
		// aaaCJGMerchant1Chest1
		{"ClefJs Karthwasten.esp", 0x72e63},
		// SRC_LKHCharcoalMerchantChest
		{"SRC - All In One.esp", 0xeedc6},
		// _LP_MerchantBardWaresChest
		{"BecomeABard.esp", 0x5075b},
		// BSMMerchantChest
		{"BeyondSkyrimMerchant.esp", 0x55aa},
		// BUV_MerchantTGTuldinwaeChest,
		{"BUVARP SE RE.esp", 0x1254aa}
	};
	for (const auto& container : oneOffContainers)
	{
		std::string espName(std::get<0>(container));
		RE::FormID formID(std::get<1>(container));
		RE::TESObjectCONT* chestForm(FindExactMatch<RE::TESObjectCONT>(espName, formID));
		if (chestForm)
		{
			REL_MESSAGE("CONT {}:0x{:08x} added to Blacklist", espName, chestForm->GetFormID());
			m_containerBlackList.insert(chestForm);
		}
		else
		{
			DBG_MESSAGE("CONT {}/0x{:08x} not found", espName, formID);
		}
	}

	for (RE::TESObjectCONT* container : dhnd->GetFormArray<RE::TESObjectCONT>())
	{
		if (m_containerBlackList.contains(container))
		{
			DBG_MESSAGE("Skip already-blacklisted Container {}/0x{:08x}", container->GetName(), container->GetFormID());
			continue;
		}
		// does container have any sentinel Vendor Item?
		bool matched(false);
		container->ForEachContainerObject([&](RE::ContainerObject& entry) -> RE::BSContainer::ForEachResult {
			auto entryContents(entry.obj);
			if (vendorSentinels.find(entryContents->As<RE::TESLevItem>()) != vendorSentinels.cend())
			{
				REL_MESSAGE("Block Vendor Container {}/0x{:08x}", container->GetName(), container->GetFormID());
				matched = true;
				// only continue if insert fails, not that this will likely do much good
				return m_containerBlackList.insert(container).second ?
					RE::BSContainer::ForEachResult::kStop : RE::BSContainer::ForEachResult::kContinue;
			}
			else
			{
				DBG_MESSAGE("{}/0x{:08x} in Container {}/0x{:08x} not sentinel Vendor Item", entryContents->GetName(), entryContents->GetFormID(),
					container->GetName(), container->GetFormID());
			}
			// continue the scan
			return RE::BSContainer::ForEachResult::kContinue;
		});
		if (!matched)
		{
			DBG_MESSAGE("Ignoring non-Vendor Container {}/0x{:08x}", container->GetName(), container->GetFormID());
		}
	}
}

void DataCase::ExcludeImmersiveArmorsGodChest()
{
	// check for form in Load Order
	RE::TESObjectCONT* godChestForm(FindExactMatch<RE::TESObjectCONT>("Hothtrooper44_ArmorCompilation.esp", 0x4b352));
	if (godChestForm)
	{
		REL_MESSAGE("Block Immersive Armors 'all the loot' chest {}/0x{:08x}", godChestForm->GetName(), godChestForm->GetFormID());
		m_containerBlackList.insert(godChestForm);
	}
}

void DataCase::ExcludeGrayCowlStonesChest()
{
	// check for best matching candidate in Load Order - use exact match as the name is the very vague "Chest"
	RE::TESObjectCONT* stonesChestForm(FindExactMatch<RE::TESObjectCONT>("Gray Fox Cowl.esm", 0x1a184));
	if (stonesChestForm)
	{
		REL_MESSAGE("Block Gray Cowl Stones chest {}/0x{:08x}", stonesChestForm->GetName(), stonesChestForm->GetFormID());
		m_containerBlackList.insert(stonesChestForm);
	}
}

void DataCase::ExcludeMissivesBoards()
{
	// if Missives is installed and loads later than SHSE, conditionally blacklist the Noticeboards to avoid auto-looting of non-quest Missives
	static constexpr const char* modName = "Missives.esp";
	m_missivesBoards = CollectionManager::Collectibles().MutableCollectionByLabel(MissivesGroup, MissivesCollection);
	if (m_missivesBoards && (!LoadOrder::Instance().IncludesMod(modName) || LoadOrder::Instance().ModPrecedesSHSE(modName)))
	{
		REL_MESSAGE("Missives absent or loads before SHSE: disable Missive Board blacklist");
		// must disable the related Blacklist Collection
		m_missivesBoards->Disable();
	}
}

void DataCase::CheckAutoMiningOK()
{
	// check for incompatible Mining mods and disable auto-mining if any are active
	static constexpr const char* modName = "Advanced Mining.esp";
	m_miningDisabled = LoadOrder::Instance().IncludesMod(modName);
	REL_MESSAGE("Auto-mining disabled, found incompatible mod {}", modName);
}

bool DataCase::AutoMiningDisabled() const
{
	return m_miningDisabled;
}

void DataCase::ExcludeBuildYourNobleHouseIncomeChest()
{
	// check for matching form in Load Order
	RE::TESObjectCONT* incomeChestForm(FindExactMatch<RE::TESObjectCONT>("LC_BuildYourNobleHouse.esp", 0xaacdd));
	if (incomeChestForm)
	{
		REL_MESSAGE("Block 'Build Your Noble House' Income Chest {}/0x{:08x}", incomeChestForm->GetName(), incomeChestForm->GetFormID());
		m_containerBlackList.insert(incomeChestForm);
	}
}

void DataCase::IncludeFossilMiningExcavation()
{
	static std::string espName("Fossilsyum.esp");
	static RE::FormID excavationSiteFormID(0x3f41b);
	RE::TESForm* excavationSiteForm(LoadOrder::Instance().LookupForm(excavationSiteFormID, espName));
	if (excavationSiteForm)
	{
		REL_MESSAGE("Record Fossil Mining Excavation Site {}/0x{:08x} as oreVein:volcanicDigSite", excavationSiteForm->GetName(), excavationSiteForm->GetFormID());
		SetObjectTypeForForm(excavationSiteForm, ObjectType::oreVein);
		m_resourceTypeByOreVein.insert(std::make_pair(excavationSiteForm->As<RE::TESObjectACTI>(), ResourceType::volcanicDigSite));
	}
}

void DataCase::IncludeSeptimSpecialCases()
{
	static std::string espName("Dragonborn.esm");
	static std::vector<RE::FormID> pilesOfGold({ 0x18486, 0x18488 });
	for (const auto goldPileFormID : pilesOfGold)
	{
		RE::TESForm* goldPileForm(RE::TESDataHandler::GetSingleton()->LookupForm(goldPileFormID, espName));
		if (goldPileForm)
		{
			ForceObjectTypeForForm(goldPileForm, ObjectType::septims);
		}
	}
	IncludeCoinReplacerRedux();
	IncludeCorpseCoinage();
	IncludeBSBruma();
	IncludeToolsOfKagrenac();
	IncludeCOIN();
}

void DataCase::IncludeCoinReplacerRedux()
{
	// Coin Replacer Redux adds similar
	static std::string crrName("SkyrimCoinReplacerRedux.esp");
	static std::vector<RE::FormID> pilesOfCoin({ 0x800, 0x801, 0x802 });
	for (const auto coinPileFormID : pilesOfCoin)
	{
		RE::TESForm* coinPileForm(LoadOrder::Instance().LookupForm(coinPileFormID, crrName));
		if (coinPileForm)
		{
			ForceObjectTypeForForm(coinPileForm, ObjectType::septims);
		}
	}
}

void DataCase::IncludeCorpseCoinage()
{
	static std::string espName("CorpseToCoinage.esp");
	static RE::FormID corpseCoinageFormID(0xaa03);
	RE::TESForm* corpseCoinageForm(LoadOrder::Instance().LookupForm(corpseCoinageFormID, espName));
	if (corpseCoinageForm)
	{
		ForceObjectTypeForForm(corpseCoinageForm, ObjectType::septims);
	}
}

void DataCase::IncludeBSBruma()
{
	static std::string espName("BSAssets.esm");
	static RE::FormID ayleidGoldFormID(0x6028dc);
	RE::TESForm* ayleidGoldForm(RE::TESDataHandler::GetSingleton()->LookupForm(ayleidGoldFormID, espName));
	if (ayleidGoldForm)
	{
		ForceObjectTypeForForm(ayleidGoldForm, ObjectType::septims);
	}
}

void DataCase::IncludeToolsOfKagrenac()
{
	static std::string espName("Tools of Kagrenac.esp");
	static RE::FormID ayleidGoldFormID(0x6028dc);
	RE::TESForm* ayleidGoldForm(RE::TESDataHandler::GetSingleton()->LookupForm(ayleidGoldFormID, espName));
	if (ayleidGoldForm)
	{
		ForceObjectTypeForForm(ayleidGoldForm, ObjectType::septims);
	}
}

void DataCase::IncludeCOIN()
{
	static std::string crrName("C.O.I.N.esp");
	static std::vector<RE::FormID> pilesOfInterestingCoin({ 0x810, 0x9c6 });
	for (const auto coinPileFormID : pilesOfInterestingCoin)
	{
		RE::TESForm* coinPileForm(LoadOrder::Instance().LookupForm(coinPileFormID, crrName));
		if (coinPileForm)
		{
			ForceObjectTypeForForm(coinPileForm, ObjectType::septims);
		}
	}
	static std::string injectedName("Update.esm");
	static std::vector<RE::FormID> interestingCoins({
		0xde5012, 0xde5013, 0xde5014, 0xde5015, 0xde5016, 0xde5017,
	 	0xde5018, 0xde5019, 0xde5020, 0xde5021, 0xde5022, 0xde5023, 0xde5024 });
	for (const auto coinFormID : interestingCoins)
	{
		RE::TESForm* coinForm(RE::TESDataHandler::GetSingleton()->LookupForm(coinFormID, injectedName));
		if (coinForm)
		{
			ForceObjectTypeForForm(coinForm, ObjectType::septims);
		}
	}
}

void DataCase::HandleHearthfireExtendedApiary()
{
	static std::string espName("hearthfireextended.esp");
	static RE::FormID apiaryFormID(0xd62);
	const RE::TESForm* apiaryForm(LoadOrder::Instance().LookupForm(apiaryFormID, espName));
	if (apiaryForm && apiaryForm->Is(RE::FormType::Activator))
	{
		// the ACTI can be inspected repeatedly and (after first pass) fruitlessly if we do not prevent it
		AddFirehose(apiaryForm);
	}
}

void DataCase::RecordUnderwear()
{
	const std::string GroupName("SpecialCases");
	const std::string CollectionName("SHSE-Underwear");
	m_underwear = CollectionManager::SpecialCases().CollectionByLabel(GroupName, CollectionName);
}

void DataCase::RecordOffLimitsLocations()
{
	DBG_MESSAGE("Pre-emptively block all off-limits locations");
	std::vector<std::tuple<std::string, RE::FormID>> illegalCells = {
		{"Skyrim.esm", 0x32ae7},					// QASmoke
		{"Skyrim.esm", 0x6c3b6},					// AAADeleteWhenDoneTestJeremy
		{"LC_BuildYourNobleHouse.esp", 0x20c0b},	// LCBYNPCplanter00xx
		{"CerwidenCompanion.esp", 0x4a4bb},			// kcfAssetsCell01
		{"konahrik_accoutrements.esp", 0x625d3},	// KAxTestCell
		{"Helgen Reborn.esp", 0xa886cd}				// aaaBalokDummyCell
	};
	for (const auto& pluginForm : illegalCells)
	{
		std::string espName(std::get<0>(pluginForm));
		RE::FormID formID(std::get<1>(pluginForm));
		RE::TESObjectCELL* cell(FindExactMatch<RE::TESObjectCELL>(espName, formID));
		if (cell)
		{
			REL_MESSAGE("No looting in cell {}/0x{:08x}", cell->GetName(), cell->GetFormID());
			m_offLimitsLocations.insert({ cell->GetFormID(), "" });
		}
	}
}

void DataCase::RecordPlayerHouseCells(void)
{
	DBG_MESSAGE("Record free CELLs that are actually Player Houses");
	std::vector<std::tuple<std::string, RE::FormID>> houseCells = {
		{"Helgen Reborn.esp", 0x4a592},			// aaaBalokTowerDisplay
		{"Helgen Reborn.esp", 0x28dac},			// aaaBalokTowerLower
		{"Helgen Reborn.esp", 0x28d9a}			// aaaBalokTower
	};
	for (const auto& pluginForm : houseCells)
	{
		std::string espName(std::get<0>(pluginForm));
		RE::FormID formID(std::get<1>(pluginForm));
		const RE::TESObjectCELL* cell(FindExactMatch<RE::TESObjectCELL>(espName, formID));
		if (cell)
		{
			REL_MESSAGE("Cell {}/0x{:08x} treated as Player House", cell->GetName(), cell->GetFormID());
			PlayerHouses::Instance().SetCell(cell);
		}
	}
}

void DataCase::BlockOffLimitsContainers()
{
	// block all the known off-limits containers - list is invariant during gaming session
	RecursiveLockGuard guard(m_blockListLock);
	for (const auto refrID : m_offLimitsContainers)
	{
		BlockReferenceByID(refrID, Lootability::ContainerPermanentlyOffLimits);
	}
}

void DataCase::BlockFirehoseSource(const RE::TESObjectREFR* refr)
{
	RecursiveLockGuard guard(m_blockListLock);
	if (!refr)
		return;
	// looted REFR was 'blocked while I am in this cell' before the triggering event was fired
	m_firehoseSources.insert(refr->GetFormID());
	BlockReference(refr, Lootability::CannotRelootFirehoseSource);
}

// unblock mineable - may or may not actually be firehose
void DataCase::ForgetFirehoseSource(const RE::TESObjectREFR* refr)
{
	RecursiveLockGuard guard(m_blockListLock);
	m_firehoseSources.erase(refr->GetFormID());
	m_blockRefr.erase(refr->GetFormID());
}

void DataCase::ForgetFirehoseSources()
{
	RecursiveLockGuard guard(m_blockListLock);
	m_firehoseSources.clear();
}

bool DataCase::IsFirehose(const RE::TESForm* form) const
{
	RecursiveLockGuard guard(m_blockListLock);
	return m_firehoseForms.contains(form);
}

void DataCase::AddFirehose(const RE::TESForm* form)
{
	RecursiveLockGuard guard(m_blockListLock);
	REL_MESSAGE("Record 0x{:08x}/{} as FireHose", form->GetFormID(), form->GetName());
	m_firehoseForms.insert(form);
}

void DataCase::BlockReference(const RE::TESObjectREFR* refr, const Lootability reason)
{
	if (!refr
#if NDEBUG && !defined(_FULL_LOGGING)
	// dynamic forms must never be recorded as their FormID may be reused
	 || refr->IsDynamicForm()
#endif	 
	 )
		return;
	BlockReferenceByID(refr->GetFormID(), reason);
}

void DataCase::BlockReferenceByID(const RE::FormID refrID, const Lootability reason)
{
	RecursiveLockGuard guard(m_blockListLock);
	// store most recently-generated reason
	m_blockRefr[refrID] = reason;
}

Lootability DataCase::IsReferenceBlocked(const RE::TESObjectREFR* refr) const
{
	if (!refr)
		return Lootability::NullReference;
	// dynamic forms may be recorded even though their FormID may be reused
	RecursiveLockGuard guard(m_blockListLock);
	const auto blocked(m_blockRefr.find(refr->GetFormID()));
	return blocked == m_blockRefr.cend() ? Lootability::Lootable : blocked->second;
}

void DataCase::ResetBlockedReferences(const bool gameReload)
{
	RecursiveLockGuard guard(m_blockListLock);
	// settings change may make things lootable
	m_blockRefr.clear();
	if (gameReload)
	{
		DBG_MESSAGE("Reset entire list of blocked REFRs");
		ForgetFirehoseSources();
		m_syntheticFloraHarvested.clear();
		return;
	}
	// Volcanic dig sites from Fossil Mining and firehose item sources are only cleared on game reload,
	// to simulate the delay in becoming lootable again.
	// Only allow one auto-loot visit per gaming session, unless player dies.
	// Firehose item sources are BYOH mined materials and HearthfiresExtended Apiary
	decltype(m_firehoseSources) firehouseSources(m_firehoseSources);
	for (const auto refrID : m_blockRefr)
	{
		RE::TESForm* form(RE::TESForm::LookupByID(refrID.first));
		if (!form)
			continue;
		RE::TESObjectREFR* refr(form->As<RE::TESObjectREFR>());
		if (!refr)
			continue;
		if (GetEffectiveObjectType(refr->GetBaseObject()) == ObjectType::oreVein &&
			OreVeinResourceType(refr->GetBaseObject()->As<RE::TESObjectACTI>()) == ResourceType::volcanicDigSite)
		{
			firehouseSources.insert(refrID.first);
		}
	}
	DBG_MESSAGE("Reset blocked REFRs apart from {} volcanic and {} firehose",
		firehouseSources.size() - m_firehoseSources.size(), m_firehoseSources.size());
	for (const auto digSite : firehouseSources)
	{
		m_blockRefr.insert({ digSite, Lootability::CannotRelootFirehoseSource });
	}
	BlockOffLimitsContainers();
}

void DataCase::UnblockReferences(const Lootability reason)
{
	RecursiveLockGuard guard(m_blockListLock);
	for (auto refr = m_blockRefr.begin(); refr != m_blockRefr.end(); ) 
	{
		if (refr->second == reason)
		{
			DBG_VMESSAGE("Reset blocked REFR 0x{:08x} with reason {}", refr->first, LootabilityName(reason));
			refr = m_blockRefr.erase(refr);
		}
		else
		{
			++refr;
		}
	}
}

bool DataCase::BlacklistReference(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return false;
#if NDEBUG && !defined(_FULL_LOGGING)
	// dynamic forms must never be recorded as their FormID may be reused
	if (refr->IsDynamicForm())
		return false;
#endif	 
	RecursiveLockGuard guard(m_blockListLock);
	return (m_blacklistRefr.insert(refr->GetFormID())).second;
}

bool DataCase::IsReferenceOnBlacklist(const RE::TESObjectREFR* refr) const
{
	if (!refr)
		return false;
#if NDEBUG && !defined(_FULL_LOGGING)
	// dynamic forms must never be recorded as their FormID may be reused
	if (refr->IsDynamicForm())
		return false;
#endif	 
	RecursiveLockGuard guard(m_blockListLock);
	return m_blacklistRefr.contains(refr->GetFormID());
}

void DataCase::ClearReferenceBlacklist()
{
	DBG_MESSAGE("Reset blacklisted REFRs");
	RecursiveLockGuard guard(m_blockListLock);
	m_blacklistRefr.clear();
}

void DataCase::ClearReferenceBlacklistEphemeral()
{
#if DEBUG || defined(_FULL_LOGGING)
	DBG_MESSAGE("Reset ephemeral blacklisted REFRs");
	RecursiveLockGuard guard(m_blockListLock);
	for (auto refr = m_blacklistRefr.begin(); refr != m_blacklistRefr.end(); )
	{
		if ((*refr) < 0xFF000000)
		{
			refr = m_blacklistRefr.erase(refr);
		}
		else
		{
			++refr;
		}
	}
#endif
}

void DataCase::RefreshKnownIngredients()
{
	RE::PlayerCharacter* player(RE::PlayerCharacter::GetSingleton());
	if (!player)
		return;

	RecursiveLockGuard guard(m_blockListLock);
	for (auto& ingredientFlag : m_ingredientEffectsKnown)
	{
		RE::IngredientItem* ingredient(RE::TESForm::LookupByID<RE::IngredientItem>(ingredientFlag.first));
		if (!ingredient)
		{
			ingredientFlag.second = false;		// assume unknown effects exist
			continue;
		}
		ingredientFlag.second = AllEffectsKnown(ingredient);
	}
}

bool DataCase::IsIngredientKnown(const RE::TESForm* form) const
{
	const RE::IngredientItem* ingredient(form->As<RE::IngredientItem>());
	if (!ingredient)
		return false;
	RecursiveLockGuard guard(m_blockListLock);
	auto ingredientFlag(m_ingredientEffectsKnown.find(ingredient->GetFormID()));
	if (ingredientFlag == m_ingredientEffectsKnown.end())
		return false;
	if (!ingredientFlag->second)
	{
		// not known on last check - recheck and update
		ingredientFlag->second = AllEffectsKnown(ingredient);
	}
	return ingredientFlag->second;
}

bool DataCase::AllEffectsKnown(const RE::IngredientItem* ingredient) const
{
	bool known(true);		// if we find one we don't know, this flips
	for (const auto effect : ingredient->effects)
	{
		if (!effect->baseEffect->GetKnown())
		{
			DBG_VMESSAGE("Found unknown effect {}/0x{:08x} on ingredient {}/0x{:08x}", effect->baseEffect->GetFullName(),
				effect->baseEffect->GetFormID(), ingredient->GetFullName(), ingredient->GetFormID());
			known = false;
			break;
		}
	}
	if (known)
	{
		DBG_VMESSAGE("All effects known for ingredient {}/0x{:08x}", ingredient->GetFullName(), ingredient->GetFormID());
	}
	return known;
}

bool DataCase::IsEphemeralForm(const RE::TESForm* form) const
{ 
	return form->GetFormType() != RE::FormType::AlchemyItem && form->IsDynamicForm();
}

bool DataCase::BlockForm(const RE::TESForm* form, const Lootability reason)
{
	if (!form)
		return false;
	// ephemeral dynamic forms must never be recorded as their FormID may be reused
	if (IsEphemeralForm(form))
		return false;
	RecursiveLockGuard guard(m_blockListLock);
	return (m_blockForm.insert({ form, reason })).second;
}

Lootability DataCase::IsFormBlocked(const RE::TESForm* form) const
{
	if (!form)
		return Lootability::NullReference;
	// ephemeral dynamic forms must never be recorded as their FormID may be reused
	if (IsEphemeralForm(form))
		return Lootability::Lootable;
	RecursiveLockGuard guard(m_blockListLock);
	const auto matched(m_blockForm.find(form));
	if (matched != m_blockForm.cend())
	{
		return matched->second;
	}
	return Lootability::Lootable;
}

void DataCase::ResetBlockedForms()
{
	DBG_MESSAGE("Reset Blocked Forms");
	RecursiveLockGuard guard(m_blockListLock);
	m_blockForm = m_permanentBlockedForms;
}

// used for BlackList Collections. Also blocks the form for this loaded game, and on reload.
bool DataCase::BlockFormPermanently(const RE::TESForm* form, const Lootability reason)
{
	if (!form)
		return false;
#if NDEBUG && !defined(_FULL_LOGGING)
	// dynamic forms must never be recorded as their FormID may be reused
	if (form->IsDynamicForm())
		return false;
#endif	 
	RecursiveLockGuard guard(m_blockListLock);
	BlockForm(form, reason);
	return (m_permanentBlockedForms.insert({ form, reason })).second;
}

ObjectType DataCase::GetFormObjectType(const RE::FormID formID) const
{
	RecursiveLockGuard guard(m_formToTypeLock);
	const auto entry(m_objectTypeByForm.find(formID));
	if (entry != m_objectTypeByForm.cend())
		return entry->second;
	return ObjectType::unknown;
}

bool DataCase::SetObjectTypeForFormID(const RE::FormID formID, const ObjectType objectType)
{
	const RE::TESForm* target(RE::TESForm::LookupByID(formID));
	if (target)
	{
		return SetObjectTypeForForm(target, objectType);
	}
	else
	{
		REL_WARNING("FormID 0x{:08x} cannot be loaded, ignoring attempt to categorize as {}", formID, GetObjectTypeName(objectType));
		return false;
	}
}

void DataCase::RegisterPlayerCreatedALCH(RE::AlchemyItem* consumable)
{
	if (!consumable)
		return;
	RecursiveLockGuard guard(m_formToTypeLock);
	if (m_objectTypeByForm.contains(consumable->GetFormID()))
		return;
	ObjectType objectType(ConsumableObjectType<RE::AlchemyItem>(consumable));
	REL_MESSAGE("Register player-created ALCH {}/0x{:08x} as {}", consumable->GetFullName(), consumable->GetFormID(), GetObjectTypeName(objectType));
	SetObjectTypeForForm(consumable, objectType);
}

[[nodiscard]] bool DataCase::IsSyntheticFlora(const RE::TESBoundObject* boundObject) const
{
	if (!boundObject)
		return false;
	RecursiveLockGuard guard(m_blockListLock);
	const RE::TESObjectACTI* activator(boundObject->As<RE::TESObjectACTI>());
	return activator && m_syntheticFlora.contains(activator);
}

void DataCase::ClearSyntheticFlora(const RE::TESBoundObject* boundObject)
{
	if (!boundObject)
		return;
	RecursiveLockGuard guard(m_blockListLock);
	const RE::TESObjectACTI* activator(boundObject->As<RE::TESObjectACTI>());
	m_syntheticFlora.erase(activator);
}

[[nodiscard]] bool DataCase::IsSyntheticFloraHarvested(const RE::TESObjectREFR* candidate) const
{
	RecursiveLockGuard guard(m_blockListLock);
	if (candidate)
	{
		auto harvestedState(m_syntheticFloraHarvested.find(candidate));
		if (harvestedState != m_syntheticFloraHarvested.end())
		{
			// reset after a sensible interval so we can check if the synthetic flora script has respawned the yielded loot
			constexpr float SyntheticFloraResetInterval(5.0);
			float elapsed(RE::Calendar::GetSingleton()->GetCurrentGameTime() - harvestedState->second.second);
			if (elapsed > SyntheticFloraResetInterval)
			{
				REL_MESSAGE("Reset Harvested flag for REFR 0x{:08x} synthetic Flora 0x{:08x} after {:.2f} game-days",
					candidate->GetFormID(), candidate->GetBaseObject()->GetFormID(), elapsed);
				harvestedState->second.first = false;
			}
			return harvestedState->second.first;
		}
	}
	return false;
}

void DataCase::SetSyntheticFloraHarvested(const RE::TESObjectREFR* candidate, const bool isHarvested)
{
	RecursiveLockGuard guard(m_blockListLock);
	if (candidate)
	{
		m_syntheticFloraHarvested[candidate] = {isHarvested, RE::Calendar::GetSingleton()->GetCurrentGameTime()};
	}
}

bool DataCase::SetObjectTypeForForm(const RE::TESForm* form, const ObjectType objectType)
{
	const auto inserted(m_objectTypeByForm.insert(std::make_pair(form->GetFormID(), objectType)));
	std::string name(form->GetName());
	if (name.empty())
	{
		name = FormUtils::SafeGetFormEditorID(form);
	}
	if (inserted.second)
	{
		REL_VMESSAGE("{}/0x{:08x} uses ObjectType {}", name, form->GetFormID(), GetObjectTypeName(objectType));
		if (objectType == ObjectType::ingredient)
		{
			m_ingredientEffectsKnown.insert({ form->GetFormID(), false});
		}
	}
	else if (objectType != inserted.first->second)
	{
		REL_WARNING("Cannot use ObjectType {} for {}/0x{:08x}, already using {}", GetObjectTypeName(objectType),
			name, form->GetFormID(), GetObjectTypeName(inserted.first->second));
	}
	return inserted.second;
}

void DataCase::ForceObjectTypeForForm(const RE::TESForm* form, const ObjectType objectType)
{
	auto inserted(m_objectTypeByForm.insert(std::make_pair(form->GetFormID(), objectType)));
	if (inserted.second)
	{
		REL_VMESSAGE("{}/0x{:08x} force-categorized as {}", form->GetName(), form->GetFormID(), GetObjectTypeName(objectType).c_str());
		if (objectType == ObjectType::ingredient)
		{
			m_ingredientEffectsKnown.insert({ form->GetFormID(), false });
		}
	}
	else
	{
		REL_WARNING("{}/0x{:08x} force-categorized as {}, overwriting {}", form->GetName(), form->GetFormID(),
			GetObjectTypeName(objectType), GetObjectTypeName(inserted.first->second));
		inserted.first->second = objectType;
	}
}

ObjectType DataCase::GetObjectTypeForFormType(const RE::FormType formType) const
{
	const auto entry(m_objectTypeByFormType.find(formType));
	if (entry != m_objectTypeByFormType.cend())
		return entry->second;
	return ObjectType::unknown;
}

bool DataCase::SetObjectTypeForFormType(const RE::FormType formType, const ObjectType objectType)
{
	const auto inserted(m_objectTypeByFormType.insert({ formType, objectType }));
	if (inserted.second)
	{
		REL_VMESSAGE("Formtype {} categorized as {}", GetFormTypeName(formType), GetObjectTypeName(objectType));
	}
	else
	{
		REL_WARNING("Cannot categorize FormType {} as {}, already stored as {}", GetFormTypeName(formType),
			GetObjectTypeName(objectType), GetObjectTypeName(inserted.first->second));
	}
	return inserted.second;
}

ObjectType DataCase::GetObjectTypeForForm(const RE::TESForm* form) const
{
	ObjectType objectType(GetObjectTypeForFormType(form->GetFormType()));
	if (objectType == ObjectType::unknown)
	{
		objectType = GetFormObjectType(form->formID);
	}
	return objectType;
}

ResourceType DataCase::OreVeinResourceType(const RE::TESObjectACTI* mineable) const
{
	const auto matched(m_resourceTypeByOreVein.find(mineable));
	if (matched != m_resourceTypeByOreVein.cend())
		return matched->second;
	return ResourceType::ore;
}

const char* DataCase::GetTranslation(const char* key) const
{
	const auto& translation(m_translations.find(key));
	if (translation == m_translations.cend())
		return nullptr;
	return translation->second.c_str();
}

const RE::TESBoundObject* DataCase::ConvertIfLeveledItem(const RE::TESBoundObject* form) const
{
	const RE::TESProduceForm* produceForm(form->As<RE::TESProduceForm>());
	if (produceForm)
	{
		const auto matched(m_produceFormContents.find(produceForm));
		if (matched != m_produceFormContents.cend())
		{
			return matched->second;
		}
	}
	return form;
}

void DataCase::ListsClear(const bool gameReload)
{
	RecursiveLockGuard guard(m_blockListLock);
	DBG_MESSAGE("Clear arrow history");
	m_arrowCheck.clear();

	// only clear blacklist on game reload
	if (gameReload)
	{
		ClearReferenceBlacklist();
		RefreshKnownIngredients();
	}
	// reset blocked Base Objects and REFRs, reseed with off-limits containers
	ResetBlockedForms();
	ResetBlockedReferences(gameReload);
	ClearReferenceBlacklistEphemeral();
}

bool DataCase::SkipAmmoLooting(RE::TESObjectREFR* refr)
{
	bool skip(false);
	RE::NiPoint3 pos = refr->GetPosition();
	if (pos == RE::NiPoint3())
	{
		DBG_VMESSAGE("Arrow position unknown {:0.2f},{:0.2f},{:0.2f}", pos.x, pos.y, pos.z);
		BlockReference(refr, Lootability::CorruptArrowPosition);
		skip = true;
	}

	RecursiveLockGuard guard(m_blockListLock);
	if (!m_arrowCheck.contains(refr))
	{
		DBG_VMESSAGE("Newly detected, save arrow position {:0.2f},{:0.2f},{:0.2f}", pos.x, pos.y, pos.z);
		m_arrowCheck.insert(std::make_pair(refr, pos));
		skip = true;
	}
	else
	{
		RE::NiPoint3 prev = m_arrowCheck.at(refr);
		if (prev != pos)
		{
			float dx(pos.x - prev.x);
			float dy(pos.y - prev.y);
			float dz(pos.z - prev.z);
			// check for in-flight arrow movement speed is scaled if player is current experiencing Slow Time
			const double ArrowInFlightUnitsPerScan(PlayerState::Instance().ArrowMovingThreshold());
			if (fabs(dx) > ArrowInFlightUnitsPerScan || fabs(dy) > ArrowInFlightUnitsPerScan || fabs(dz) > ArrowInFlightUnitsPerScan)
			{
				DBG_VMESSAGE("In flight: change in arrow position dx={:0.2f},dy={:0.2f},dz={:0.2f} vs {:0.2f}",
					dx, dy, dz, ArrowInFlightUnitsPerScan);
				m_arrowCheck[refr] = pos;
				skip = true;
			}
			else
			{
				DBG_VMESSAGE("OK, not in flight, change in arrow position dx={:0.2f},dy={:0.2f},dz={:0.2f} vs {:0.2f}",
					dx, dy, dz, ArrowInFlightUnitsPerScan);
			}
		}
		else
		{
			DBG_VMESSAGE("Arrow is stationary");
		}
		if (!skip)
		{
			m_arrowCheck.erase(refr);
		}
	}
	return skip;
}

void DataCase::CategorizeLootables()
{
	// used to taxonomize ACTIvators
	REL_MESSAGE("*** LOAD *** Load Text Translation");
	GetTranslationData();

	REL_MESSAGE("*** LOAD *** Store Activation Verbs");
	StoreActivationVerbs();

	REL_MESSAGE("*** LOAD *** Categorize Statics");
	CategorizeStatics();

	REL_MESSAGE("*** LOAD *** Set Object Type By Keywords");
	SetObjectTypeByKeywords();

	// consumable item categorization is useful for Activator, Flora, Tree and direct access
	REL_MESSAGE("*** LOAD *** Categorize Consumable: ALCH");
	CategorizeConsumables<RE::AlchemyItem>();

	REL_MESSAGE("*** LOAD *** Categorize Consumable: INGR");
	CategorizeConsumables<RE::IngredientItem>();

	REL_MESSAGE("*** LOAD *** Categorize by Keyword: MISC");
	CategorizeByKeyword<RE::TESObjectMISC>();

	// force-categorize all special cases yielding gold or coins
	IncludeSeptimSpecialCases();

	// Classes inheriting from TESProduceForm may have an ingredient, categorized as the appropriate consumable
	// This 'ingredient' can be MISC (e.g. Coin Replacer Redux Coin Purses) so those must be done first, as above by keyword
	// or forcing as septims
	REL_MESSAGE("*** LOAD *** Categorize by Ingredient: FLOR");
	CategorizeByIngredient<RE::TESFlora>();

	REL_MESSAGE("*** LOAD *** Categorize by Ingredient: TREE");
	CategorizeByIngredient<RE::TESObjectTREE>();

	REL_MESSAGE("*** LOAD *** Categorize by Keyword: ARMO");
	CategorizeByKeyword<RE::TESObjectARMO>();

	REL_MESSAGE("*** LOAD *** Categorize by Keyword: WEAP");
	CategorizeByKeyword<RE::TESObjectWEAP>();

	// Activators are done last, deterministic categorization above is preferable
	REL_MESSAGE("*** LOAD *** Categorize by Activation Verb ACTI");
	CategorizeByActivationVerb();

#if _DEBUG
	for (const auto& unhandledVerb : m_unhandledActivationVerbs)
	{
		DBG_VMESSAGE("Activation verb {} unhandled at present", unhandledVerb);
	}
#endif
	// crafting components
	REL_MESSAGE("*** LOAD *** Get Crafting Items");
	FindCraftingItems();

	// Analyze perks that affect looting
	DBG_MESSAGE("*** LOAD *** Analyze Perks");
	AnalyzePerks();

	// Analyze NPC Race and Keywords for dead body filtering
	DBG_MESSAGE("*** LOAD *** Analyze NPC Race and Keywords");
	NPCFilter::Instance().Load();

	// Handle any special cases based on Load Order, including base game 'known exceptions'
	REL_MESSAGE("*** LOAD *** Detect and Handle Exceptions");
	HandleExceptions();
}

void DataCase::RefreshBuiltinSpecialCases()
{
	CollectionManager::SpecialCases().OnGameReload();

	// blacklist underwear in case user has "don't show me their junk" setting turned on
	RecordUnderwear();
	// Blacklist Missives Boards, if present and so ordered 
	ExcludeMissivesBoards();
}

bool DataCase::UseUnderwear() const
{
	return m_underwear && m_underwear->IsActive();
}

bool DataCase::IsUnderwear(const RE::TESForm* armour) const
{
	ConditionMatcher matcher(armour);
	return m_underwear->IsMemberOf(matcher);
}

void DataCase::HandleExceptions()
{
	// on first pass, detect off limits containers and other special cases to avoid rescan on game reload
	DBG_MESSAGE("Pre-emptively handle special cases from Load Order");
	ExcludeImmersiveArmorsGodChest();
	ExcludeGrayCowlStonesChest();
	ExcludeBuildYourNobleHouseIncomeChest();

	ExcludeFactionContainers();
	ExcludeVendorContainers();

	shse::PlayerState::Instance().ExcludeMountedIfForbidden();
	RecordOffLimitsLocations();
	RecordPlayerHouseCells();

	// whitelist Fossil sites
	IncludeFossilMiningExcavation();
	// whitelist Hearthfire Extended Apiary
	HandleHearthfireExtendedApiary();

	// exclude auto-mining if incompatible mods loaded
	CheckAutoMiningOK();
}

ObjectType DataCase::DecorateIfEnchanted(const RE::TESForm* form, const ObjectType rawType)
{
	const RE::TESEnchantableForm* enchantable(form->As<RE::TESEnchantableForm>());
	if (enchantable && enchantable->formEnchanting)
	{
		if (rawType == ObjectType::jewelry)
		{
			return ObjectType::enchantedJewelry;
		}
		else if (rawType == ObjectType::weapon)
		{
			return ObjectType::enchantedWeapon;
		}
		else
		{
			return ObjectType::enchantedArmor;
		}
	}
	return rawType;
}

// Classify items by their keywords
void DataCase::SetObjectTypeByKeywords()
{
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	if (!dhnd)
		return;

	std::unordered_map<std::string, ObjectType> typeByKeyword =
	{
		// Skyrim core
		{"ArmorLight", ObjectType::armor},
		{"ArmorHeavy", ObjectType::armor},
		{"VendorItemArrow", ObjectType::ammo},
		{"VendorItemBook", ObjectType::book},
		{"VendorItemRecipe", ObjectType::book},
		{"VendorItemGem", ObjectType::gem},
		{"VendorItemOreIngot", ObjectType::oreIngot},
		{"VendorItemAnimalHide", ObjectType::animalHide},
		{"VendorItemAnimalPart", ObjectType::animalHide},
		{"VendorItemJewelry", ObjectType::jewelry},
		{"VendorItemArmor", ObjectType::armor},
		{"VendorItemClothing", ObjectType::armor},
		{"VendorItemIngredient", ObjectType::ingredient},
		{"VendorItemKey", ObjectType::key},
		{"VendorItemPotion", ObjectType::potion},
		{"VendorItemPoison", ObjectType::poison},
		{"VendorItemScroll", ObjectType::scroll},
		{"VendorItemSpellTome", ObjectType::spellbook},
		{"VendorItemSoulGem", ObjectType::soulgem},
		{"VendorItemStaff", ObjectType::weapon},
		{"VendorItemWeapon", ObjectType::weapon},
		{"VendorItemClutter", ObjectType::clutter},
		{"VendorItemFireword", ObjectType::clutter},
		// Legacy of the Dragonborn
		{"VendorItemJournal", ObjectType::book},
		{"VendorItemNote", ObjectType::book},
		{"VendorItemFateCards", ObjectType::clutter},
		// Skyrim core
		{"WeapTypeBattleaxe", ObjectType::weapon},
		{"WeapTypeBoundArrow", ObjectType::ammo},
		{"WeapTypeBow", ObjectType::weapon},
		{"WeapTypeDagger", ObjectType::weapon},
		{"WeapTypeGreatsword", ObjectType::weapon},
		{"WeapTypeMace", ObjectType::weapon},
		{"WeapTypeStaff", ObjectType::weapon},
		{"WeapTypeSword", ObjectType::weapon},
		{"WeapTypeWarAxe", ObjectType::weapon},
		{"WeapTypeWarhammer", ObjectType::weapon},
		//CACO
		{"WAF_WeapTypeGrenade", ObjectType::weapon},
		{"WAF_WeapTypeScalpel", ObjectType::weapon}
	};
	std::vector<std::pair<std::string, ObjectType>> typeByVendorItemSubstring =
	{
		// All appear in Skyrim core, extended in mods e.g. CACO, SkyREM EVE
		// Order is important, we scan linearly during mod/data load
		{"Drink", ObjectType::drink},
		{"VendorItemFood", ObjectType::food},
		{"VendorItemDrink", ObjectType::drink}
	};
	std::unordered_set<std::string> glowableBooks = {
		// Legacy of the Dragonborn
		"VendorItemJournal",
		"VendorItemNote"
	};

	for (RE::BGSKeyword* keywordDef : dhnd->GetFormArray<RE::BGSKeyword>())
	{
		std::string keywordName(FormUtils::SafeGetFormEditorID(keywordDef));
		if (keywordName.empty())
		{
			REL_WARNING("KYWD record 0x{:08x} has missing/blank EDID, skip", keywordDef->GetFormID());
			continue;
		}
		// Store player house keyword for SearchTask usage
		if (keywordName == "LocTypePlayerHouse" || keywordName == "BYOHHouseLocationKeyword")
		{
			REL_VMESSAGE("Found PlayerHouse KYWD {}/0x{:08x}", keywordName, keywordDef->GetFormID());
			PlayerHouses::Instance().AddLocationKeyword(keywordDef);
			continue;
		}
		// Immersive Armors gives some armors the VendorItemSpellTome keyword. No, they are not spellBook
		if (keywordName == "VendorItemSpellTome")
		{
			REL_VMESSAGE("Found Spell Tome KYWD {}/0x{:08x}", keywordName, keywordDef->GetFormID());
			m_spellTomeKeyword = keywordDef;
		}
		// SPERG mining resource types
		if (keywordName == "VendorItemOreIngot" || keywordName == "VendorItemGem")
		{
			REL_VMESSAGE("Found SPERG Prospector Perk resource type KYWD {}/0x{:08x}", keywordName, keywordDef->GetFormID());
			ScanGovernor::Instance().SetSPERGKeyword(keywordDef);
		}
		if (glowableBooks.find(keywordName) != glowableBooks.cend())
		{
			REL_VMESSAGE("Found Glowable Book KYWD {}/0x{:08x}", keywordName, keywordDef->GetFormID());
			m_glowableBookKeywords.insert(keywordDef->GetFormID());
		}

		ObjectType objectType(ObjectType::unknown);
		const auto matched(typeByKeyword.find(keywordName));
		if (matched != typeByKeyword.cend())
		{
			objectType = matched->second;
		}
		else if (const auto substringMatch = std::find_if(typeByVendorItemSubstring.cbegin(), typeByVendorItemSubstring.cend(),
			[&](const std::pair<std::string, ObjectType>& comparand) -> bool
			{
				if (keywordName.find(comparand.first) != std::string::npos)
				{
					objectType = comparand.second;
					return true;
				}
				return false;
			}) != typeByVendorItemSubstring.cend())
		{
			DBG_VMESSAGE("KYWD 0x{:08x} ({}) matched substring", keywordDef->GetFormID(), keywordName);
		}
		else
		{
			DBG_VMESSAGE("KYWD 0x{:08x} ({}) skipped", keywordDef->GetFormID(), keywordName.c_str());
			continue;
		}
		SetObjectTypeForForm(keywordDef, DecorateIfEnchanted(keywordDef, objectType));
	}
}

template <> ObjectType DataCase::DefaultObjectType<RE::TESObjectARMO>()
{
	return ObjectType::armor;
}

template <> ObjectType DataCase::DefaultObjectType<RE::TESObjectWEAP>()
{
	return ObjectType::weapon;
}

template <> ObjectType DataCase::OverrideIfBadChoice<RE::TESObjectARMO>(const RE::TESForm* form, const ObjectType objectType)
{
	ObjectType rawType(objectType);
	if (rawType == ObjectType::animalHide)
		rawType = ObjectType::armor;
	return DecorateIfEnchanted(form, rawType);
}

template <> ObjectType DataCase::OverrideIfBadChoice<RE::TESObjectWEAP>(const RE::TESForm* form, const ObjectType objectType)
{
	return DecorateIfEnchanted(form, objectType);
}

template <> ObjectType DataCase::ConsumableObjectType<RE::AlchemyItem>(RE::AlchemyItem* consumable)
{
	const static RE::FormID drinkSound = 0x0B6435;
	if (consumable->IsFood())
		return (consumable->data.consumptionSound && consumable->data.consumptionSound->formID == drinkSound) ? ObjectType::drink : ObjectType::food;
	else
		return (consumable->IsPoison()) ? ObjectType::poison : ObjectType::potion;
}

template <> ObjectType DataCase::ConsumableObjectType<RE::IngredientItem>(RE::IngredientItem*)
{
	return ObjectType::ingredient;
}

bool DataCase::PerksAddLeveledItemsOnDeath(const RE::Actor* actor) const
{
	const auto deathPerk = std::find_if(m_leveledItemOnDeathPerks.cbegin(), m_leveledItemOnDeathPerks.cend(),
		[=] (const RE::BGSPerk* perk) -> bool { return actor->HasPerk(const_cast<RE::BGSPerk*>(perk)); });
	if (deathPerk != m_leveledItemOnDeathPerks.cend())
	{
		DBG_VMESSAGE("Leveled item added at death for perk {}/0x{:08x}", (*deathPerk)->GetName(), (*deathPerk)->GetFormID());
		return true;
	}
	return false;
}

float DataCase::PerkIngredientMultiplier(const RE::Actor* actor) const
{
	// default is one ingredient
	float result(1.0);
	const RE::BGSPerk* matched(nullptr);
	std::for_each(m_modifyHarvestedPerkMultipliers.cbegin(), m_modifyHarvestedPerkMultipliers.cend(),
		[&](const auto& perkEntry) {
		if (actor->HasPerk(const_cast<RE::BGSPerk*>(perkEntry.first)))
		{
			if (matched)
			{
				DBG_VMESSAGE("Perk conflict ingredient harvesting via {}/0x{:08x}, discarding", perkEntry.first->GetName(), perkEntry.first->GetFormID());
			}
			else
			{
				DBG_VMESSAGE("Perk {}/0x{:08x} used for harvesting, multiplier {:0.2f}", perkEntry.first->GetName(), perkEntry.first->GetFormID(), perkEntry.second);
				matched = perkEntry.first;
				result = perkEntry.second;
			}
		}
	});
	return result;
}

bool DataCase::IsSlowTimeEffectActive() const
{
	for (const auto effect : m_slowTimeEffects)
	{
		auto target = RE::PlayerCharacter::GetSingleton()->AsMagicTarget();
		if (target && target->HasMagicEffect(effect))
			return true;
	}
	return false;
}

std::string DataCase::GetModelPath(const RE::TESForm* thisForm) const
{
	if (thisForm)
	{
		const RE::TESObjectMISC* miscObject(thisForm->As<RE::TESObjectMISC>());
		if (miscObject)
			return miscObject->GetModel();
		const RE::TESObjectCONT* container(thisForm->As<RE::TESObjectCONT>());
		if (container)
			return container->GetModel();
	}
	return std::string();
}

bool DataCase::CheckObjectModelPath(const RE::TESForm* thisForm, const char* arg) const
{
	if (!thisForm || strlen(arg) == 0)
		return false;
	std::string s = GetModelPath(thisForm);
	if (s.empty())
		return false;
	StringUtils::ToLower(s);
	return (s.find(arg, 0) != std::string::npos) ? true : false;
}

void DataCase::CategorizeStatics()
{
	// These form types always map to the same Object Type
	SetObjectTypeForFormType(RE::FormType::ActorCharacter, ObjectType::actor);
	SetObjectTypeForFormType(RE::FormType::Container, ObjectType::container);
	SetObjectTypeForFormType(RE::FormType::Ingredient, ObjectType::ingredient);
	SetObjectTypeForFormType(RE::FormType::SoulGem, ObjectType::soulgem);
	SetObjectTypeForFormType(RE::FormType::KeyMaster, ObjectType::key);
	SetObjectTypeForFormType(RE::FormType::Scroll, ObjectType::scroll);
	SetObjectTypeForFormType(RE::FormType::Ammo, ObjectType::ammo);
	SetObjectTypeForFormType(RE::FormType::ProjectileArrow, ObjectType::ammo);
	SetObjectTypeForFormType(RE::FormType::Light, ObjectType::clutter);

	// Map well-known forms to ObjectType
	SetObjectTypeForFormID(LockPick, ObjectType::lockpick);
	SetObjectTypeForFormID(Gold, ObjectType::septims);
	auto wispCoreForm(RE::TESDataHandler::GetSingleton()->LookupForm(WispCore, "Skyrim.esm"));
	if (wispCoreForm->Is(RE::FormType::Activator))
	{
		ForceObjectTypeForForm(wispCoreForm, ObjectType::flora);
		m_syntheticFlora.insert(wispCoreForm->As<RE::TESObjectACTI>());
	}
	// WACCF makes this a Scroll. It's not a Scroll.
	SetObjectTypeForFormID(RollOfPaper, ObjectType::clutter);

	// record CACO items that are marked Potion but are not Potions
	static std::string cacoName("Complete Alchemy & Cooking Overhaul.esp");
	static std::vector<std::pair<RE::FormID, ObjectType>> nonPotions({
		{0x1cc0642,	ObjectType::ingredient},	// Beeswax
		{0x1cc0645,	ObjectType::ingredient},	// Distilled Alcohol
		{0x1cc0650,	ObjectType::clutter},		// Bandages
		{0x1cc0651,	ObjectType::clutter},		// Clean Bandages
		{0x1cca200,	ObjectType::clutter},		// Mortar and Pestle, Common
		{0x1cca201,	ObjectType::clutter},		// Alchemist's Retort
		{0x3db5a,	ObjectType::clutter},		// Mortar and Pestle, Bronze
		{0x27ee0d,	ObjectType::clutter},		// Mortar and Pestle, Silver
		{0x37203f,	ObjectType::clutter},		// Mortar and Pestle, Expert
		{0x372040,	ObjectType::clutter},		// Mortar and Pestle, Master
		{0x609cef,	ObjectType::clutter}		// Alchemist's Crucible
		});
	for (const auto nonPotion : nonPotions)
	{
		// using modname does not always work because formids with 0x1cc prefix are hard-coded to not follow the pattern
		RE::TESForm* nonPotionForm(RE::TESForm::LookupByID(nonPotion.first));
		if (!nonPotionForm)
		{
			nonPotionForm = RE::TESDataHandler::GetSingleton()->LookupForm(nonPotion.first, cacoName);
		}
		if (nonPotionForm)
		{
			SetObjectTypeForForm(nonPotionForm, nonPotion.second);
		}
	}

	// record Hearthfire building materials
	static std::string hearthFiresName("HearthFires.esm");
	static std::vector<RE::FormID> clayOrStone({ 0x9f2, 0x9f3, 0xA14, 0x306b, 0x310b, 0xa511 });
	for (const auto clayOrStoneFormID : clayOrStone)
	{
		RE::TESForm* clayOrStoneForm(RE::TESDataHandler::GetSingleton()->LookupForm(clayOrStoneFormID, hearthFiresName));
		if (clayOrStoneForm)
		{
			AddFirehose(clayOrStoneForm);
		}
	}
}

template <>
ObjectType DataCase::DefaultIngredientObjectType(const RE::TESFlora*)
{
	return ObjectType::flora;
}

template <>
ObjectType DataCase::DefaultIngredientObjectType(const RE::TESObjectTREE*)
{
	return ObjectType::food;
}

DataCase::ProduceFormCategorizer::ProduceFormCategorizer(
	RE::TESProduceForm* produceForm, const RE::TESLevItem* rootItem, const std::string& targetName) :
	LeveledItemCategorizer(rootItem), m_targetName(targetName), m_produceForm(produceForm),
	m_contents(nullptr), m_contentsType(ObjectType::unknown)
{
}

void DataCase::ProduceFormCategorizer::ProcessContentLeaf(RE::TESBoundObject* itemForm, ObjectType itemType)
{
	if (!m_contents)
	{
		REL_VMESSAGE("Target {}/0x{:08x} has contents type {} in form {}/0x{:08x}", m_targetName, m_rootItem->GetFormID(),
			GetObjectTypeName(itemType), itemForm->GetName(), itemForm->GetFormID());
		if (!DataCase::GetInstance()->m_produceFormContents.insert({ m_produceForm, itemForm }).second)
		{
			REL_WARNING("Leveled Item {}/0x{:08x} contents already present", m_targetName, m_rootItem->GetFormID());
		}
		else
		{
			DataCase::GetInstance()->SetObjectTypeForForm(itemForm, itemType);
			m_contents = itemForm;
			m_contentsType = itemType;
		}
	}
	else if (m_contents == itemForm)
	{
		DBG_VMESSAGE("Target {}/0x{:08x} contents type {} already recorded", m_targetName, m_rootItem->GetFormID(),
			GetObjectTypeName(itemType));
	}
	else
	{
		// 'ingredient' supersedes 'food' as object type - currently this is the only clash, and 'ingredient' is subjectively assumed more likely to be
		// wanted by the average user than 'food'
		ObjectType existingType(DataCase::GetInstance()->GetObjectTypeForForm(m_contents));
		if (itemType == ObjectType::ingredient && existingType == ObjectType::food)
		{
			REL_WARNING("Target {}/0x{:08x} contents type {} overwriting type {} for form {}/0x{:08x}", m_targetName, m_rootItem->GetFormID(),
				GetObjectTypeName(itemType), GetObjectTypeName(existingType), m_contents->GetName(), m_contents->GetFormID());
			DataCase::GetInstance()->ForceObjectTypeForForm(m_contents, itemType);
			m_contentsType = itemType;
		}
		else
		{
			REL_WARNING("Target {}/0x{:08x} contents type {} already stored under different form {}/0x{:08x} as type {}", m_targetName, m_rootItem->GetFormID(),
				GetObjectTypeName(itemType), m_contents->GetName(), m_contents->GetFormID(),
				GetObjectTypeName(existingType));
		}

	}
}

}
