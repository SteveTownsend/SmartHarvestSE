{
  Identifies FormLists with concrete collectible objects.
}
unit UserScript;

var
  formLists: TStringList;
  formIds: TStringList;
  buffer: TStringList;
  collectionPreamble: string;
  collectionTemplate: string;
  collectionPostscript: string;
  notFirst: Boolean;
  fileCount: integer;
  nameMap: TStringList;

function Initialize: integer;
begin
  notFirst := False;

  collectionPreamble := '{ "$comment": "Definitions are allowed for up to 128 rulesets per file for Collections", "$schema": "./SHSE.SchemaCollections.json", "groupPolicy": { "action": "glow", "notify": true, "repeat": false }, "useMCM": true, "collections": [ ';
  collectionTemplate := '{OPTIONALCOMMA}{ "name": "{NAME}", "description": "{DESCRIPTION}", "rootFilter": { "operator": "AND", "condition": { "formList": [ { "listPlugin": "{PLUGIN}" , "formID": "{FORMID}" } ] } } }';
  collectionPostscript := ' ] }';

  formLists := TStringList.Create;
  formIds := TStringList.Create;
  buffer := TStringList.Create;
  fileCount := 0;
  formLists.Add(collectionPreamble);
 
  nameMap := TStringList.Create;
  nameMap.Values['AOS'] := 'Amulets of Skyrim';
  nameMap.Values['AOBJewelry'] := 'Jewelry - Artifacts of Boethiah';
  nameMap.Values['ArmoryAncientNord'] := 'Armory - Ancient Nord';
  nameMap.Values['ArmoryBlades'] := 'Armory - Blades';
  nameMap.Values['ArmoryDGA'] := 'Armory - Dawnguard Arsenal';
  nameMap.Values['ArmoryDaedric'] := 'Armory - Daedric';
  nameMap.Values['ArmoryDawnguard'] := 'Armory - Dawnguard';
  nameMap.Values['ArmoryDragon'] := 'Armory - Dragon';
  nameMap.Values['ArmoryDwarven'] := 'Armory - Dwarven';
  nameMap.Values['ArmoryEastWingAncientNordStalhrim'] := 'Armory - Ancient Nord Stalhrim';
  nameMap.Values['ArmoryEastWingBrimstoneWyrmstone'] := 'Armory - Brimstone & Wyrmstone';
  nameMap.Values['ArmoryEbony'] := 'Armory - Ebony';
  nameMap.Values['ArmoryElven'] := 'Armory - Elven';
  nameMap.Values['ArmoryExtraDisplays'] := 'Armory - Extra Displays';
  nameMap.Values['ArmoryFalmer'] := 'Armory - Falmer';
  nameMap.Values['ArmoryForsworn'] := 'Armory - Forsworn';
  nameMap.Values['ArmoryGAR'] := 'Armory - GAR';
  nameMap.Values['ArmoryGlass'] := 'Armory - Glass';
  nameMap.Values['ArmoryGuardArmorDisplay'] := 'Armory - Guard';
  nameMap.Values['ArmoryHAAncientNordic'] := 'Armory - HA Ancient Nord';
  nameMap.Values['ArmoryHABlades'] := 'Armory - HA Blades';
  nameMap.Values['ArmoryHADaedric'] := 'Armory - HA Daedric';
  nameMap.Values['ArmoryHADawnguard'] := 'Armory - HA Dawnguard';
  nameMap.Values['ArmoryHADragon'] := 'Armory - HA Dragon';
  nameMap.Values['ArmoryHADwarven'] := 'Armory - HA Dwarven';
  nameMap.Values['ArmoryHAEbony'] := 'Armory - HA Ebony';
  nameMap.Values['ArmoryHAElven'] := 'Armory - HA Elven';
  nameMap.Values['ArmoryHAFalmer'] := 'Armory - HA Falmer';
  nameMap.Values['ArmoryHAForsworn'] := 'Armory - HA Forsworn';
  nameMap.Values['ArmoryHAGlass'] := 'Armory - HA Glass';
  nameMap.Values['ArmoryHAImperial'] := 'Armory - HA Imperial';
  nameMap.Values['ArmoryHAIron'] := 'Armory - HA Iron';
  nameMap.Values['ArmoryHANordHero'] := 'Armory - HA Nord Heroic';
  nameMap.Values['ArmoryHANordic'] := 'Armory - HA Nordic';
  nameMap.Values['ArmoryHAOrcish'] := 'Armory - HA Orcish';
  nameMap.Values['ArmoryHASilver'] := 'Armory - HA Silver';
  nameMap.Values['ArmoryHAStalhrim'] := 'Armory - HA Stalhrim';
  nameMap.Values['ArmoryHASteel'] := 'Armory - HA Steel';
  nameMap.Values['ArmoryHeavyArmory'] := 'Armory - HA Armory';
  nameMap.Values['ArmoryImmersiveArmorsBlades'] := 'Armory - IA Blades';
  nameMap.Values['ArmoryImmersiveArmorsCW'] := 'Armory - IA Civil War';
  nameMap.Values['ArmoryImmersiveArmorsDragon'] := 'Armory - IA Dragon';
  nameMap.Values['ArmoryImmersiveWeaponsAncientNordic'] := 'Armory - IW Ancient Nord';
  nameMap.Values['ArmoryImmersiveWeaponsBlades'] := 'Armory - IW Blades';
  nameMap.Values['ArmoryImmersiveWeaponsDaedric'] := 'Armory - IW Daedric';
  nameMap.Values['ArmoryImmersiveWeaponsDawnguard'] := 'Armory - IW Dawnguard';
  nameMap.Values['ArmoryImmersiveWeaponsDragon'] := 'Armory - IW Dragon';
  nameMap.Values['ArmoryImmersiveWeaponsDwarven'] := 'Armory - IW Dwarven';
  nameMap.Values['ArmoryImmersiveWeaponsEbony'] := 'Armory - IW Ebony';
  nameMap.Values['ArmoryImmersiveWeaponsElven'] := 'Armory - IW Elven';
  nameMap.Values['ArmoryImmersiveWeaponsFalmer'] := 'Armory - IW Falmer';
  nameMap.Values['ArmoryImmersiveWeaponsGlass'] := 'Armory - IW Glass';
  nameMap.Values['ArmoryImmersiveWeaponsIron'] := 'Armory - IW Iron';
  nameMap.Values['ArmoryImmersiveWeaponsOrcish'] := 'Armory - IW Orcish';
  nameMap.Values['ArmoryImmersiveWeaponsSteel'] := 'Armory - IW Steel';
  nameMap.Values['ArmoryImmersiveWeaponsWolf'] := 'Armory - IW Wolf';
  nameMap.Values['ArmoryIron'] := 'Armory - Iron';
  nameMap.Values['ArmoryJaysusSwords'] := 'Armory - Jaysus Swords';
  nameMap.Values['ArmoryNordic'] := 'Armory - Nordic';
  nameMap.Values['ArmoryOrcish'] := 'Armory - Orcish';
  nameMap.Values['ArmorySnowElf'] := 'Armory - Snow Elven';
  nameMap.Values['ArmoryStalhrim'] := 'Armory - Stalhrim';
  nameMap.Values['ArmorySteel'] := 'Armory - Steel';
  nameMap.Values['ArmoryThaneWeapons'] := 'Armory - Thane Weapons';
  nameMap.Values['Armory_BWP'] := 'Armory - Bonemold Weapons';
  nameMap.Values['Armory_NewArmouryAkaviri'] := 'Armory - New Akaviri';
  nameMap.Values['Armory_NewArmouryDaedric'] := 'Armory - New Daedric';
  nameMap.Values['Armory_NewArmouryDawnguard'] := 'Armory - New Dawnguard';
  nameMap.Values['Armory_NewArmouryDragonbone'] := 'Armory - New Dragonbone';
  nameMap.Values['Armory_NewArmouryDraugr'] := 'Armory - New Draugr';
  nameMap.Values['Armory_NewArmouryDwarven'] := 'Armory - New Dwarven';
  nameMap.Values['Armory_NewArmouryEbony'] := 'Armory - New Ebony';
  nameMap.Values['Armory_NewArmouryElven'] := 'Armory - New Elven';
  nameMap.Values['Armory_NewArmouryFalmer'] := 'Armory - New Falmer';
  nameMap.Values['Armory_NewArmouryForsworn'] := 'Armory - New Forsworn';
  nameMap.Values['Armory_NewArmouryGlass'] := 'Armory - New Glass';
  nameMap.Values['Armory_NewArmouryImperial'] := 'Armory - New Imperial';
  nameMap.Values['Armory_NewArmouryIron'] := 'Armory - New Iron';
  nameMap.Values['Armory_NewArmouryNordic'] := 'Armory - New Nordic';
  nameMap.Values['Armory_NewArmouryOrcish'] := 'Armory - New Orcish';
  nameMap.Values['Armory_NewArmouryStalhrim'] := 'Armory - New Stalhrim';
  nameMap.Values['Armory_NewArmourySteel'] := 'Armory - New Steel';
  nameMap.Values['CCAoC'] := 'Arms of Chaos';
  nameMap.Values['CC_DwMudcrab'] := 'Dwemer Mudcrab';
  nameMap.Values['CC_NixHound'] := 'Nix Hound';
  nameMap.Values['DBHCWI'] := 'Immersive College';
  nameMap.Values['DBHGrayCowl'] := 'Gray Cowl';
  nameMap.Values['DBHKA'] := 'Konahrik''s Accoutrements';
  nameMap.Values['DBHTeldrynSerious'] := 'Teldryn Serious';
  nameMap.Values['DBHallAHO'] := 'Project AHO';
  nameMap.Values['DBHallAchievements'] := 'Achievements';
  nameMap.Values['DBHallClockwork'] := 'Clockwork';
  nameMap.Values['DBHallFalskaar'] := 'Falskaar';
  nameMap.Values['DBHallForgottenCity'] := 'Forgotten City';
  nameMap.Values['DBHallHelgen'] := 'Helgen Reborn';
  nameMap.Values['DBHallMAS'] := 'Moon and Star';
  nameMap.Values['DBHallMoonpath'] := 'Moonpath to Elsweyr';
  nameMap.Values['DBHallSkyrimSewers'] := 'Skyrim Sewers';
  nameMap.Values['DBHallSkyshardsBlackreach'] := 'Skyshards - Blackreach';
  nameMap.Values['DBHallSkyshardsDawnguard'] := 'Skyshards - Dawnguard';
  nameMap.Values['DBHallSkyshardsEastmarch'] := 'Skyshards - Eastmarch';
  nameMap.Values['DBHallSkyshardsExplorer'] := 'Skyshards - Explorer';
  nameMap.Values['DBHallSkyshardsFalkreath'] := 'Skyshards - Falkreath';
  nameMap.Values['DBHallSkyshardsFalskaar'] := 'Skyshards - Falskaar';
  nameMap.Values['DBHallSkyshardsHaafingar'] := 'Skyshards - Haafingar';
  nameMap.Values['DBHallSkyshardsHjaalmarch'] := 'Skyshards - Hjaalmarch';
  nameMap.Values['DBHallSkyshardsMuseum'] := 'Skyshards - Museum';
  nameMap.Values['DBHallSkyshardsPale'] := 'Skyshards - The Pale';
  nameMap.Values['DBHallSkyshardsReach'] := 'Skyshards - The Reach';
  nameMap.Values['DBHallSkyshardsRift'] := 'Skyshards - The Rift';
  nameMap.Values['DBHallSkyshardsSolstheim'] := 'Skyshards - Solstheim';
  nameMap.Values['DBHallSkyshardsWhiterun'] := 'Skyshards - Whiterun';
  nameMap.Values['DBHallSkyshardsWinterhold'] := 'Skyshards - Winterhold';
  nameMap.Values['DBHallSkyshardsWyrmstooth'] := 'Skyshards - Wyrmstooth';
  nameMap.Values['DBHallTheBrotherhoodOfOld'] := 'Brotherhood of Old';
  nameMap.Values['DGBittercup'] := 'Daedric Gallery - CC Bittercup';
  nameMap.Values['DGGoblins'] := 'Daedric Gallery - CC Goblins';
  nameMap.Values['DGSaS'] := 'Daedric Gallery - CC Saints & Seducers';
  nameMap.Values['DGShadowrend'] := 'CC Shadowrend';
  nameMap.Values['DaedricGallery'] := 'Daedric Artifacts';
  nameMap.Values['DaedricGalleryAOB'] := 'Daedric Gallery - Artifacts of Boethiah';
  nameMap.Values['DaedricGalleryCCRE'] := 'CC Ruin''s Edge';
  nameMap.Values['DaedricGalleryCCStaffOfSheogorath'] := 'CC Staff of Sheogorath';
  nameMap.Values['DaedricGalleryGrayCowl'] := 'Gray Cowl of Nocturnal';
  nameMap.Values['DaedricGalleryIdentityCrisis'] := 'Identity Crisis';
  nameMap.Values['DaedricGalleryImmersiveWeapons'] := 'Daedric Gallery - Immersive Weapons';
  nameMap.Values['DaedricGalleryUmbra'] := 'CC Umbra';
  nameMap.Values['EEHPlayerHomes'] := 'Player Homes';
  nameMap.Values['Guildhouse'] := 'Guildhouse';
  nameMap.Values['HLEDwemerSpectres'] := 'Dwemer Spectres';
  nameMap.Values['HOFMihaill'] := 'Legend of Slenderman';
  nameMap.Values['HOHAOB'] := 'Artifacts of Boethiah';
  nameMap.Values['HOHCultureandArts'] := 'Culture and Arts';
  nameMap.Values['HOHEbonyWarrior'] := 'Ryn''s Ebony Warrior Overhaul';
  nameMap.Values['HOHGroundFloorLeft'] := 'Hall of Heroes - Ground Floor Left';
  nameMap.Values['HOHGroundFloorRight'] := 'Hall of Heroes - Ground Floor Right';
  nameMap.Values['HOHImmersiveArmorsUnique'] := 'Hall of Heroes - Immersive Armors';
  nameMap.Values['HOHImmersiveWeapons'] := 'Hall of Heroes - Immersive Weapons';
  nameMap.Values['HOHJewelry'] := 'Jewelry';
  nameMap.Values['HOHJewelryTOK'] := 'Jewelry - Tools of Kagrenac';
  nameMap.Values['HOHMasksAndClaws'] := 'Masks & Claws';
  nameMap.Values['HOHOAP'] := 'Oblivion Artifacts Pack';
  nameMap.Values['HOHOAPJewelry'] := 'Jewelry - Oblivion Artifacts Pack';
  nameMap.Values['HOHReceptionHall'] := 'Reception Hall';
  nameMap.Values['HOHRelicHunterAddon'] := 'Relic Hunter Addon';
  nameMap.Values['HOHTOK'] := 'Tools of Kagrenac';
  nameMap.Values['HOHUpperGallery'] := 'Upper Gallery';
  nameMap.Values['HOLEAetherium'] := 'Aetherium Weapons & Armor';
  nameMap.Values['HOLEAethernautics'] := 'Aethernautics';
  nameMap.Values['HOLEBlackreachRailroad'] := 'Blackreach Railroad';
  nameMap.Values['HOLEBrhuce'] := 'Brhuce Hammar';
  nameMap.Values['HOLELull'] := 'Wheels of Lull';
  nameMap.Values['HOLEMainFloor'] := 'Hall of Lost Empires - Main Floor';
  nameMap.Values['HOLEMzarkWonders'] := 'Wonders of Mzark';
  nameMap.Values['HOLETOK'] := 'Hall of Lost Empires - Tools of Kagrenac';
  nameMap.Values['HOLEUpperRing'] := 'Hall of Lost Empires - Upper';
  nameMap.Values['HOOBGJars'] := 'Badgremlin''s Jars';
  nameMap.Values['HOOInterestingNPCs'] := 'Interesting NPCs';
  nameMap.Values['HOOMainFloor'] := 'Hall of Oddities - Main Floor';
  nameMap.Values['HOOSUT'] := 'Skyrim Unique Treasures';
  nameMap.Values['HOOUnslaad'] := 'Unslaad';
  nameMap.Values['HOOVigilant'] := 'Vigilant';
  nameMap.Values['HOOWyrmstooth'] := 'Wyrmstooth';
  nameMap.Values['HOSBGHeads'] := 'Badgremlin''s Trophy Heads';
  nameMap.Values['HOSDisplays'] := 'Hall of Secrets - Displays';
  nameMap.Values['HOSRoyalArmory'] := 'Royal Armory';
  nameMap.Values['HOSSinisterSeven'] := 'Sinister Seven';
  nameMap.Values['HOSUndeath'] := 'Undeath';
  nameMap.Values['HOSVolkiharKnight'] := 'Volkihar Knight';
  nameMap.Values['HOS_YCMDB'] := 'Your Choices Matter - Dark Brotherhood';
  nameMap.Values['HoFBeldamsWeave'] := 'Beldam''s Weave';
  nameMap.Values['HoFBillRoW'] := 'Billyro Weapons';
  nameMap.Values['HoFCarvedBrink'] := 'Carved Brink';
  nameMap.Values['HoFCollect1'] := 'Your Orb to Ponder';
  nameMap.Values['HoFCollect10'] := 'Hall of Forgotten - Collection 10';
  nameMap.Values['HoFCollect2'] := 'Dragonling Eggs';
  nameMap.Values['HoFCollect3'] := 'Golden Egg Treasure Hunt';
  nameMap.Values['HoFCollect4'] := 'More Colorful Collectibles';
  nameMap.Values['HoFCollect5'] := 'Children''s Toys';
  nameMap.Values['HoFCollect6'] := 'Forgotten Art';
  nameMap.Values['HoFCollect7'] := 'PSBoss''s Statues';
  nameMap.Values['HoFCollect8'] := 'Forgotten in History';
  nameMap.Values['HoFCollect9'] := 'Narrative Loot';
  nameMap.Values['HoFCollect9_books'] := 'Narrative Loot - Books';
  nameMap.Values['HoFCollect9_misc'] := 'Narrative Loot - Paintings';
  nameMap.Values['HoFColovianPrince'] := 'Colovian Prince';
  nameMap.Values['HoFCore'] := 'Hall of Forgotten';
  nameMap.Values['HoFDCRKS'] := 'DCR - King Crusader';
  nameMap.Values['HoFDX'] := 'DX Armors';
  nameMap.Values['HoFEbonyWarriorRI'] := 'Ebony Warrior Re-imagined';
  nameMap.Values['HoFEchtra'] := 'Away, Come Away';
  nameMap.Values['HoFFARAAM'] := 'Regal Paladin Armor';
  nameMap.Values['HoFFaaC'] := 'Female Armors & Accessories Collection';
  nameMap.Values['HoFFerrumNibenis'] := 'Ferrum Nibenis';
  nameMap.Values['HoFGlenMoril'] := 'Glenmoril';
  nameMap.Values['HoFGOAAIO'] := 'Goampuja AIO';
  nameMap.Values['HoFHelRising'] := 'Hel Rising';
  nameMap.Values['HoFHound'] := 'Curse of the Hound Amulet';
  nameMap.Values['HoFKSA'] := 'Viridian Knight Armor';
  nameMap.Values['HoFKozakowy'] := 'Kozakowy''s Falka Armor';
  nameMap.Values['HoFKvetchiMS'] := 'Kvetchi Mercenary';
  nameMap.Values['HoFLAShiels'] := 'Legendary Alpha Shields';
  nameMap.Values['HoFLunarG'] := 'Lunar Guard Armor';
  nameMap.Values['HoFMaelstrom'] := 'Maelstrom';
  nameMap.Values['HoFMagnusg'] := 'Magnus Gauntlet';
  nameMap.Values['HoFMidwoodIsland'] := 'Midwood Island';
  nameMap.Values['HoFOnyxLancer'] := 'Onyx Lancer';
  nameMap.Values['HoFPUAIO'] := 'Pulcharmsolis AIO';
  nameMap.Values['HoFROH'] := 'Relics of Hyrule';
  nameMap.Values['HoFROHBooksItem'] := 'Relics of Hyrule - Books';
  nameMap.Values['HoFROHTokenItem'] := 'Relics of Hyrule - Tokens';
  nameMap.Values['HoFSirenRoot'] := 'Sirenroot';
  nameMap.Values['HoFTitusMede'] := 'Titus Mede Armor';
  nameMap.Values['HoFVelothi'] := 'Armors of Velothi';
  nameMap.Values['HoFWeapon'] := 'Hall of Forgotten - Weapons';
  nameMap.Values['HoFksws03'] := 'Baba Yaga and the Labyrinth';
  nameMap.Values['HoFksws04'] := 'Welkynar Knight';
  nameMap.Values['HoHDeadMansDread'] := 'Hall of Heroes - CC Dead Man''s Dread';
  nameMap.Values['HoHGallowsHall'] := 'CC Gallows Hall';
  nameMap.Values['HoHKotN'] := 'Knights of the Nine';
  nameMap.Values['HoHWintersun'] := 'Wintersun';
  nameMap.Values['HoLEktWeaponPack'] := 'Kthonia''s Weapon Pack';
  nameMap.Values['HoWAADaedricMail'] := 'CC Daedric Mail';
  nameMap.Values['HoWAADaedricPlate'] := 'CC Daedric Plate';
  nameMap.Values['HoWAADragonplate'] := 'CC Dragon Plate';
  nameMap.Values['HoWAADragonscale'] := 'CC Dragon Scale';
  nameMap.Values['HoWAAEbonyPlate'] := 'CC Ebony Plate';
  nameMap.Values['HoWAAOrcishPlate'] := 'CC Orcish Plate';
  nameMap.Values['HoWAAOrcishScaled'] := 'CC Orcish Scaled';
  nameMap.Values['HoWAASilver'] := 'CC Silver';
  nameMap.Values['HoWAlmsivi'] := 'CC Ghosts of the Tribunal';
  nameMap.Values['HoWArcaneAccessories'] := 'CC Arcane Accessories';
  nameMap.Values['HoWArcaneArcher'] := 'CC Arcane Archer';
  nameMap.Values['HoWBackpacks'] := 'CC Backpacks';
  nameMap.Values['HoWBittercup'] := 'CC Bittercup';
  nameMap.Values['HoWBoneWolf'] := 'CC Bone Wolf';
  nameMap.Values['HoWCivilWarChampions'] := 'CC Civil War Champions';
  nameMap.Values['HoWCore'] := 'Hall of Wonders - Core';
  nameMap.Values['HoWDawnfang'] := 'CC Dawnfang';
  nameMap.Values['HoWDeadMansDread'] := 'CC Dead Man''s Dread';
  nameMap.Values['HoWDwarvenMail'] := 'CC Dwarven Mail';
  nameMap.Values['HoWDwarvenPlate'] := 'CC Dwarven Plate';
  nameMap.Values['HoWECSS'] := 'Extended Cut - Saints & Seducers';
  nameMap.Values['HoWEliteCrossbows'] := 'CC Elite Crossbows';
  nameMap.Values['HoWElvenHunter'] := 'CC Elven Hunter';
  nameMap.Values['HoWExpandedCrossbows'] := 'CC Expanded Crossbows';
  nameMap.Values['HoWFearsomeFists'] := 'CC Fearsome Fists';
  nameMap.Values['HoWFishing'] := 'CC Fishing';
  nameMap.Values['HoWForgottenSeasons'] := 'CC Forgotten Seasons';
  nameMap.Values['HoWGoblins'] := 'CC Goblins';
  nameMap.Values['HoWHeadsmansCleaver'] := 'CC Headsman''s Cleaver';
  nameMap.Values['HoWHorseArmorDLCElven'] := 'CC Horse Armor - Elven';
  nameMap.Values['HoWHorseArmorDLCSteel'] := 'CC Horse Armor - Steel';
  nameMap.Values['HoWIronPlate'] := 'CC Iron Plate';
  nameMap.Values['HoWLeatherScout'] := 'CC Leather Scout';
  nameMap.Values['HoWNecroGrimoire'] := 'CC Necromancer''s Grimoire';
  nameMap.Values['HoWNetchLeather'] := 'CC Netch Leather';
  nameMap.Values['HoWNordicJewelry'] := 'CC Nordic Jewelry';
  nameMap.Values['HoWPets'] := 'CC Pets';
  nameMap.Values['HoWPotD'] := 'CC Plague of the Dead';
  nameMap.Values['HoWRareCurios'] := 'CC Rare Curios';
  nameMap.Values['HoWRedguardElite'] := 'CC Redguard Elite';
  nameMap.Values['HoWSaS'] := 'CC Saints & Seducers';
  nameMap.Values['HoWSaturalia'] := 'CC Saturalia';
  nameMap.Values['HoWSpellKnightArmor'] := 'CC Spell Knight Armor';
  nameMap.Values['HoWStalhrimFur'] := 'CC Stalhrim Fur';
  nameMap.Values['HoWStaves'] := 'CC Staves';
  nameMap.Values['HoWSteelSoldier'] := 'CC Steel Soldier';
  nameMap.Values['HoWTheCause'] := 'CC Cause';
  nameMap.Values['HoWVigilEnforcerArmor'] := 'CC Vigil Enforcer Armor';
  nameMap.Values['HoWWildHorses'] := 'CC Wild Horses';
  nameMap.Values['HoWtBoS'] := 'CC Bow of Shadows';
  nameMap.Values['IBOMItems'] := 'Ice Blade of the Monarch';
  nameMap.Values['KRI_Xelzaz'] := 'Safehouse - Xelzaz';
  nameMap.Values['LibraryAutographs'] := 'Library - Autographs';
  nameMap.Values['LibraryLowerFloorLeft'] := 'Library - Lower Left';
  nameMap.Values['LibraryLowerFloorRight'] := 'Library - Lower Rigth';
  nameMap.Values['LibraryMaps'] := 'Library - Maps';
  nameMap.Values['LibraryMapsNTH'] := 'Library - New Treasure Maps';
  nameMap.Values['LibraryRareBooks'] := 'Library - Rare Books';
  nameMap.Values['LibraryTGCON'] := 'Library - Gray Cowl of Nocturnal';
  nameMap.Values['LibraryTHMaps'] := 'Library - Treasure Maps';
  nameMap.Values['LibraryUpperFloor'] := 'Library - Upper';
  nameMap.Values['MUSTARDJARItems'] := 'Badgremlin''s Mustard Jar';
  nameMap.Values['NSBGFariy'] := 'Natural Science - Badgremlin''s Fairies';
  nameMap.Values['NSBGFish'] := 'Natural Science - Badgremlin''s Fish';
  nameMap.Values['NSFossils'] := 'Natural Science - Fossils';
  nameMap.Values['NSGemstone'] := 'Natural Science - Gemstones';
  nameMap.Values['NSShells'] := 'Natural Science - Shells';
  nameMap.Values['NaturalScienceAnimals'] := 'Natural Science - Animals';
  nameMap.Values['SHAuri'] := 'Safehouse - Auri';
  nameMap.Values['SHHoth'] := 'Safehouse - Hoth';
  nameMap.Values['SHInigo'] := 'Safehouse - Inigo';
  nameMap.Values['SHKaidan'] := 'Safehouse - Kaidan';
  nameMap.Values['SHLucien'] := 'Safehouse - Lucien';
  nameMap.Values['SHRedcap'] := 'Safehouse - Redcap';
  nameMap.Values['SHRemiel'] := 'Safehouse - Remiel';
  nameMap.Values['SHVilja'] := 'Safehouse - Vilja';
  nameMap.Values['SUDs'] := 'Skyrim''s Unique Drinks';
  nameMap.Values['SafehouseCloaks'] := 'Safehouse - Cloaks';
  nameMap.Values['SafehouseDolls'] := 'Safehouse - Dolls';
  nameMap.Values['SafehouseShirley'] := 'Safehouse - Shirley';
  nameMap.Values['SafehouseSoapDisplay'] := 'Safehouse - Soaps';
  nameMap.Values['StoreRoomReserveVintages'] := 'Reserved Vintages';
  nameMap.Values['ToolStorage'] := 'Tool Storage';
  nameMap.Values['TWZItems'] := 'Zim''s Thane Weapons';
  nameMap.Values['YUMCheeseDisplay'] := 'Cheesemod';
