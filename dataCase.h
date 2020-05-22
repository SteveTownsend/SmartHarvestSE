#pragma once

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>

#include "objects.h"

#include "CommonLibSSE/include/RE/BGSProjectile.h"

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
	void ClearBlockedReferences();

	// permanent REFR blacklist, reset on game reload
	bool BlacklistReference(const RE::TESObjectREFR* refr);
	bool IsReferenceOnBlacklist(const RE::TESObjectREFR* refr);
	void ClearReferenceBlacklist();

	bool IsReferenceLockedContainer(const RE::TESObjectREFR* refr);
	void ForgetLockedContainers();
	void UpdateLockedContainers();

	bool BlockForm(const RE::TESForm* form);
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

	const char* GetTranslation(const char* key) const;

	const RE::TESAmmo* ProjToAmmo(const RE::BGSProjectile* proj);
	RE::TESForm* ConvertIfLeveledItem(RE::TESForm* form) const;

	void CategorizeLootables(void);
	void ListsClear(const bool gameReload);
	bool SkipAmmoLooting(RE::TESObjectREFR* refr);

	bool SetLootableForProducer(RE::TESForm* critter, RE::TESForm* ingredient);
	RE::TESForm* GetLootableForProducer(RE::TESForm* producer) const;

	inline bool IsBookGlowableKeyword(RE::BGSKeyword* keyword) const
	{
		return keyword && m_glowableBookKeywords.find(keyword->GetFormID()) != m_glowableBookKeywords.cend();
	}

private:
	std::unordered_map<std::string, std::string> translations;

	std::unordered_map<const RE::TESObjectREFR*, RE::NiPoint3> arrowCheck;
	std::unordered_map<const RE::BGSProjectile*, RE::TESAmmo*> ammoList;

	std::unordered_set<RE::FormID> userBlockedForm;
	std::unordered_set<const RE::TESForm*> blockForm;
	std::unordered_set<RE::FormID> blockRefr;
	std::unordered_set<RE::FormID> blacklistRefr;
	std::unordered_map<RE::FormID, std::chrono::time_point<std::chrono::high_resolution_clock>> m_lockedContainers;

	std::unordered_map<RE::FormType, ObjectType> m_objectTypeByFormType;
	std::unordered_map<RE::FormID, ObjectType> m_objectTypeByForm;
	std::unordered_map<RE::TESProduceForm*, RE::TESForm*> m_produceFormContents;
	std::unordered_set<RE::FormID> m_glowableBookKeywords;

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
#if _DEBUG
			_MESSAGE("Checking target %s/0x%08x", targetName, form->formID);
#endif

			const RE::TESBoundObject* ingredient(target->produceItem);
			if (!ingredient)
			{
#if _DEBUG
				_MESSAGE("No ingredient for %s/0x%08x", targetName, form->formID);
#endif
				continue;
			}

			// Categorize ingredient
			ObjectType storedType(ObjectType::unknown);
			const RE::TESLevItem* leveledItem(ingredient->As<RE::TESLevItem>());
			if (leveledItem)
			{
#if _DEBUG
				_MESSAGE("%s/0x%08x ingredient is Leveled Item", targetName, form->formID);
#endif
				ProduceFormCategorizer(target, leveledItem, targetName).CategorizeContents();
			}
			else
			{
				// Try the ingredient form on this Produce holder
				storedType = GetObjectTypeForForm(ingredient);
				if (storedType != ObjectType::unknown)
				{
#if _DEBUG
					_MESSAGE("Target %s/0x%08x has ingredient %s/0x%08x stored as type %s", targetName, form->formID,
						ingredient->GetName(), ingredient->formID, GetObjectTypeName(storedType).c_str());
#endif
					SetLootableForProducer(form, const_cast<RE::TESBoundObject*>(ingredient));
				}
				else
				{
					storedType = DefaultIngredientObjectType(target);
				}
				if (storedType != ObjectType::unknown)
				{ 
					// Store mapping of Produce holder to ingredient
					if (SetObjectTypeForForm(form->formID, storedType))
					{
#if _DEBUG
						_MESSAGE("Target %s/0x%08x stored as type %s", targetName, form->formID, GetObjectTypeName(storedType).c_str());
#endif
					}
					else
					{
#if _DEBUG
						_MESSAGE("Target %s/0x%08x (%s) already stored, check data", targetName, form->formID, GetObjectTypeName(storedType).c_str());
#endif
					}
				}
				else
				{
#if _DEBUG
					_MESSAGE("Target %s/0x%08x not stored", targetName, form->formID);
#endif
				}
			}
		}
	}

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
#if _DEBUG
				_MESSAGE("Skipping non-consumable form 0x%08x", form->formID);
