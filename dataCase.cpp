#include "PrecompiledHeaders.h"
#include "PrecompiledHeaders.h"

#include "tasks.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

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


DataCase* DataCase::s_pInstance = nullptr;

DataCase::DataCase()
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

	DBG_MESSAGE("Reading translations from %s", path.c_str());
	RE::BSResourceNiBinaryStream fs(path.c_str());
	if (!fs.good())
		return;

	UInt16	bom = 0;
	bool	ret = fs.read(&bom, sizeof(UInt16) / sizeof(wchar_t));
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

		translations[keyS] = translationS;
		DBG_VMESSAGE("Translation entry: %s -> %s", keyS.c_str(), translationS.c_str());

	}
	DBG_MESSAGE("* TranslationData(%d)", translations.size());

	return;
}

// process comma-separated list of allowed ACTI verbs, to make localization INI-based
void DataCase::ActivationVerbsByType(const char* activationVerbKey, const ObjectType objectType)
{
	RE::BSString iniVerbs(GetTranslation(activationVerbKey));
	std::istringstream verbStream(iniVerbs.c_str());
	std::string nextVerb;
	while (std::getline(verbStream, nextVerb, ',')) {
		DBG_VMESSAGE("Activation verb %s has ObjectType %s", nextVerb.c_str(), GetObjectTypeName(objectType).c_str());
		m_objectTypeByActivationVerb[nextVerb] = objectType;
	}
}

// Some activation verbs are used to handle referenced forms as a catch-all, though we prefer other rules
void DataCase::StoreActivationVerbs()
{
	ActivationVerbsByType("$SHSE_ACTIVATE_VERBS_CLUTTER", ObjectType::clutter);
	ActivationVerbsByType("$SHSE_ACTIVATE_VERBS_CRITTER", ObjectType::critter);
	ActivationVerbsByType("$SHSE_ACTIVATE_VERBS_FLORA", ObjectType::flora);
	ActivationVerbsByType("$SHSE_ACTIVATE_VERBS_OREVEIN", ObjectType::oreVein);
	ActivationVerbsByType("$SHSE_ACTIVATE_VERBS_MANUAL", ObjectType::manualLoot);
}

ObjectType DataCase::GetObjectTypeForActivationText(const RE::BSString& activationText) const
{
	std::string verb(GetVerbFromActivationText(activationText));
	const auto verbMatched(m_objectTypeByActivationVerb.find(verb));
	if (verbMatched != m_objectTypeByActivationVerb.cend())
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

	for (RE::TESForm* form : dhnd->GetFormArray(RE::FormType::Activator))
	{
		RE::TESObjectACTI* typedForm(form->As<RE::TESObjectACTI>());
		if (!typedForm || !typedForm->GetFullNameLength())
			continue;
		const char* formName(typedForm->GetFullName());
		DBG_VMESSAGE("Categorizing %s/0x%08x by activation verb", formName, form->formID);

		ObjectType correctType(ObjectType::unknown);
		bool hasDefault(false);
		RE::BSString activationText;
		if (typedForm->GetActivateText(RE::PlayerCharacter::GetSingleton(), activationText))
		{
			ObjectType activatorType(GetObjectTypeForActivationText(activationText));
			if (activatorType != ObjectType::unknown)
			{
				if (SetObjectTypeForForm(form->formID, activatorType))
				{
					DBG_VMESSAGE("%s/0x%08x activated using '%s' categorized as %s", formName, form->formID,
						GetVerbFromActivationText(activationText).c_str(), GetObjectTypeName(activatorType).c_str());
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
						m_resourceTypeByOreVein.insert(std::make_pair(typedForm, resourceType));
						DBG_VMESSAGE("%s/0x%08x has ResourceType %s", formName, form->formID, PrintResourceType(resourceType));
					}
				}
				else
				{
					REL_WARNING("%s/0x%08x (%s) already stored, check data", formName, form->formID, GetObjectTypeName(activatorType).c_str());
				}
				continue;
			}
		}
		DBG_MESSAGE("%s/0x%08x not mappable, activated using '%s'", formName, form->formID, GetVerbFromActivationText(activationText).c_str());
	}
}

