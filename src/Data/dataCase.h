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
#pragma once

#include <mutex>
#include <chrono>

#include "Looting/ProducerLootables.h"
#include "Looting/objects.h"

namespace shse
{

class LeveledItemCategorizer
{
public:
	LeveledItemCategorizer(const RE::TESLevItem* rootItem);
	virtual ~LeveledItemCategorizer();
	void CategorizeContents();

private:
	void ProcessContentsAtLevel(const RE::TESLevItem* leveledItem);

protected:
	virtual void ProcessContentLeaf(RE::TESBoundObject* itemForm, ObjectType itemType) = 0;

	const RE::TESLevItem* m_rootItem;
	// prevent infinite recursion
	std::unordered_set<const RE::TESLevItem*> m_lvliSeen;
};

class DataCase
{
public:
	static DataCase* GetInstance(void)
	{
		if (s_pInstance == nullptr)
			s_pInstance = new DataCase();
		return s_pInstance;
	}

	typedef std::unordered_set<const RE::TESForm*> FormCategory;

	void BlockFirehoseSource(const RE::TESObjectREFR* refr);
	void ForgetFirehoseSources();
	bool IsFirehose(const RE::TESForm* form) const;
	void AddFirehose(const RE::TESForm* form);

	void BlockReference(const RE::TESObjectREFR* refr, const Lootability reason);
	void BlockReferenceByID(const RE::FormID refrID, const Lootability reason);
	Lootability IsReferenceBlocked(const RE::TESObjectREFR* refr) const;
	void ResetBlockedReferences(const bool gameReload);

	// permanent REFR blacklist, reset on game reload
	bool BlacklistReference(const RE::TESObjectREFR* refr);
	bool IsReferenceOnBlacklist(const RE::TESObjectREFR* refr) const;
	void ClearReferenceBlacklist();

	void RefreshKnownIngredients();
	bool IsIngredientKnown(const RE::TESForm* form) const;

	bool IsEphemeralForm(const RE::TESForm* form) const;
	bool BlockForm(const RE::TESForm* form, const Lootability reason);
	Lootability IsFormBlocked(const RE::TESForm* form) const;
	void ResetBlockedForms();
	bool BlockFormPermanently(const RE::TESForm* form, const Lootability reason);

	bool ReferencesBlacklistedContainer(const RE::TESObjectREFR* refr) const;

	ObjectType GetFormObjectType(const RE::FormID formID) const;
	bool SetObjectTypeForFormID(const RE::FormID formID, const ObjectType objectType);
	bool SetObjectTypeForForm(const RE::TESForm* form, const ObjectType objectType);
	void ForceObjectTypeForForm(const RE::TESForm* form, const ObjectType objectType);
	ObjectType GetObjectTypeForFormType(const RE::FormType formType) const;
	bool SetObjectTypeForFormType(const RE::FormType formType, const ObjectType objectType);
	ObjectType GetObjectTypeForForm(const RE::TESForm* form) const;

	ResourceType OreVeinResourceType(const RE::TESObjectACTI* mineable) const;

	const char* GetTranslation(const char* key) const;

	const RE::TESBoundObject* ConvertIfLeveledItem(const RE::TESBoundObject* form) const;

	void CategorizeLootables(void);
	void ListsClear(const bool gameReload);
	bool SkipAmmoLooting(RE::TESObjectREFR* refr);

	inline bool IsBookGlowableKeyword(RE::BGSKeyword* keyword) const
	{
		return keyword && m_glowableBookKeywords.find(keyword->GetFormID()) != m_glowableBookKeywords.cend();
	}

	bool PerksAddLeveledItemsOnDeath(const RE::Actor* actor) const;
	float PerkIngredientMultiplier(const RE::Actor* actor) const;

	inline const std::unordered_map<RE::FormID, std::string>& OffLimitsLocations()
	{
		return m_offLimitsLocations;
	}
	inline bool IsOffLimitsLocation(const RE::TESForm* badPlace) const
	{
		return m_offLimitsLocations.contains(badPlace->GetFormID());
	}
	inline bool IsOffLimitsContainer(const RE::TESObjectREFR* containerRef) const
	{
		return m_offLimitsContainers.contains(containerRef->GetFormID());
	}

	template <typename T>
	T* FindExactMatch(const std::string& defaultESP, const RE::FormID maskedFormID)
	{
		if (defaultESP == "Skyrim.esm")
			return RE::TESForm::LookupByID<T>(maskedFormID);
		T* typedForm(RE::TESDataHandler::GetSingleton()->LookupForm<T>(maskedFormID, defaultESP));
		if (typedForm)
		{
			DBG_MESSAGE("Found exact match 0x{:08x} for {}:0x{:06x}", typedForm->GetFormID(), defaultESP.c_str(), maskedFormID);
		}
		else
		{
			DBG_MESSAGE("No exact match for {}:0x{:06x}", defaultESP.c_str(), maskedFormID);
		}
		return typedForm;
	}

