#pragma once

enum class ObjectType : UInt8
{
	unknown = 0,
	flora,
	critter,
	ingredients,
	septims,
	gems,
	lockpick,
	animalHide,
	animalParts,
	oreIngot,
	soulgem,
	keys,
	clutter,
	light,
	// books - must be contiguous
	books,
	spellbook,
	skillbook,
	booksRead,
	spellbookRead,
	skillbookRead,
	// end books
	scrolls,
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
	userlist,
	container,
	actor,
	ashPile,
	manualLoot
};

