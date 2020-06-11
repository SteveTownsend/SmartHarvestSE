#pragma once

#include <mutex>
#include <chrono>

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

	bool BlockReference(const RE::TESObjectREFR* refr);
	bool IsReferenceBlocked(const RE::TESObjectREFR* refr);
	void ClearBlockedReferences(const bool gameReload);

	// permanent REFR blacklist, reset on game reload
	bool BlacklistReference(const RE::TESObjectREFR* refr);
	bool IsReferenceOnBlacklist(const RE::TESObjectREFR* refr);
	void ClearReferenceBlacklist();

	bool IsReferenceLockedContainer(const RE::TESObjectREFR* refr);
	void ForgetLockedContainers();
	void UpdateLockedContainers();

	bool BlockForm(const RE::TESForm* form);
	bool UnblockForm(const RE::TESForm* form);
	bool IsFormBlocked(const RE::TESForm* form);
	void ResetBlockedForms();

	ObjectType GetFormObjectType(RE::FormID formID) const;
	bool SetObjectTypeForForm(RE::FormID formID, ObjectType objectType);
	ObjectType GetObjectTypeForFormType(RE::FormType formType) const;

	template <typename T>
	ObjectType GetObjectTypeForForm(T* form) const
	{
		ObjectType objectType(GetObjectTypeForFormType(form->formType));
		if (objectType == ObjectType::unknown)
		{
			objectType = GetFormObjectType(form->formID);
		}
		return objectType;
	}

	ResourceType DataCase::OreVeinResourceType(const RE::TESObjectACTI* mineable) const;

	const char* GetTranslation(const char* key) const;

	const RE::TESAmmo* ProjToAmmo(const RE::BGSProjectile* proj);
	const RE::TESForm* ConvertIfLeveledItem(const RE::TESForm* form) const;

	void CategorizeLootables(void);
	void ListsClear(const bool gameReload);
	bool SkipAmmoLooting(RE::TESObjectREFR* refr);

	bool SetLootableForProducer(RE::TESForm* critter, RE::TESForm* ingredient);
	RE::TESForm* GetLootableForProducer(RE::TESForm* producer) const;

	inline bool IsBookGlowableKeyword(RE::BGSKeyword* keyword) const
	{
		return keyword && m_glowableBookKeywords.find(keyword->GetFormID()) != m_glowableBookKeywords.cend();
	}

	bool PerksAddLeveledItemsOnDeath(const RE::Actor* actor) const;

