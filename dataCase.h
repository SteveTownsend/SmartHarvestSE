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

	struct listData
	{
		std::unordered_map<std::string, std::string> translations;

		std::unordered_map<const RE::TESObjectREFR*, RE::NiPoint3> arrowCheck;
		std::unordered_map<const RE::BGSProjectile*, RE::TESAmmo*> ammoList;

		std::unordered_set<const RE::TESForm*> blockForm;
		std::unordered_set<const RE::TESObjectREFR*> blockRefr;

		std::vector<RE::TESSound*> sound;
	} lists;

	bool BlockReference(const RE::TESObjectREFR* refr);
	bool UnblockReference(const RE::TESObjectREFR* refr);
	bool IsReferenceBlocked(const RE::TESObjectREFR* refr);

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

	const RE::TESAmmo* ProjToAmmo(const RE::BGSProjectile* proj);
	const RE::TESForm* ConvertIfLeveledItem(const RE::TESForm* form) const;

	void BuildList(void);
	void ListsClear(void);

	bool SetIngredientForCritter(RE::TESForm* critter, RE::TESForm* ingredient);
	const RE::IngredientItem* GetIngredientForCritter(RE::TESObjectACTI* activator) const;

private:
	std::unordered_map<RE::FormType, ObjectType> m_objectTypeByFormType;
	std::unordered_map<RE::FormID, ObjectType> m_objectTypeByForm;
	std::unordered_map<const RE::TESProduceForm*, const RE::TESForm*> m_leveledItemContents;

	mutable concurrency::critical_section m_critterIngredientLock;
	std::unordered_map<const RE::TESObjectACTI*, const RE::IngredientItem*> m_critterIngredient;

	bool GetTSV(std::unordered_set<const RE::TESForm*> *tsv, const char* fileName);

	void GetBlockContainerData(void);
	void GetAmmoData(void);

	template <typename T>
	ObjectType IngredientObjectType(const T* form)
	{
		return ObjectType::unknown;
	}

	template <>	ObjectType IngredientObjectType(const RE::TESFlora* form);
	template <>	ObjectType IngredientObjectType(const RE::TESObjectTREE* form);

	class LeveledItemCategorizer
	{
	public:
		LeveledItemCategorizer(const RE::TESProduceForm* produceForm, const RE::TESLevItem* rootItem, const std::string& targetName);
		std::pair<RE::TESForm*, ObjectType> FindContents();

	private:
		void FindContentsAtLevel(const RE::TESLevItem* leveledItem);

		const RE::TESProduceForm* m_produceForm;
		const RE::TESLevItem* m_rootItem;
		const std::string m_targetName;
		RE::TESForm* m_contents;
		ObjectType m_objectType;
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
			if (ingredient == nullptr)
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
				std::pair<RE::TESForm*, ObjectType> contents(LeveledItemCategorizer(target, leveledItem, targetName).FindContents());
				if (contents.first)
				{
					RE::TESForm* contentsForm(contents.first);
					if (!m_leveledItemContents.insert(std::make_pair(target, contentsForm)).second)
					{
#if _DEBUG
						_MESSAGE("Leveled Item %s/0x%08x contents already present", targetName, form->formID);
#endif
					}
					else
					{
#if _DEBUG
						_MESSAGE("Leveled Item %s/0x%08x has contents %s/0x%08x", targetName, form->formID, contentsForm->GetName(), contentsForm->formID);
#endif
						if (!m_objectTypeByForm.insert(std::make_pair(contentsForm->formFlags, contents.second)).second)
						{
#if _DEBUG
							_MESSAGE("Leveled Item %s/0x%08x contents %s/0x%08x already has an ObjectType", targetName, form->formID, contentsForm->GetName(), contentsForm->formID);
#endif
						}
					}
				}
				else
				{
#if _DEBUG
					_MESSAGE("Leveled Item %s/0x%08x not stored", targetName, form->formID);
#endif
				}
			}
			else
			{
				storedType = IngredientObjectType(target);
				if (storedType != ObjectType::unknown)
				{ 
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
	void CategorizeConsumables();

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

