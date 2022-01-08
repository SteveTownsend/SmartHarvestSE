@if exist "S:\github\x64\release\Smart Harvest SE-x.x.x.x.rar" (del "S:\github\x64\release\Smart Harvest SE-x.x.x.x.rar")

@"C:\Program Files\WinRAR\rar.exe" a -ep -apSKSE/Plugins "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/x64/release/SmartHarvestSE.dll
@"C:\Program Files\WinRAR\rar.exe" a -ep -apSKSE/Plugins "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/Collections/Examples/SHSE.Collections.BlackList.json
@"C:\Program Files\WinRAR\rar.exe" a -ep -apSKSE/Plugins "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/Collections/Examples/SHSE.Collections.ManualLoot.json
@"C:\Program Files\WinRAR\rar.exe" a -ep -apSKSE/Plugins "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/Collections/Schema/SHSE.SchemaCollections.json
@"C:\Program Files\WinRAR\rar.exe" a -ep -apSKSE/Plugins "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/Filters/SHSE.SchemaFilters.json
@"C:\Program Files\WinRAR\rar.exe" a -ep -apSKSE/Plugins "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/Config/SmartHarvestSE.ini
@"C:\Program Files\WinRAR\rar.exe" a -ep -apSKSE/Plugins "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/Config/SmartHarvestSE.Defaults.ini

@"C:\Program Files\WinRAR\rar.exe" a -ep -ap"Collections/Edit Scripts" "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" "s:/github/SmartHarvestSE/Collections/Edit Scripts/SHSE Find Collectible FLSTs.pas"

@"C:\Program Files\WinRAR\rar.exe" a -ep -apCollections/Examples "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/Collections/Examples/SHSE.Collections.Hunterborn.json
@"C:\Program Files\WinRAR\rar.exe" a -ep -apCollections/Examples "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/Collections/Examples/SHSE.Collections.LoTD.json
@"C:\Program Files\WinRAR\rar.exe" a -ep -apCollections/Examples "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/Collections/Examples/SHSE.Collections.LoTD2.json
@"C:\Program Files\WinRAR\rar.exe" a -ep -apCollections/Examples "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/Collections/Examples/SHSE.Collections.Samples.json
@"C:\Program Files\WinRAR\rar.exe" a -ep -apCollections/Examples "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/Collections/Examples/SHSE.Collections.Uniques.json

@"C:\Program Files\WinRAR\rar.exe" a -ep -apCollections/JSONTest "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/x64/release/JSONTest.exe

@"C:\Program Files\WinRAR\rar.exe" a -ep -apFilters "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/Filters/SHSE.Filter.DeadBody.json

@"C:\Program Files\WinRAR\rar.exe" a -ep -apInterface/towawot "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/Interface/towawot/AutoHarvestSE.dds

@"C:\Program Files\WinRAR\rar.exe" a -ep -apInterface/translations "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/Interface/translations/SmartHarvestSE_*.txt

@"C:\Program Files\WinRAR\rar.exe" a -ep -apScripts/Source "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/Scripts/SHSE_*.psc

@"C:\Program Files\WinRAR\rar.exe" a -ep -apScripts "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/Scripts/compiled/SHSE_*.pex

@"C:\Program Files\WinRAR\rar.exe" a -ep -aptextures/effects/gradients "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/textures/effects/gradients/shse_*.dds
@"C:\Program Files\WinRAR\rar.exe" a -ep -aptextures/effects/gradients "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/textures/effects/gradients/brushedcopperbronze.dds
@"C:\Program Files\WinRAR\rar.exe" a -ep -aptextures/effects/gradients "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/textures/effects/gradients/brushedgold.dds
@"C:\Program Files\WinRAR\rar.exe" a -ep -aptextures/effects/gradients "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/textures/effects/gradients/brushedsilver.dds

@"C:\Program Files\WinRAR\rar.exe" a -ep "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/LICENSE
@"C:\Program Files\WinRAR\rar.exe" a -ep "s:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" s:/github/SmartHarvestSE/Plugin/SmartHarvestSE.esp