	void RegisterPlayerCreatedALCH(RE::AlchemyItem* consumable);

	// special case statics
	static constexpr RE::FormID LockPick = 0x0A;
	static constexpr RE::FormID Gold = 0x0F;
	static constexpr RE::FormID WispCore = 0x10EB2A;
	static constexpr RE::FormID RollOfPaper = 0x33761;

private:
	std::unordered_map<std::string, std::string> m_translations;

	std::unordered_map<const RE::TESObjectREFR*, RE::NiPoint3> m_arrowCheck;

	std::unordered_map<RE::FormID, std::string> m_offLimitsLocations;
	std::unordered_set<RE::FormID> m_offLimitsContainers;
	std::unordered_set<RE::TESContainer*> m_containerBlackList;
	std::unordered_map<const RE::TESForm*, Lootability> m_permanentBlockedForms;
	std::unordered_map<const RE::TESForm*, Lootability> m_blockForm;
	std::unordered_set<const RE::TESForm*> m_firehoseForms;
	std::unordered_set<RE::FormID> m_firehoseSources;
	std::unordered_map<RE::FormID, Lootability> m_blockRefr;
	std::unordered_set<RE::FormID> m_blacklistRefr;

	std::unordered_map<RE::FormType, ObjectType> m_objectTypeByFormType;
	std::unordered_map<RE::FormID, ObjectType> m_objectTypeByForm;
	mutable RecursiveLock m_formToTypeLock;

	mutable std::unordered_map<RE::FormID, bool> m_ingredientEffectsKnown;
	std::unordered_map<const RE::TESProduceForm*, const RE::TESBoundObject*> m_produceFormContents;
	std::unordered_set<RE::FormID> m_glowableBookKeywords;
	std::unordered_set<const RE::BGSPerk*> m_leveledItemOnDeathPerks;
	// assume simple setters for now, like vanilla Green Thumb
	std::unordered_map<const RE::BGSPerk*, float> m_modifyHarvestedPerkMultipliers;
	RE::BGSKeyword* m_spellTomeKeyword;

	mutable RecursiveLock m_blockListLock;

	void RecordOffLimitsLocations(void);
	void RecordPlayerHouseCells(void);
	void BlockOffLimitsContainers(void);

	bool AllEffectsKnown(const RE::IngredientItem* ingredient) const;

	template <typename T>
	ObjectType DefaultIngredientObjectType(const T* form)
	{
		return ObjectType::unknown;
	}

	template <>	ObjectType DefaultIngredientObjectType(const RE::TESFlora* form);
	template <>	ObjectType DefaultIngredientObjectType(const RE::TESObjectTREE* form);

	class ProduceFormCategorizer : public LeveledItemCategorizer
	{
	public:
		ProduceFormCategorizer(RE::TESProduceForm* produceForm, const RE::TESLevItem* rootItem, const std::string& targetName);
		inline ObjectType ContentsType() const { return m_contentsType; }

	protected:
		virtual void ProcessContentLeaf(RE::TESBoundObject* itemForm, ObjectType itemType) override;

	private:
		const std::string m_targetName;
		RE::TESProduceForm* m_produceForm;
		RE::TESForm* m_contents;
		ObjectType m_contentsType;
	};

	template <typename T>
	void CategorizeByIngredient()
	{
		RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
		if (!dhnd)
			return;

		for (T* target : dhnd->GetFormArray<T>())
		{
			if (!target->GetFullNameLength())
				continue;
			const char * targetName(target->GetFullName());
			RE::TESBoundObject* ingredient(target->produceItem);
			if (!ingredient)
			{
				REL_WARNING("No ingredient for {}/0x{:08x}", targetName, target->GetFormID());
				continue;
			}

			// Categorize ingredient
			ObjectType storedType(ObjectType::unknown);
			const RE::TESLevItem* leveledItem(ingredient->As<RE::TESLevItem>());
			if (leveledItem)
			{
				ProduceFormCategorizer categorizer(target, leveledItem, targetName);
				categorizer.CategorizeContents();
				REL_VMESSAGE("{}/0x{:08x} has ingredient Leveled Item, type {}", targetName, target->GetFormID(),
					GetObjectTypeName(categorizer.ContentsType()));
			}
			else
			{
				// Try the ingredient form on this Produce holder
				storedType = GetObjectTypeForForm(ingredient);
				if (storedType != ObjectType::unknown)
				{
					REL_VMESSAGE("{}/0x{:08x} has ingredient {}/0x{:08x} stored as type {}", targetName, target->GetFormID(),
						ingredient->GetName(), ingredient->GetFormID(), GetObjectTypeName(storedType).c_str());
					ProducerLootables::Instance().SetLootableForProducer(target, ingredient);
				}
				else
				{
					storedType = DefaultIngredientObjectType(target);
					REL_VMESSAGE("{}/0x{:08x} has ingredient {}/0x{:08x} with default type {}", targetName, target->GetFormID(),
						ingredient->GetName(), ingredient->GetFormID(), GetObjectTypeName(storedType).c_str());
				}
				if (storedType != ObjectType::unknown)
				{ 
					// Store mapping of Produce holder to ingredient - this is the most correct type for this item producer
					SetObjectTypeForForm(target, storedType);
				}
				else
				{
					REL_WARNING("Target {}/0x{:08x} not stored", targetName, target->GetFormID());
				}
			}
		}
	}