end;

function DoStringReplace(const Input, Find, Replace : String) : String;
var
  P : Integer;
begin
  Result := Input;

  repeat
    P := Pos(Find, Result);
    if P > 0 then begin
      Delete(Result, P, Length(Find));
      Insert(Replace, Result, P);
    end;
  until P = 0;
end;

// adds separator before each instance of final consecutive capital letter
function MakeName(const Input : String) : String;
var
  P, inserted, wordLength : Integer;
  lastWasCap : Boolean;
  SEPARATOR : String;
begin
  Result := Input;
  SEPARATOR := ' ';
  lastWasCap := false;
  inserted := 0;
  wordLength := 0;
  for P := 1 to Length(Input) do begin
    if Input[P] = Upcase(Input[P]) then begin
      if not lastWasCap and wordLength > 0 then begin
        Insert(SEPARATOR, Result, P + inserted);
        inserted := inserted + 1;
        wordLength := 0;
      end;
      wordLength := wordLength + 1;
      lastWasCap := True;
    end else begin
      // not a capital letter - could be end of a sequence of caps
      if lastWasCap and wordLength > 1 then begin
        Insert(SEPARATOR, Result, P + inserted - 1);
        inserted := inserted + 1;
        wordLength := 0;
      end;
      wordLength := wordLength + 1;
      lastWasCap := False;
    end;
  end;