void DataCase::AnalyzePerks(void)
{
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	if (!dhnd)
		return;

	for (RE::TESForm* form : dhnd->GetFormArray(RE::FormType::Perk))
	{
		const RE::BGSPerk* perk(form->As<RE::BGSPerk>());
		if (!perk)
			continue;
		DBG_MESSAGE("Perk %s/0x%08x being checked", perk->GetName(), perk->GetFormID());
		for (const RE::BGSPerkEntry* perkEntry : perk->perkEntries)
		{
			if (perkEntry->GetType() != RE::PERK_ENTRY_TYPE::kEntryPoint)
				continue;

			const RE::BGSEntryPointPerkEntry* entryPoint(static_cast<const RE::BGSEntryPointPerkEntry*>(perkEntry));
			if (entryPoint->entryData.entryPoint == RE::BGSEntryPoint::ENTRY_POINT::kAddLeveledListOnDeath &&
				entryPoint->entryData.function == RE::BGSEntryPointPerkEntry::EntryData::Function::kAddLeveledList)
			{
				DBG_MESSAGE("Leveled items added on death by perk %s/0x%08x", perk->GetName(), perk->GetFormID());
				m_leveledItemOnDeathPerks.insert(perk);
				break;
			}
		}
	}
}

bool DataCase::GetTSV(std::unordered_set<RE::FormID> *tsv, const char* fileName)
{
	std::string filepath(FileUtils::GetPluginPath() + std::string(SHSE_NAME) + std::string("\\override\\") + std::string(fileName));
	std::ifstream ifs(filepath);
	if (ifs.fail())
	{
		REL_MESSAGE("* override TSV:%s inaccessible", filepath.c_str());
		filepath = FileUtils::GetPluginPath() + std::string(SHSE_NAME) + std::string("\\default\\") + std::string(fileName);
		ifs.open(filepath);
		if (ifs.fail())
		{
			REL_WARNING("* default TSV:%s inaccessible", filepath.c_str());
			return false;
		}
	}
	REL_MESSAGE("Using TSV file %s", filepath.c_str());

	// The correct file is open when we get here
	std::string str;
	while (getline(ifs, str))
	{
		if (str[0] == '#' || str[0] == ';' || (str[0] == '/' && str[1] == '/'))
			continue;

		if (str.find_first_not_of("\t") == std::string::npos)
			continue;

		auto vec = StringUtils::Split(str, '\t');
		std::string modName = vec[0];

		std::optional<UInt8> modIndex = RE::TESDataHandler::GetSingleton()->GetLoadedModIndex(vec[0].c_str());
		if (!modIndex.has_value())
			continue;

		UInt32 formID = std::stoul(vec[1], nullptr, 16);
		formID |= (modIndex.value() << 24);

		RE::TESForm* pForm = RE::TESForm::LookupByID(formID);
		if (pForm)
			tsv->insert(formID);
	}

	REL_MESSAGE("* TSV:%s(%d)", fileName, tsv->size());
	return true;
}

void DataCase::ExcludeImmersiveArmorsGodChest()
{
	static std::string espName("Hothtrooper44_ArmorCompilation.esp");
	static RE::FormID godChestFormID(0x4b352);
	RE::TESForm* godChestForm(RE::TESDataHandler::GetSingleton()->LookupForm(godChestFormID, espName));
	if (godChestForm)
	{
		DBG_MESSAGE("Block Immersive Armors 'all the loot' chest %s(0x%08x)", godChestForm->GetName(), godChestForm->GetFormID());
		m_offLimitsForms.insert(godChestForm);
	}
}

void DataCase::IncludeFossilMiningExcavation()
{
	static std::string espName("Fossilsyum.esp");
	static RE::FormID excavationSiteFormID(0x3f41b);
	RE::TESForm* excavationSiteForm(RE::TESDataHandler::GetSingleton()->LookupForm(excavationSiteFormID, espName));
	if (excavationSiteForm)
	{
		DBG_MESSAGE("Record Fossil Mining Excavation Site %s(0x%08x) as oreVein:volcanicDigSite", excavationSiteForm->GetName(), excavationSiteForm->GetFormID());
		SetObjectTypeForForm(excavationSiteForm->GetFormID(), ObjectType::oreVein);
		m_resourceTypeByOreVein.insert(std::make_pair(excavationSiteForm->As<RE::TESObjectACTI>(), ResourceType::volcanicDigSite));
	}
}