	void HandleExceptions(void);
	ObjectType DecorateIfEnchanted(const RE::TESForm* form, const ObjectType rawType);
	void SetObjectTypeByKeywords();

	template <typename T>
	ObjectType ConsumableObjectType(T* consumable)
	{
		return ObjectType::unknown;
	}

	template <> ObjectType ConsumableObjectType<RE::AlchemyItem>(RE::AlchemyItem* consumable);
	template <> ObjectType ConsumableObjectType<RE::IngredientItem>(RE::IngredientItem* consumable);

	template <typename T>
	void CategorizeConsumables()
	{
		RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
		if (!dhnd)
			return;
		for (T* consumable : dhnd->GetFormArray<T>())
		{
			if (consumable->GetFullNameLength() == 0)
			{
				DBG_VMESSAGE("Skipping unnamed form 0x{:08x}", consumable->GetFormID());
				continue;
			}

			std::string formName(consumable->GetFullName());
			if (GetFormObjectType(consumable->GetFormID()) != ObjectType::unknown)
			{
				DBG_VMESSAGE("Skipping previously categorized form {}/0x{:08x}", formName.c_str(), consumable->GetFormID());
				continue;
			}

			ObjectType objectType(ConsumableObjectType<T>(consumable));
			SetObjectTypeForForm(consumable, objectType);
		}
	}

	template <typename T>
	ObjectType DefaultObjectType()
	{
		return ObjectType::clutter;
	}
	template <> ObjectType  DefaultObjectType<RE::TESObjectARMO>();
	template <> ObjectType  DefaultObjectType<RE::TESObjectWEAP>();

	template <typename T>
	ObjectType OverrideIfBadChoice(const RE::TESForm*, const ObjectType objectType)
	{
		return objectType;
	}
	template <> ObjectType OverrideIfBadChoice<RE::TESObjectARMO>(const RE::TESForm* form, const ObjectType objectType);
	template <> ObjectType OverrideIfBadChoice<RE::TESObjectWEAP>(const RE::TESForm* form, const ObjectType objectType);

	std::unordered_map<std::string, ObjectType> m_objectTypeByActivationVerb;
	mutable std::unordered_set<std::string> m_unhandledActivationVerbs;
	std::unordered_map<const RE::TESObjectACTI*, ResourceType> m_resourceTypeByOreVein;

	ObjectType GetObjectTypeForActivationText(const RE::BSString& activationText) const;

	inline std::string GetVerbFromActivationText(const RE::BSString& activationText) const
	{
		std::string strActivation;
		const char* nextChar(activationText.c_str());
		size_t index(0);
		while (!isspace(*nextChar) && index < activationText.size())
		{
			strActivation.push_back(*nextChar);
			++nextChar;
			++index;
		}
		return strActivation;
	}