private:
	std::unordered_map<std::string, std::string> translations;

	std::unordered_map<const RE::TESObjectREFR*, RE::NiPoint3> arrowCheck;
	std::unordered_map<const RE::BGSProjectile*, RE::TESAmmo*> ammoList;

	std::unordered_set<RE::TESObjectREFR*> m_offLimitsContainers;
	std::unordered_set<RE::TESForm*> m_offLimitsForms;
	std::unordered_set<RE::FormID> userBlockedForm;
	std::unordered_set<const RE::TESForm*> blockForm;
	std::unordered_set<RE::FormID> blockRefr;
	std::unordered_set<RE::FormID> blacklistRefr;
	std::unordered_map<RE::FormID, std::chrono::time_point<std::chrono::high_resolution_clock>> m_lockedContainers;

	std::unordered_map<RE::FormType, ObjectType> m_objectTypeByFormType;
	std::unordered_map<RE::FormID, ObjectType> m_objectTypeByForm;
	std::unordered_map<const RE::TESProduceForm*, const RE::TESForm*> m_produceFormContents;
	std::unordered_set<RE::FormID> m_glowableBookKeywords;
	std::unordered_set<const RE::BGSPerk*> m_leveledItemOnDeathPerks;

	mutable RecursiveLock m_producerIngredientLock;
	mutable RecursiveLock m_blockListLock;
	std::unordered_map<RE::TESForm*, RE::TESForm*> m_producerLootable;

	bool GetTSV(std::unordered_set<RE::FormID> *tsv, const char* fileName);

	void BlockOffLimitsContainers(void);
	void GetAmmoData(void);

	template <typename T>
	ObjectType DefaultIngredientObjectType(const T* form)
	{
		return ObjectType::unknown;
	}

	template <>	ObjectType DefaultIngredientObjectType(const RE::TESFlora* form);
	template <>	ObjectType DefaultIngredientObjectType(const RE::TESObjectTREE* form);

	class LeveledItemCategorizer
	{
	public:
		LeveledItemCategorizer(const RE::TESLevItem* rootItem, const std::string& targetName);
		void CategorizeContents();

	private:
		void ProcessContentsAtLevel(const RE::TESLevItem* leveledItem);

	protected:
		virtual void ProcessContentLeaf(RE::TESForm* itemForm, ObjectType itemType) = 0;

		const RE::TESLevItem* m_rootItem;
		const std::string m_targetName;
	};

	class ProduceFormCategorizer : public LeveledItemCategorizer
	{
	public:
		ProduceFormCategorizer(RE::TESProduceForm* produceForm, const RE::TESLevItem* rootItem, const std::string& targetName);

	protected:
		virtual void ProcessContentLeaf(RE::TESForm* itemForm, ObjectType itemType) override;

	private:
		RE::TESProduceForm* m_produceForm;
		RE::TESForm* m_contents;
	};

	template <typename T>
	void CategorizeByIngredient()
	{
		RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
		if (!dhnd)
			return;

		for (RE::TESForm* form : dhnd->GetFormArray(T::FORMTYPE))
		{
			T* target(form->As<T>());
			if (!target || !target->GetFullNameLength())
				continue;
			const char * targetName(target->GetFullName());
			DBG_VMESSAGE("Checking target %s/0x%08x", targetName, form->formID);

			const RE::TESBoundObject* ingredient(target->produceItem);
			if (!ingredient)
			{
				REL_WARNING("No ingredient for %s/0x%08x", targetName, form->formID);
				continue;
			}

			// Categorize ingredient
			ObjectType storedType(ObjectType::unknown);
			const RE::TESLevItem* leveledItem(ingredient->As<RE::TESLevItem>());
			if (leveledItem)
			{
				DBG_VMESSAGE("%s/0x%08x ingredient is Leveled Item", targetName, form->formID);
				ProduceFormCategorizer(target, leveledItem, targetName).CategorizeContents();
			}
			else
			{
				// Try the ingredient form on this Produce holder
				storedType = GetObjectTypeForForm(ingredient);
				if (storedType != ObjectType::unknown)
				{
					DBG_VMESSAGE("Target %s/0x%08x has ingredient %s/0x%08x stored as type %s", targetName, form->formID,
						ingredient->GetName(), ingredient->formID, GetObjectTypeName(storedType).c_str());
					SetLootableForProducer(form, const_cast<RE::TESBoundObject*>(ingredient));
				}
				else
				{
					storedType = DefaultIngredientObjectType(target);
				}
				if (storedType != ObjectType::unknown)
				{ 
					// Store mapping of Produce holder to ingredient - this is the most correct type for this item producer
					if (SetObjectTypeForForm(form->formID, storedType))
					{
						DBG_VMESSAGE("Target %s/0x%08x stored as type %s", targetName, form->formID, GetObjectTypeName(storedType).c_str());
					}
					else
					{
						REL_WARNING("Target %s/0x%08x (%s) already stored, check data", targetName, form->formID, GetObjectTypeName(storedType).c_str());
					}
				}
				else
				{
					DBG_VMESSAGE("Target %s/0x%08x not stored", targetName, form->formID);
				}
			}
		}
	}

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
		for (RE::TESForm* form : dhnd->GetFormArray(T::FORMTYPE))
		{
			T* consumable(form->As<T>());
			if (!consumable)
			{
				DBG_VMESSAGE("Skipping non-consumable form 0x%08x", form->formID);
				continue;
			}

			RE::TESFullName* pFullName = form->As<RE::TESFullName>();
			if (!pFullName || pFullName->GetFullNameLength() == 0)
			{
				DBG_VMESSAGE("Skipping unnamed form 0x%08x", form->formID);
				continue;
			}

			std::string formName(pFullName->GetFullName());
			if (GetFormObjectType(form->formID) != ObjectType::unknown)
			{
				DBG_VMESSAGE("Skipping previously categorized form %s/0x%08x", formName.c_str(), form->formID);
				continue;
			}

			ObjectType objectType(ConsumableObjectType<T>(consumable));
			DBG_MESSAGE("Consumable %s/0x%08x has type %s", formName.c_str(), form->formID, GetObjectTypeName(objectType).c_str());
			m_objectTypeByForm[form->formID] = objectType;
		}
	}

	template <typename T>
	ObjectType DefaultObjectType()
	{
		return ObjectType::clutter;
	}
	template <> ObjectType  DefaultObjectType<RE::TESObjectARMO>();

	template <typename T>
	ObjectType OverrideIfBadChoice(const RE::TESForm* form, const ObjectType objectType)
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
		for (RE::TESForm* form : dhnd->GetFormArray(T::FORMTYPE))
		{
			T* typedForm(form->As<T>());
			if (!typedForm || !typedForm->GetFullNameLength())
				continue;
			const char* formName(typedForm->GetFullName());
			DBG_VMESSAGE("Categorizing %s/0x%08x", formName, form->formID);
			if ((form->formFlags & T::RecordFlags::kNonPlayable) == T::RecordFlags::kNonPlayable)
			{
				DBG_VMESSAGE("%s/0x%08x is NonPlayable", formName, form->formID);
				continue;
			}
			RE::BGSKeywordForm* keywordForm(form->As<RE::BGSKeywordForm>());
			if (!keywordForm)
			{
				DBG_WARNING("%s/0x%08x Not a Keyword", formName, form->formID);
				continue;
			}

			ObjectType correctType(ObjectType::unknown);
			bool hasDefault(false);
			for (UInt32 index = 0; index < keywordForm->GetNumKeywords(); ++index)
			{
				std::optional<RE::BGSKeyword*> keyword(keywordForm->GetKeywordAt(index));
				if (!keyword)
					continue;
				const auto matched(m_objectTypeByForm.find(keyword.value()->formID));
				if (matched != m_objectTypeByForm.cend())
				{
					// if default type, postpone storage in case there is a more specific match
					if (matched->second == DefaultObjectType<T>())
					{
						hasDefault = true;
					}
					else if (correctType != ObjectType::unknown)
					{
						REL_WARNING("%s/0x%08x mapped to %s already stored with keyword %s, check data", formName, form->formID,
							GetObjectTypeName(matched->second).c_str(), GetObjectTypeName(correctType).c_str());
					}
					else
					{
						correctType = matched->second;
					}
				}
			}
			if (correctType == ObjectType::unknown && hasDefault)
			{
				correctType = DefaultObjectType<T>();
			}
			else
			{
				correctType = OverrideIfBadChoice<T>(form, correctType);
			}
			if (correctType != ObjectType::unknown)
			{
				if (SetObjectTypeForForm(form->formID, correctType))
				{
					DBG_VMESSAGE("%s/0x%08x stored as %s", formName, form->formID, GetObjectTypeName(correctType).c_str());
				}
				else
				{
					REL_WARNING("%s/0x%08x (%s) already stored, check data", formName, form->formID, GetObjectTypeName(correctType).c_str());
				}
				continue;
			}

			// fail-safe is to check if the form has value and store as clutter if so
			// Also, check model path for - you guessed it - clutter. Some base game MISC objects lack keywords.
			if (typedForm->value > 0 || CheckObjectModelPath(form, "clutter"))
			{
				if (SetObjectTypeForForm(form->formID, ObjectType::clutter))
				{
					DBG_VMESSAGE("%s/0x%08x with value %d stored as clutter", formName, form->formID, std::max(typedForm->value, SInt32(0)));
				}
				else
				{
					REL_WARNING("%s/0x%08x (defaulting as clutter) already stored, check data", formName, form->formID);
				}
				continue;
			}
			DBG_VMESSAGE("%s/0x%08x not mappable", formName, form->formID);
		}
	}

	void GetTranslationData(void);
	void ActivationVerbsByType(const char* activationVerbKey, const ObjectType objectType);
	void StoreActivationVerbs(void);
	void CategorizeByActivationVerb(void);
	void BuildCollectionDefinitions(void);
	void AnalyzePerks(void);

	std::string GetModelPath(const RE::TESForm* thisForm) const;
	bool CheckObjectModelPath(const RE::TESForm* thisForm, const char* arg) const;

	static DataCase* s_pInstance;

	// special case statics
	static constexpr RE::FormID LockPick = 0x0A;
	static constexpr RE::FormID Gold = 0x0F;

	void CategorizeStatics();
	void ExcludeImmersiveArmorsGodChest();
	void IncludeFossilMiningExcavation();
	DataCase(void);
};