void DataCase::BlockOffLimitsContainers()
{
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	// on first pass, detect off limits containers to avoid rescan on game reload
	if (dhnd && m_offLimitsContainers.empty())
	{
		DBG_MESSAGE("Pre-emptively block all off-limits containers");
		for (RE::TESForm* form : dhnd->GetFormArray(RE::FormType::Faction))
		{
			if (!form)
				continue;

			RE::TESFaction* faction = form->As<RE::TESFaction>();
			if (!faction)
				continue;

			RE::TESObjectREFR* containerRef = nullptr;
			if (faction->data.kVendor)
			{
				containerRef = faction->vendorData.merchantContainer;
				if (containerRef)
				{
					DBG_VMESSAGE("Blocked vendor container : %s(%08x)", containerRef->GetName(), containerRef->formID);
					m_offLimitsContainers.insert(containerRef);
				}
			}

			containerRef = faction->crimeData.factionStolenContainer;
			if (containerRef)
			{
				DBG_VMESSAGE("Blocked stolenGoodsContainer : %s(%08x)", containerRef->GetName(), containerRef->formID);
				m_offLimitsContainers.insert(containerRef);
			}

			containerRef = faction->crimeData.factionPlayerInventoryContainer;
			if (containerRef)
			{
				DBG_VMESSAGE("Blocked playerInventoryContainer : %s(%08x)", containerRef->GetName(), containerRef->formID);
				m_offLimitsContainers.insert(containerRef);
			}
		}
		ExcludeImmersiveArmorsGodChest();
		IncludeFossilMiningExcavation();
	}
	// block all the known off-limits containers - list is invariant during gaming session
	for (const auto refr : m_offLimitsContainers)
	{
		BlockReference(refr);
	}
	for (const auto form : m_offLimitsForms)
	{
		BlockForm(form);
	}
}

void DataCase::GetAmmoData()
{
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	if (!dhnd)
		return;

	DBG_MESSAGE("Loading AmmoData");
	for (RE::TESForm* form : dhnd->GetFormArray(RE::FormType::Ammo))
	{
		RE::TESAmmo* ammo(form->As<RE::TESAmmo>());
		if (!ammo)
			continue;

		DBG_VMESSAGE("Checking %s", ammo->GetFullName());
		if (!ammo->GetPlayable())
		{
			DBG_VMESSAGE("Not playable");
			continue;
		}

		std::string name;
		name = PluginUtils::GetBaseName(ammo);
		if (name.empty())
		{
			DBG_VMESSAGE("base name empty");
			continue;
     	}
		DBG_VMESSAGE("base name %s", name.c_str());

		RE::BGSProjectile* proj = ammo->data.projectile;
		if (!proj)
			continue;

		DBG_VMESSAGE("Adding Projectile %s with ammo %s", proj->GetFullName(), ammo->GetFullName());
		ammoList[proj] = ammo;
	}

	REL_MESSAGE("* AmmoData(%d)", ammoList.size());
}

bool DataCase::BlockReference(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return false;
	// dynamic forms must never be recorded as their FormID may be reused
	if (refr->IsDynamicForm())
		return false;
	RecursiveLockGuard guard(m_blockListLock);
	return (blockRefr.insert(refr->GetFormID())).second;
}

bool DataCase::IsReferenceBlocked(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return false;
	// dynamic forms must never be recorded as their FormID may be reused
	if (refr->IsDynamicForm())
		return false;
	RecursiveLockGuard guard(m_blockListLock);
	return blockRefr.count(refr->GetFormID()) > 0;
}

void DataCase::ClearBlockedReferences(const bool gameReload)
{
	DBG_MESSAGE("Reset list of blocked REFRs");
	RecursiveLockGuard guard(m_blockListLock);
	// Volcanic dig sites from Fossil Mining are only cleared on game reload, to simulate the 30 day delay in
	// the mining script. Only allow one auto-mining visit per gaming session, unless player dies.
	decltype(blockRefr) volcanicDigSites;
	for (const auto refrID : blockRefr)
	{
		RE::TESForm* form(RE::TESForm::LookupByID(refrID));
		if (!form)
			continue;
		RE::TESObjectREFR* refr(form->As<RE::TESObjectREFR>());
		if (!refr)
			continue;
		if (GetBaseFormObjectType(refr->GetBaseObject()) == ObjectType::oreVein &&
			OreVeinResourceType(refr->GetBaseObject()->As<RE::TESObjectACTI>()) == ResourceType::volcanicDigSite)
		{
			volcanicDigSites.insert(refrID);
		}
	}
	blockRefr.swap(volcanicDigSites);
}