#endif
				continue;
			}

			RE::TESFullName* pFullName = form->As<RE::TESFullName>();
			if (!pFullName || pFullName->GetFullNameLength() == 0)
			{
#if _DEBUG
				_MESSAGE("Skipping unnamed form 0x%08x", form->formID);
#endif
				continue;
			}

			std::string formName(pFullName->GetFullName());
			if (GetFormObjectType(form->formID) != ObjectType::unknown)
			{
#if _DEBUG
				_MESSAGE("Skipping previously categorized form %s/0x%08x", formName.c_str(), form->formID);
#endif
				continue;
			}

			ObjectType objectType(ConsumableObjectType<T>(consumable));
#if _DEBUG
			_MESSAGE("Consumable %s/0x%08x has type %s", formName.c_str(), form->formID, GetObjectTypeName(objectType).c_str());
#endif
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
	ObjectType OverrideIfBadChoice(const ObjectType objectType)
	{
		return objectType;
	}
	template <> ObjectType OverrideIfBadChoice<RE::TESObjectARMO>(const ObjectType objectType);

	std::unordered_map<std::string, ObjectType> m_objectTypeByActivationVerb;
	mutable std::unordered_set<std::string> m_unhandledActivationVerbs;
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
#if _DEBUG
			_MESSAGE("Categorizing %s/0x%08x", formName, form->formID);
#endif
			if ((form->formFlags & T::RecordFlags::kNonPlayable) == T::RecordFlags::kNonPlayable)
			{
#if _DEBUG
				_MESSAGE("%s/0x%08x is NonPlayable", formName, form->formID);
#endif
				continue;
			}
			RE::BGSKeywordForm* keywordForm(form->As<RE::BGSKeywordForm>());
			if (!keywordForm)
			{
#if _DEBUG
				_MESSAGE("%s/0x%08x Not a Keyword", formName, form->formID);
#endif
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
#if _DEBUG
						_MESSAGE("%s/0x%08x mapped to %s already stored with keyword %s, check data", formName, form->formID,
							GetObjectTypeName(matched->second).c_str(), GetObjectTypeName(correctType).c_str());
#endif
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
				correctType = OverrideIfBadChoice<T>(correctType);
			}
			if (correctType != ObjectType::unknown)
			{
				if (SetObjectTypeForForm(form->formID, correctType))
				{
#if _DEBUG
					_MESSAGE("%s/0x%08x stored as %s", formName, form->formID, GetObjectTypeName(correctType).c_str());
#endif
				}
				else
				{
#if _DEBUG
					_MESSAGE("%s/0x%08x (%s) already stored, check data", formName, form->formID, GetObjectTypeName(correctType).c_str());
#endif
				}
				continue;
			}

			// fail-safe is to check if the form has value and store as clutter if so
			// Also, check model path for - you guessed it - clutter. Some base game MISC objects lack keywords.
			RE::TESValueForm* valueForm(form->As<RE::TESValueForm>());
			if ((valueForm && valueForm->value > 0) || CheckObjectModelPath(form, "clutter"))
			{
				if (SetObjectTypeForForm(form->formID, ObjectType::clutter))
				{
#if _DEBUG
					_MESSAGE("%s/0x%08x with value %d stored as clutter", formName, form->formID, valueForm->value);
#endif
				}
				else
				{
#if _DEBUG
					_MESSAGE("%s/0x%08x (defaulting as clutter) already stored, check data", formName, form->formID);
#endif
				}
				continue;
			}
#if _DEBUG
			_MESSAGE("%s/0x%08x not mappable", formName, form->formID);
#endif
		}
	}

	void GetTranslationData(void);
	void ActivationVerbsByType(const char* activationVerbKey, const ObjectType objectType);
	void StoreActivationVerbs(void);
	void CategorizeByActivationVerb(void);

	std::string GetModelPath(const RE::TESForm* thisForm) const;
	bool CheckObjectModelPath(const RE::TESForm* thisForm, const char* arg) const;

	static DataCase* s_pInstance;

	// special case statics
	static constexpr RE::FormID LockPick = 0x0A;
	static constexpr RE::FormID Gold = 0x0F;

	void CategorizeStatics();
	DataCase(void);
};

