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
	whitelist,
	blacklist,
	container,
	actor,
	ashPile,
	manualLoot,
	collectible
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