bool DataCase::BlacklistReference(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return false;
	// dynamic forms must never be recorded as their FormID may be reused
	if (refr->IsDynamicForm())
		return false;
	RecursiveLockGuard guard(m_blockListLock);
	return (blacklistRefr.insert(refr->GetFormID())).second;
}

bool DataCase::IsReferenceOnBlacklist(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return false;
	// dynamic forms must never be recorded as their FormID may be reused
	if (refr->IsDynamicForm())
		return false;
	RecursiveLockGuard guard(m_blockListLock);
	return blacklistRefr.count(refr->GetFormID()) > 0;
}

void DataCase::ClearReferenceBlacklist()
{
	DBG_MESSAGE("Reset blacklisted REFRs");
	RecursiveLockGuard guard(m_blockListLock);
	blacklistRefr.clear();
}

// Remember locked containers so we do not indiscriminately auto-loot them after a player unlock, if config forbids
bool DataCase::IsReferenceLockedContainer(const RE::TESObjectREFR* refr)
{
	if (!refr)
		return false;
	RecursiveLockGuard guard(m_blockListLock);
	auto lockedMatch(m_lockedContainers.find(refr->GetFormID()));
	if (!IsContainerLocked(refr))
	{
		if (lockedMatch != m_lockedContainers.end())
		{
			// If container is not locked, but previously was stored as locked, see if it remains unlocked for long enough
			// to safely erase our record of it
			// We take this approach in case unlock has script lag and we auto-loot before manually seeing the container
			// For locked container, we want the player to have the enjoyment of manually looting after unlocking. If 
			// they don't want this, just configure 'Loot locked container'.
			const auto recordedTime(lockedMatch->second);
			if (std::chrono::high_resolution_clock::now() - recordedTime > std::chrono::milliseconds(SearchTask::ObjectGlowDurationSpecialSeconds * 1000))
			{
				DBG_VMESSAGE("Forget previously-locked container %s/0x%08x", refr->GetName(), refr->GetFormID());
				m_lockedContainers.erase(lockedMatch);
				return false;
			}
			return true;
		}
		return false;
	}
	else
	{
		// container is locked - save current time, update if already stored
		if (lockedMatch == m_lockedContainers.end())
		{
			m_lockedContainers.insert(std::make_pair(refr->GetFormID(), std::chrono::high_resolution_clock::now()));
			DBG_VMESSAGE("Remember locked container %s/0x%08x", refr->GetName(), refr->GetFormID());
		}
		else
		{
			lockedMatch->second = std::chrono::high_resolution_clock::now();
		}
		return true;
	}
}

void DataCase::ForgetLockedContainers()
{
	DBG_MESSAGE("Clear locked containers from last cell");
	RecursiveLockGuard guard(m_blockListLock);
	m_lockedContainers.clear();
}

void DataCase::UpdateLockedContainers()
{
	RecursiveLockGuard guard(m_blockListLock);
	DBG_MESSAGE("Update last checked time on %d locked containers", m_lockedContainers.size());
	auto currentTime(std::chrono::high_resolution_clock::now());
	for (auto& lockedContainer : m_lockedContainers)
	{
		lockedContainer.second = currentTime;
	}
}

bool DataCase::BlockForm(const RE::TESForm* form)
{
	if (!form)
		return false;
	// dynamic forms must never be recorded as their FormID may be reused
	if (form->IsDynamicForm())
		return false;
	RecursiveLockGuard guard(m_blockListLock);
	return (blockForm.insert(form)).second;
}

bool DataCase::UnblockForm(const RE::TESForm* form)
{
	if (!form)
		return false;
	// dynamic forms must never be recorded as their FormID may be reused
	if (form->IsDynamicForm())
		return false;
	RecursiveLockGuard guard(m_blockListLock);
	return blockForm.erase(form) > 0;
}

bool DataCase::IsFormBlocked(const RE::TESForm* form)
{
	if (!form)
		return false;
	// dynamic forms must never be recorded as their FormID may be reused
	if (form->IsDynamicForm())
		return false;
	RecursiveLockGuard guard(m_blockListLock);
	return blockForm.count(form) > 0;
}