end;

function EncodeAsCollection(e: IInterface; edid, rawName: string): string;
var
  name: string;
begin
    formIds.Add(IntToHex(GetLoadOrderFormID(e), 8));
    buffer.Add(IntToHex(GetLoadOrderFormID(e), 8));
    if (notFirst) then begin
        result := DoStringReplace(collectionTemplate, '{OPTIONALCOMMA}', ',');
    end else begin
        notFirst := True;
        result := DoStringReplace(collectionTemplate, '{OPTIONALCOMMA}', '');
    end;
    
  name := nameMap.Values[rawName];
  if (name = '') then begin
      AddMessage('No mapping found for ' + rawName + ' from ' + GetFileName(e) + '! Report to add to script');
      name := MakeName(rawName);
  end

    result := DoStringReplace(result, '{NAME}', name);
    result := DoStringReplace(result, '{DESCRIPTION}', 'Display: ' + edid);
    result := DoStringReplace(result, '{PLUGIN}', GetFileName(e));
    result := DoStringReplace(result, '{FORMID}', '00' + copy(IntToHex(FixedFormID(e), 8), length(IntToHex(FixedFormID(e), 8))-5, 6));
  end;

function Process(e: IInterface): integer;
var
  edid, rawName: string;
  itemCount: integer;
begin
  if Signature(e) <> 'FLST' then
    Exit;
    
  if formIds.Indexof(IntToHex(GetLoadOrderFormID(e), 8)) < 0 then begin  
      // Whitelist LotD displays
      edid := GetEditValue(ElementBySignature(e, 'EDID'));
      
      itemCount := ElementCount(ElementByName(e, 'FormIDs'));
      
      if ((pos('KRI_', edid) = 1) or (pos('DBM_', edid) = 1)) and (pos('Item', edid) > 1) and (pos('Alt', edid) = 0) and (pos('ALT', edid) = 0) and (pos('Allt', edid) = 0) then begin
          rawName := '';
          
          if (pos('DBM_Section', edid) = 1) then begin
            AddMessage('LotD Display - ' + edid + ' - ' + GetFileName(e));
            rawName := DoStringReplace(edid, 'DBM_Section', '');
            rawName := DoStringReplace(rawName, 'Items', '');
            formLists.Add(EncodeAsCollection(e, edid, rawName));
          end
          else if (pos('DBM_HUB_Section', edid) = 1) then begin
            AddMessage('LotD HoF Display - ' + edid + ' - ' + GetFileName(e));
            rawName := DoStringReplace(edid, 'DBM_HUB_Section', '');
            rawName := DoStringReplace(rawName, 'Items', '');
            formLists.Add(EncodeAsCollection(e, edid, rawName));
          end
          else if (pos('DBM_CC_Section', edid) = 1) then begin
            AddMessage('LotD HoW Display - ' + edid + ' - ' + GetFileName(e));
            rawName := DoStringReplace(edid, 'DBM_CC_Section', '');
            rawName := DoStringReplace(rawName, 'Items', '');
            formLists.Add(EncodeAsCollection(e, edid, rawName));
          end
          // Exceptions
          else if (edid = 'DBM_YUMCheeseDisplayItems') then begin
            AddMessage('LotD Special Display - ' + edid + ' - ' + GetFileName(e));
            rawName := 'YUMCheeseDisplay';
            formLists.Add(EncodeAsCollection(e, edid, rawName));
          end
          else if (edid = 'DBM_AOSItems') then begin
            AddMessage('LotD Special Display - ' + edid + ' - ' + GetFileName(e));
            rawName := 'AOS';
            formLists.Add(EncodeAsCollection(e, edid, rawName));
          end
          else if (edid = 'DBM_DaedricGalleryIdentityCrisisItems') then begin
            AddMessage('LotD Special Display - ' + edid + ' - ' + GetFileName(e));
            rawName := 'DaedricGalleryIdentityCrisis';
            formLists.Add(EncodeAsCollection(e, edid, rawName));
          end
          else if (edid = 'DBM_RoomStoreroomMUSTARDJARItems') then begin
            AddMessage('LotD Special Display - ' + edid + ' - ' + GetFileName(e));
            rawName := 'MUSTARDJARItems';
            formLists.Add(EncodeAsCollection(e, edid, rawName));
          end
          else if (edid = 'DBM_CCAoC_SectionHoWItems') then begin
            AddMessage('LotD Special Display - ' + edid + ' - ' + GetFileName(e));
            rawName := 'CCAoC';
            formLists.Add(EncodeAsCollection(e, edid, rawName));
          end
          else if (edid = 'DBM_CC_NixHoundItems') then begin
            AddMessage('LotD Special Display - ' + edid + ' - ' + GetFileName(e));
            rawName := 'CC_NixHound';
            formLists.Add(EncodeAsCollection(e, edid, rawName));
          end
          else if (edid = 'DBM_CC_DwMudcrabItems') then begin
            AddMessage('LotD Special Display - ' + edid + ' - ' + GetFileName(e));
            rawName := 'CC_DwMudcrab';
            formLists.Add(EncodeAsCollection(e, edid, rawName));
          end
          else if (edid = 'DBM_TWZItems') then begin
            AddMessage('LotD Special Display - ' + edid + ' - ' + GetFileName(e));
            rawName := 'TWZItems';
            formLists.Add(EncodeAsCollection(e, edid, rawName));
          end
          else if (edid = 'DBM_AOBJewelryItems') then begin
            AddMessage('LotD Special Display - ' + edid + ' - ' + GetFileName(e));
            rawName := 'AOBJewelry';
            formLists.Add(EncodeAsCollection(e, edid, rawName));
          end
          else if (edid = 'DBM_IBOMItems') then begin
            AddMessage('LotD Special Display - ' + edid + ' - ' + GetFileName(e));
            rawName := 'IBOMItems';
            formLists.Add(EncodeAsCollection(e, edid, rawName));
          end
          else if (edid = 'KRI_ItemsList_Xelzaz') then begin
            AddMessage('LotD Special Display - ' + edid + ' - ' + GetFileName(e));
            rawName := 'KRI_Xelzaz';
            formLists.Add(EncodeAsCollection(e, edid, rawName));
          end;
          
          if (rawName = '') then begin
            // AddMessage('Did not process ' + edid + ' from ' + GetFileName(e) + '...');
          end;
      end;
                
      if (buffer.count = 128) then begin
        CreateJson();
      end;
  end;
end;

function CreateJson: string;
var
  fname: string;
begin
  AddMessage('Found 128 collections!');
  fname := ProgramPath + 'Edit Scripts\SHSE.Collections.LotD' + IntToStr(fileCount) + '.json';
  AddMessage('Saving list to ' + fname);
  formLists.Add(collectionPostscript);
  formLists.SaveToFile(fname);
  formLists.Free;
  buffer.Free;
 
  buffer := TStringList.Create;
  formLists := TStringList.Create;  
  formLists.Add(collectionPreamble);
  notFirst := False;
 
  fileCount := fileCount + 1;
end;

function Finalize: integer;
var
  fname: string;
begin
  fname := ProgramPath + 'Edit Scripts\SHSE.Collections.LotD' + IntToStr(fileCount) + '.json';
  AddMessage('Saving list to ' + fname);
  formLists.Add(collectionPostscript);
  formLists.SaveToFile(fname);
  formLists.Free;
  AddMessage('Found ' + IntToStr(formIds.Count) + ' collections in load order');
  formIds.Free;
  buffer.Free;
end;

end.