	template <typename T>
	void CategorizeByKeyword()
	{
		RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
		if (!dhnd)
			return;

		// use keywords from preference
		for (T* typedForm : dhnd->GetFormArray<T>())
		{
			if (!typedForm->GetFullNameLength())
			{
				DBG_VMESSAGE("0x{:08x} is unnamed", typedForm->GetFormID());
				continue;
			}
			const char* formName(typedForm->GetFullName());
			if ((typedForm->formFlags & T::RecordFlags::kNonPlayable) == T::RecordFlags::kNonPlayable)
			{
				DBG_VMESSAGE("{}/0x{:08x} is NonPlayable", formName, typedForm->GetFormID());
				continue;
			}
			RE::BGSKeywordForm* keywordForm(typedForm->As<RE::BGSKeywordForm>());
			if (!keywordForm)
			{
				DBG_WARNING("{}/0x{:08x} Not a Keyword", formName, typedForm->GetFormID());
				continue;
			}

			ObjectType correctType(ObjectType::unknown);
			for (uint32_t index = 0; index < keywordForm->GetNumKeywords(); ++index)
			{
				std::optional<RE::BGSKeyword*> keyword(keywordForm->GetKeywordAt(index));
				if (!keyword)
					continue;
				// lock not needed during initialization
				const auto matched(m_objectTypeByForm.find(keyword.value()->GetFormID()));
				if (matched != m_objectTypeByForm.cend())
				{
					// if default type, postpone storage in case there is a more specific match
					if (matched->second == DefaultObjectType<T>())
					{
						continue;
					}
					// Immersive Armors gives some armors the VendorItemSpellTome keyword. No, they are not spellBook
					if (keyword == m_spellTomeKeyword)
					{
						// spellBook is not a legal mapping for MISC, ARMO or WEAP. Revisit if we add more instantiations.
						REL_WARNING("Spell tome Keyword not valid for {}/0x{:08x}",formName, typedForm->GetFormID());
					}
					else if (correctType != ObjectType::unknown)
					{
						REL_WARNING("{}/0x{:08x} mapped to {} already stored with keyword {}, check data", formName, typedForm->GetFormID(),
							GetObjectTypeName(matched->second).c_str(), GetObjectTypeName(correctType).c_str());
					}
					else
					{
						correctType = matched->second;
					}
				}
			}
			if (correctType == ObjectType::unknown)
			{
				correctType = DefaultObjectType<T>();
			}
			correctType = OverrideIfBadChoice<T>(typedForm, correctType);
			if (correctType != ObjectType::unknown)
			{
				SetObjectTypeForForm(typedForm, correctType);
				continue;
			}

			// fail-safe is to check if the form has value and store as clutter if so
			// Also, check model path for - you guessed it - clutter. Some base game MISC objects lack keywords.
			if (typedForm->value > 0 || CheckObjectModelPath(typedForm, "clutter"))
			{
				SetObjectTypeForForm(typedForm, ObjectType::clutter);
				continue;
			}
			DBG_VMESSAGE("{}/0x{:08x} not mappable", formName, typedForm->GetFormID());
		}
	}

	void GetTranslationData(void);
	void ActivationVerbsByType(const char* activationVerbKey, const ObjectType objectType);
	void StoreActivationVerbs(void);
	void CategorizeByActivationVerb(void);
	void FindCraftingItems(void);
	void AnalyzePerks(void);

	std::string GetModelPath(const RE::TESForm* thisForm) const;
	bool CheckObjectModelPath(const RE::TESForm* thisForm, const char* arg) const;

	static DataCase* s_pInstance;

	void CategorizeStatics();
	void SetPermanentBlockedItems();
	void ExcludeFactionContainers();
	void ExcludeVendorContainers();
	void ExcludeImmersiveArmorsGodChest();
	void ExcludeGrayCowlStonesChest();
	void ExcludeMissivesBoards();
	void ExcludeBuildYourNobleHouseIncomeChest();

	template <typename T>
	T* FindBestMatch(const std::string& defaultESP, const RE::FormID maskedFormID, const std::string& name)
	{
		T* match(FindExactMatch<T>(defaultESP, maskedFormID));
		// supplied EDID and Name not checked if we match plugin/formID
		if (match)
		{
			DBG_MESSAGE("Returning exact match 0x{:08x}/{} for {}:0x{:06x}", match->GetFormID(), match->GetName(),
				defaultESP.c_str(), maskedFormID);
			return match;
		}

		// look for merged form
		RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
		if (!dhnd)
			return nullptr;

		// Check for match on name. FormID can change if this is in a merge output. Cannot use EDID as it is not loaded.
		for (T* container : dhnd->GetFormArray<T>())
		{
			if (container->GetName() == name)
			{
				if (match)
				{
					REL_MESSAGE("Ambiguity in best match 0x{:08x} vs for 0x{:08x} for {}:0x{:06x}/{}",
						match->GetFormID(), container->GetFormID(), defaultESP.c_str(), maskedFormID, name);
					return nullptr;
				}
				else
				{
					REL_MESSAGE("Found best match 0x{:08x} for {}:0x{:06x}", container->GetFormID(),
						defaultESP.c_str(), maskedFormID, name);
					match = container;
				}
			}
		}
		return match;
	}

	template <typename T>
	std::unordered_set<T*> FindExactMatchesByName(const std::string& name)
	{
		std::unordered_set<T*> matches;
		RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
		if (!dhnd)
			return matches;

		// Check for match on name. FormID can change if this is in a merge output.
		std::for_each(dhnd->GetFormArray<T>().cbegin(), dhnd->GetFormArray<T>().cend(), [&](T* entry) {
			if (entry->GetName() == name)
			{
				matches.insert(entry);
			}
		});
		return matches;
	}

	void IncludeFossilMiningExcavation();
	void IncludeCorpseCoinage();
	void IncludeHearthfireExtendedApiary();
	void IncludePileOfGold();
	void IncludeBSBruma();
	void IncludeToolsOfKagrenac();

	DataCase(void);
};

}