void DataCase::ResetBlockedForms()
{
	// reset blocked forms to just the user's list
	DBG_MESSAGE("Reset Blocked Forms");
	RecursiveLockGuard guard(m_blockListLock);
	blockForm.clear();
	for (RE::FormID formID : userBlockedForm)
	{
		DBG_VMESSAGE("Restore block status for user form 0x%08x", formID);
		BlockForm(RE::TESForm::LookupByID(formID));
	}
}

ObjectType DataCase::GetFormObjectType(RE::FormID formID) const
{
	const auto entry(m_objectTypeByForm.find(formID));
	if (entry != m_objectTypeByForm.cend())
		return entry->second;
	return ObjectType::unknown;
}

bool DataCase::SetObjectTypeForForm(RE::FormID formID, ObjectType objectType)
{
	return m_objectTypeByForm.insert(std::make_pair(formID, objectType)).second;
}

ObjectType DataCase::GetObjectTypeForFormType(RE::FormType formType) const
{
	const auto entry(m_objectTypeByFormType.find(formType));
	if (entry != m_objectTypeByFormType.cend())
		return entry->second;
	return ObjectType::unknown;
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
	const auto& translation(translations.find(key));
	if (translation == translations.cend())
		return nullptr;
	return translation->second.c_str();
}

const RE::TESAmmo* DataCase::ProjToAmmo(const RE::BGSProjectile* proj)
{
	return (proj && ammoList.find(proj) != ammoList.end()) ? ammoList[proj] : nullptr;
}

const RE::TESForm* DataCase::ConvertIfLeveledItem(const RE::TESForm* form) const
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
	arrowCheck.clear();

	// only clear blacklist on game reload
	if (gameReload)
	{
		ClearReferenceBlacklist();
	}
	// reset blocked Base Objects and REFRs, reseed with off-limits containers
	ResetBlockedForms();
	ClearBlockedReferences(gameReload);
	BlockOffLimitsContainers();
	ForgetLockedContainers();
}

bool DataCase::SkipAmmoLooting(RE::TESObjectREFR* refr)
{
	bool skip(false);
	RE::NiPoint3 pos = refr->GetPosition();
	if (pos == RE::NiPoint3())
	{
		DBG_VMESSAGE("err %0.2f,%0.2f,%0.2f", pos.x, pos.y, pos.z);
		BlockReference(refr);
		skip = true;
	}

	RecursiveLockGuard guard(m_blockListLock);
	if (arrowCheck.count(refr) == 0)
	{
		DBG_VMESSAGE("pick %0.2f,%0.2f,%0.2f", pos.x, pos.y, pos.z);
		arrowCheck.insert(std::make_pair(refr, pos));
		skip = true;
	}
	else
	{
		RE::NiPoint3 prev = arrowCheck.at(refr);
		if (prev != pos)
		{
			DBG_VMESSAGE("moving pos:%0.2f,%0.2f,%0.2f prev:%0.2f %0.2f, %0.2f", pos.x, pos.y, pos.z, prev.x, prev.y, prev.z);
			arrowCheck[refr] = pos;
			skip = true;
		}
		else
		{
			DBG_VMESSAGE("catch %0.2f,%0.2f,%0.2f", pos.x, pos.y, pos.z);
			arrowCheck.erase(refr);
		}
	}
	return skip;
}

void DataCase::CategorizeLootables()
{
	REL_MESSAGE("*** LOAD *** Load User blocked forms");
	if (!GetTSV(&userBlockedForm, "BlackList.tsv"))
		GetTSV(&userBlockedForm, "default\\BlackList.tsv");

	// used to taxonomize ACTIvators
	REL_MESSAGE("*** LOAD *** Load Text Translation");
	GetTranslationData();

	REL_MESSAGE("*** LOAD *** Store Activation Verbs");
	StoreActivationVerbs();

	REL_MESSAGE("*** LOAD *** Get Ammo Data");
	GetAmmoData();

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

	// Classes inheriting from TESProduceForm may have an ingredient, categorized as the appropriate consumable
	// This 'ingredient' can be MISC (e.g. Coin Replacer Redux Coin Purses) so those must be done first, as above by keyword
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
		DBG_VMESSAGE("Activation verb %s unhandled at present", unhandledVerb.c_str());
	}
