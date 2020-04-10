#pragma once

#include <unordered_map>

struct ContainerLister
{
public:
	ContainerLister(const RE::TESObjectREFR* refr, bool requireQuestItemAsTarget) : m_refr(refr), m_requireQuestItemAsTarget(requireQuestItemAsTarget) {};
	bool GetOrCheckContainerForms(std::unordered_map<RE::TESForm*, int>& lootableItems, bool& hasQuestObject, bool& hasEnchItem);
private:
	const RE::TESObjectREFR* m_refr;
	bool m_requireQuestItemAsTarget;
};

