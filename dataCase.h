#pragma once

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <concrt.h>

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
	bool UnblockReference(const RE::TESObjectREFR* refr);
	bool IsReferenceBlocked(const RE::TESObjectREFR* refr);

	bool BlockForm(const RE::TESForm* form);
	bool UnblockForm(const RE::TESForm* form);
	bool IsFormBlocked(const RE::TESForm* form);

	ObjectType GetFormObjectType(RE::FormID formID) const;
	bool SetFormObjectType(RE::FormID formID, ObjectType objectType);
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
	const RE::TESForm* ConvertIfLeveledItem(const RE::TESForm* form) const;

	void CategorizeLootables(void);
	void ListsClear(void);
	bool CheckAmmoLootable(RE::TESObjectREFR* refr);

	bool SetLootableForProducer(RE::TESForm* critter, RE::TESForm* ingredient);
	const RE::TESForm* GetLootableForProducer(RE::TESForm* producer) const;

private:
	struct listData
	{
		std::unordered_map<std::string, std::string> translations;

		std::unordered_map<const RE::TESObjectREFR*, RE::NiPoint3> arrowCheck;
		std::unordered_map<const RE::BGSProjectile*, RE::TESAmmo*> ammoList;

		std::unordered_set<const RE::TESForm*> userBlockedForm;
		std::unordered_set<const RE::TESForm*> blockForm;
		std::unordered_set<const RE::TESObjectREFR*> blockRefr;
	} lists;

	std::unordered_map<RE::FormType, ObjectType> m_objectTypeByFormType;
	std::unordered_map<RE::FormID, ObjectType> m_objectTypeByForm;
	std::unordered_map<const RE::TESProduceForm*, const RE::TESForm*> m_produceFormContents;

	mutable concurrency::critical_section m_producerIngredientLock;
	mutable concurrency::critical_section m_blockListLock;
	std::unordered_map<const RE::TESForm*, const RE::TESForm*> m_producerLootable;

	bool GetTSV(std::unordered_set<const RE::TESForm*> *tsv, const char* fileName);

	void CategorizeNPCDeathItems(void);
	void GetBlockContainerData(void);
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
		ProduceFormCategorizer(const RE::TESProduceForm* produceForm, const RE::TESLevItem* rootItem, const std::string& targetName);

	protected:
		virtual void ProcessContentLeaf(RE::TESForm* itemForm, ObjectType itemType) override;

	private:
		const RE::TESProduceForm* m_produceForm;
		RE::TESForm* m_contents;
	};

#if 0
	class NPCDeathItemCategorizer : public LeveledItemCategorizer
	{
	public:
		NPCDeathItemCategorizer(const RE::TESNPC* npc, const RE::TESLevItem* rootItem, const std::string& targetName);

	protected:
		virtual void ProcessContentLeaf(RE::TESForm* itemForm, ObjectType itemType) override;

	private:
		const RE::TESNPC* m_npc;
	};

#endif
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
					SetFormObjectType(form->formID, storedType);
#if _DEBUG
					_MESSAGE("Target %s/0x%08x stored as type %s", targetName, form->formID, GetObjectTypeName(storedType).c_str());
#endif
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
	void CategorizeByKeyword()
	{
		RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
		if (!dhnd)
			return;

#if _DEBUG
		size_t additions(0);
#endif
		for (RE::TESForm* form : dhnd->GetFormArray(T::FORMTYPE))
		{
			T* typedForm(form->As<T>());
			if (!typedForm || !typedForm->GetFullNameLength())
				continue;
			const char* formName(typedForm->GetFullName());
			bool stored(false);

			RE::BSString activationText;
			if (typedForm->GetActivateText(RE::PlayerCharacter::GetSingleton(), activationText))
			{
				// activation via 'Catch' -> Critter
				// activation via 'Harvest' -> Flora
				// activation via 'Mine' -> Ore Vein
				static const std::vector<std::pair<std::string, ObjectType>> activators = {
					{"Catch", ObjectType::critter},
					{"Harvest", ObjectType::flora},
					{"Mine", ObjectType::oreVein} };

				std::string strActivation(activationText.c_str(), activationText.size());
#if _DEBUG
				_MESSAGE("%s/0x%08x activated using '%s'", formName, form->formID, strActivation.c_str());
#endif
				ObjectType activatorType(ObjectType::unknown);
				for (auto const & activator : activators)
				{
					if (strActivation.length() > activator.first.length() && strActivation.compare(0, activator.first.length(), activator.first) == 0)
					{
						activatorType = activator.second;
						break;
					}
				}
				if (activatorType != ObjectType::unknown)
				{
					stored = true;
					if (SetFormObjectType(form->formID, activatorType))
					{
#if _DEBUG
						_MESSAGE("Stored %s/0x%08x as %s", formName, form->formID, GetObjectTypeName(activatorType).c_str());
						++additions;
#endif
					}
					else
					{
#if _DEBUG
						_MESSAGE("%s/0x%08x (%s) already stored, check data", formName, form->formID, GetObjectTypeName(activatorType).c_str());
#endif
					}

				}
			}

			if (!stored)
			{
#if _DEBUG
				_MESSAGE("Categorizing %s/0x%08x", formName, form->formID);
#endif
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
							_MESSAGE("%s/0x%08x already stored with a keyword, check data", formName, form->formID);
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
				if (correctType == ObjectType::clutter)
				{
					// special cases for clutter to preserve historic taxonomy
					if (CheckObjectModelPath(form, "dwemer"))
						correctType = ObjectType::clutterDwemer;
					else if (CheckObjectModelPath(form, "broken"))
						correctType = ObjectType::clutterBroken;
				}
				if (correctType != ObjectType::unknown)
				{
					stored = true;
					if (SetFormObjectType(form->formID, correctType))
					{
#if _DEBUG
						_MESSAGE("Stored %s/0x%08x as %s", formName, form->formID, GetObjectTypeName(correctType).c_str());
						++additions;
#endif
					}
				}
			}

			if (!stored)
			{
				// Check if the form has value and store as clutter if so
				RE::TESValueForm* valueForm(form->As<RE::TESValueForm>());
				if (valueForm && valueForm->value > 0)
				{
#if _DEBUG
					_MESSAGE("Uncategorized %s/0x%08x has value %d as clutter", formName, form->formID, valueForm->value);
					++additions;
#endif
					SetFormObjectType(form->formID, ObjectType::clutter);
				}
				else
				{
#if _DEBUG
					_MESSAGE("%s/0x%08x not mappable", formName, form->formID);
#endif
				}

			}
		}

#if _DEBUG
		_MESSAGE("* Stored %d in-process forms by keyword", additions);
#endif
	}

	void GetTranslationData(void);

	std::string GetModelPath(const RE::TESForm* thisForm) const;
	bool CheckObjectModelPath(const RE::TESForm* thisForm, const char* arg) const;

	static DataCase* s_pInstance;

	// special case statics
	static constexpr RE::FormID LockPick = 0x0A;
	static constexpr RE::FormID Gold = 0x0F;

	void CategorizeStatics();

	DataCase(void);
};