#endif

	// Finally, Collections are layered on top of categorized objects
	REL_MESSAGE("*** LOAD *** Build Collections");
	CollectionManager::Instance().ProcessDefinitions();

	// Analyze perks that affect looting
	DBG_MESSAGE("*** LOAD *** Analyze Perks");
	AnalyzePerks();
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
		{"VendorItemAnimalPart", ObjectType::clutter},
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
		{"VendorItemFateCards", ObjectType::clutter}
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

	for (RE::TESForm* form : dhnd->GetFormArray(RE::BGSKeyword::FORMTYPE))
	{
		RE::BGSKeyword* keywordDef(form->As<RE::BGSKeyword>());
		if (!keywordDef)
		{
			DBG_WARNING("Skipping non-keyword form 0x%08x", form->formID);
			continue;
		}

		std::string keywordName(keywordDef->GetFormEditorID());
		// Store player house keyword for SearchTask usage
		if (keywordName == "LocTypePlayerHouse")
		{
			SearchTask::SetPlayerHouseKeyword(keywordDef);
			continue;
		}
		if (glowableBooks.find(keywordName) != glowableBooks.cend())
		{
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
			DBG_VMESSAGE("0x%08x (%s) matched substring, treated as %s", form->formID, keywordName.c_str(), GetObjectTypeName(objectType).c_str());
		}
		else if (keywordName.starts_with("VendorItem"))
		{
			DBG_VMESSAGE("0x%08x (%s) treated as clutter", form->formID, keywordName.c_str());
			objectType = ObjectType::clutter;
		}
		else
		{
			DBG_VMESSAGE("0x%08x (%s) skipped", form->formID, keywordName.c_str());
			continue;
		}
		m_objectTypeByForm[form->formID] = DecorateIfEnchanted(form, objectType);
		DBG_VMESSAGE("0x%08x (%s) stored as %s", form->formID, keywordName.c_str(), GetObjectTypeName(objectType).c_str());
	}
}

template <> ObjectType DataCase::DefaultObjectType<RE::TESObjectARMO>()
{
	return ObjectType::armor;
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
	ObjectType objectType(ObjectType::unknown);
	if (consumable->IsFood())
		objectType = (consumable->data.consumptionSound && consumable->data.consumptionSound->formID == drinkSound) ? ObjectType::drink : ObjectType::food;
	else
		objectType = (consumable->IsPoison()) ? ObjectType::poison : ObjectType::potion;
	return objectType;
}

template <> ObjectType DataCase::ConsumableObjectType<RE::IngredientItem>(RE::IngredientItem* consumable)
{
	return ObjectType::ingredient;
}


// ingredient nullptr indicates this Producer is pending resolution
bool DataCase::SetLootableForProducer(RE::TESForm* producer, RE::TESForm* lootable)
{
	RecursiveLockGuard guard(m_producerIngredientLock);
	if (!lootable)
	{
		DBG_VMESSAGE("Producer %s/0x%08x needs resolving to lootable", producer->GetName(), producer->formID);
		// return value signals entry pending resolution found/not found
		return m_producerLootable.insert(std::make_pair(producer, nullptr)).second;
	}
	else
	{
		DBG_VMESSAGE("Producer %s/0x%08x has lootable %s/0x%08x", producer->GetName(), producer->formID, lootable->GetName(), lootable->formID);
		m_producerLootable[producer] = lootable;
		return true;
	}
}

RE::TESForm* DataCase::GetLootableForProducer(RE::TESForm* producer) const
{
	RecursiveLockGuard guard(m_producerIngredientLock);
	const auto matched(m_producerLootable.find(producer));
	if (matched != m_producerLootable.cend())
		return matched->second;
	return nullptr;
}

