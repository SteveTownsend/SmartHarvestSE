#pragma once

#include <unordered_map>

struct ContainerLister
{
public:
	ContainerLister(const RE::TESObjectREFR* refr, SInt32 questObjDefinition) : m_refr(refr), m_requireFullFlags(questObjDefinition != 0) {};
	bool GetOrCheckContainerForms(std::unordered_map<RE::TESForm*, int>& lootableItems, bool& hasQuestObject, bool& hasEnchItem);
private:
	const RE::TESObjectREFR* m_refr;
	bool m_requireFullFlags;
};

