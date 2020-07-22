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

enum class ObjectType : UInt8
{
	unknown = 0,
	flora,
	critter,
	ingredient,
	septims,
	gem,
	lockpick,
	animalHide,
	oreIngot,
	soulgem,
	key,
	clutter,
	light,
	// books - must be contiguous
	book,
	spellbook,
	skillbook,
	bookRead,
	spellbookRead,
	skillbookRead,
	// end books
	scroll,
	ammo,
	weapon,
	enchantedWeapon,
	armor,
	enchantedArmor,
	jewelry,
	enchantedJewelry,
	potion,
	poison,
	food,
	drink,
	oreVein,
	container,
	actor
};

enum class ResourceType : UInt8
{
	ore = 0,
	geode,
	volcanic,
	volcanicDigSite
};

inline const char* PrintResourceType(ResourceType resourceType)
{
	static std::vector<const char*> resourceTypeNames = { "Ore", "Geode", "Volcanic", "VolcanicDigSite" };
	return resourceTypeNames.at(static_cast<size_t>(resourceType));
}