bool DataCase::PerksAddLeveledItemsOnDeath(const RE::Actor* actor) const
{
	const auto deathPerk = std::find_if(m_leveledItemOnDeathPerks.cbegin(), m_leveledItemOnDeathPerks.cend(),
		[=] (const RE::BGSPerk* perk) -> bool { return actor->HasPerk(const_cast<RE::BGSPerk*>(perk)); });
	if (deathPerk != m_leveledItemOnDeathPerks.cend())
	{
		DBG_VMESSAGE("Leveled item added at death for perk %s/0x%08x", (*deathPerk)->GetName(), (*deathPerk)->GetFormID());
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
	m_objectTypeByFormType[RE::FormType::Container] = ObjectType::container;
	m_objectTypeByFormType[RE::FormType::Ingredient] = ObjectType::ingredient;
	m_objectTypeByFormType[RE::FormType::SoulGem] = ObjectType::soulgem;
	m_objectTypeByFormType[RE::FormType::KeyMaster] = ObjectType::key;
	m_objectTypeByFormType[RE::FormType::Scroll] = ObjectType::scroll;
	m_objectTypeByFormType[RE::FormType::Ammo] = ObjectType::ammo;
	m_objectTypeByFormType[RE::FormType::Light] = ObjectType::light;

	// Map well-known forms to ObjectType
	m_objectTypeByForm[LockPick] = ObjectType::lockpick;
	m_objectTypeByForm[Gold] = ObjectType::septims;
}

template <>
ObjectType DataCase::DefaultIngredientObjectType(const RE::TESFlora* form)
{
	return ObjectType::flora;
}

template <>
ObjectType DataCase::DefaultIngredientObjectType(const RE::TESObjectTREE* form)
{
	return ObjectType::food;
}

void DataCase::LeveledItemCategorizer::CategorizeContents()
{
	ProcessContentsAtLevel(m_rootItem);
}

DataCase::LeveledItemCategorizer::LeveledItemCategorizer(const RE::TESLevItem* rootItem, const std::string& targetName) : 
	m_rootItem(rootItem), m_targetName(targetName)
{
}

void DataCase::LeveledItemCategorizer::ProcessContentsAtLevel(const RE::TESLevItem* leveledItem)
{
	for (const RE::LEVELED_OBJECT& leveledObject : leveledItem->entries)
	{
		RE::TESForm* itemForm(leveledObject.form);
		if (!itemForm)
			continue;
		// Handle nesting of leveled items
		RE::TESLevItem* leveledItem(itemForm->As<RE::TESLevItem>());
		if (leveledItem)
		{
			ProcessContentsAtLevel(leveledItem);
			continue;
		}
		ObjectType itemType(DataCase::GetInstance()->GetObjectTypeForForm(itemForm));
		if (itemType != ObjectType::unknown)
		{
			ProcessContentLeaf(itemForm, itemType);
		}
	}
}

DataCase::ProduceFormCategorizer::ProduceFormCategorizer(
	RE::TESProduceForm* produceForm, const RE::TESLevItem* rootItem, const std::string& targetName) :
	LeveledItemCategorizer(rootItem, targetName), m_produceForm(produceForm), m_contents(nullptr)
{
}

void DataCase::ProduceFormCategorizer::ProcessContentLeaf(RE::TESForm* itemForm, ObjectType itemType)
{
	if (!m_contents)
	{
		DBG_VMESSAGE("Target %s/0x%08x has contents type %s in form %s/0x%08x", m_targetName.c_str(), m_rootItem->formID,
			GetObjectTypeName(itemType).c_str(), itemForm->GetName(), itemForm->formID);
		if (!DataCase::GetInstance()->m_produceFormContents.insert(std::make_pair(m_produceForm, itemForm)).second)
		{
			DBG_VMESSAGE("Leveled Item %s/0x%08x contents already present", m_targetName.c_str(), m_rootItem->formID);
		}
		else
		{
			DBG_VMESSAGE("Leveled Item %s/0x%08x has contents %s/0x%08x",
				m_targetName.c_str(), m_rootItem->formID, itemForm->GetName(), itemForm->formID);
			if (!DataCase::GetInstance()->m_objectTypeByForm.insert(std::make_pair(itemForm->formID, itemType)).second)
			{
				DBG_VMESSAGE("Leveled Item %s/0x%08x contents %s/0x%08x already has an ObjectType",
					m_targetName.c_str(), m_rootItem->formID, itemForm->GetName(), itemForm->formID);
			}
			else
			{
				DBG_VMESSAGE("Leveled Item %s/0x%08x not stored", m_targetName, m_rootItem->formID);
			}
			m_contents = itemForm;
		}
	}
	else if (m_contents == itemForm)
	{
		DBG_VMESSAGE("Target %s/0x%08x contents type %s already recorded", m_targetName.c_str(), m_rootItem->formID,
			GetObjectTypeName(itemType).c_str());
	}
	else
	{
		REL_WARNING("Target %s/0x%08x contents type %s already stored under different form %s/0x%08x", m_targetName.c_str(), m_rootItem->formID,
			GetObjectTypeName(itemType).c_str(), m_contents->GetName(), m_contents->formID);
	}
}

