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
#include "Data/iniSettings.h"

namespace shse
{

class TheftCoordinator
{
public:
	static TheftCoordinator& Instance();
	TheftCoordinator() : m_stealInProgress(false), m_stealTimer(-1) {}
	void DelayStealableItem(RE::TESObjectREFR * target, INIFile::SecondaryType targetType);
	void StealIfUndetected(void);
	const RE::Actor* ActorByIndex(const int actorIndex) const;
	void StealOrForgetItems(const bool detected);
	bool StealingItems() const;

private:
	static std::unique_ptr<TheftCoordinator> m_instance;
	mutable RecursiveLock m_theftLock;

	std::vector<std::pair<RE::TESObjectREFR*, INIFile::SecondaryType>> m_refrsToSteal;
	std::vector<std::pair<RE::TESObjectREFR*, INIFile::SecondaryType>> m_refrsStealInProgress;
	// ordered by proximity to player at time of recording
	std::vector<const RE::Actor*> m_detectingActors;
	bool m_stealInProgress;
	int m_stealTimer;
};

}
