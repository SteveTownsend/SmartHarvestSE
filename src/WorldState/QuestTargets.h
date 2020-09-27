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

namespace shse
{

class QuestTargets
{
public:
	static QuestTargets& Instance();
	QuestTargets();

	void Analyze();

	Lootability ReferencedQuestTargetLootability(const RE::TESObjectREFR* refr) const;
	Lootability QuestTargetLootability(const RE::TESForm* form, const RE::TESObjectREFR* refr) const;
	bool UserCannotPermission(const RE::TESForm* form) const;

private:
	// don't make item a Quest Target if instances are scattered all over the place
	static constexpr size_t BoringQuestTargetThreshold = 10;
	// treat as Quest Target even if flag not set, if there are very few instances (one for QUST, one for display maybe)
	static constexpr size_t RareQuestTargetThreshold = 2;

	bool IsLootableInanimateReference(const RE::TESObjectREFR* refr) const;
	bool BlacklistQuestTargetItem(const RE::TESBoundObject* item);
	bool BlacklistQuestTargetReferencedItem(const RE::TESBoundObject* item, const RE::TESObjectREFR* refr);
	bool BlacklistQuestTargetREFR(const RE::TESObjectREFR* refr);
	bool BlacklistQuestTargetNPC(const RE::TESNPC* npc);

	static std::unique_ptr<QuestTargets> m_instance;
	mutable RecursiveLock m_questLock;

	std::unordered_set<const RE::TESForm*> m_userCannotPermission;
	std::unordered_set<const RE::TESForm*> m_questTargetItems;
	std::unordered_map<const RE::TESForm*, std::unordered_set<const RE::TESObjectREFR*>> m_questTargetReferenced;
	std::unordered_set<const RE::TESObjectREFR*> m_questTargetREFRs;
};

}
