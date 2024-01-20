@if exist "S:\github\x64\release\Smart Harvest SE-x.x.x.x.rar" (del "S:\github\x64\release\Smart Harvest SE-x.x.x.x.rar")

@"C:\Program Files\WinRAR\rar.exe" a -ep -apSKSE/Plugins "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/x64/release/SmartHarvestSE.dll
@"C:\Program Files\WinRAR\rar.exe" a -ep -apSKSE/Plugins "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/Collections/Examples/SHSE.Collections.BlackList.json
@"C:\Program Files\WinRAR\rar.exe" a -ep -apSKSE/Plugins "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/Collections/Examples/SHSE.Collections.ManualLoot.json
@"C:\Program Files\WinRAR\rar.exe" a -ep -apSKSE/Plugins "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/Collections/Schema/SHSE.SchemaCollections.json
@"C:\Program Files\WinRAR\rar.exe" a -ep -apSKSE/Plugins "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/Filters/SHSE.SchemaFilters.json
@"C:\Program Files\WinRAR\rar.exe" a -ep -apSKSE/Plugins "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/Config/SmartHarvestSE.ini
@"C:\Program Files\WinRAR\rar.exe" a -ep -apSKSE/Plugins "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/Config/SmartHarvestSE.Defaults.ini

@"C:\Program Files\WinRAR\rar.exe" a -ep -ap"Collections/Edit Scripts" "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" "j:/github/SmartHarvestSE/Collections/Edit Scripts/SHSE Find Collectible FLSTs.pas"

@"C:\Program Files\WinRAR\rar.exe" a -ep -apCollections/Examples "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/Collections/Examples/SHSE.Collections.Hunterborn.json
@"C:\Program Files\WinRAR\rar.exe" a -ep -apCollections/Examples "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/Collections/Examples/SHSE.Collections.LoTD.json
@"C:\Program Files\WinRAR\rar.exe" a -ep -apCollections/Examples "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/Collections/Examples/SHSE.Collections.LoTD2.json
@"C:\Program Files\WinRAR\rar.exe" a -ep -apCollections/Examples "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/Collections/Examples/SHSE.Collections.Samples.json
@"C:\Program Files\WinRAR\rar.exe" a -ep -apCollections/Examples "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/Collections/Examples/SHSE.Collections.Uniques.json

@"C:\Program Files\WinRAR\rar.exe" a -ep -apCollections/JSONTest "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/x64/release/JSONTest.exe

@"C:\Program Files\WinRAR\rar.exe" a -ep -apFilters "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/Filters/SHSE.Filter.DeadBody.json

@"C:\Program Files\WinRAR\rar.exe" a -ep -apInterface/towawot "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/Interface/towawot/AutoHarvestSE.dds

@"C:\Program Files\WinRAR\rar.exe" a -ep -apInterface/translations "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/Interface/translations/SmartHarvestSE_*.txt

@"C:\Program Files\WinRAR\rar.exe" a -ep -apScripts/Source "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/Scripts/SHSE_*.psc

@"C:\Program Files\WinRAR\rar.exe" a -ep -apScripts "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/Scripts/compiled/SHSE_*.pex

@"C:\Program Files\WinRAR\rar.exe" a -ep -aptextures/effects/gradients "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/textures/effects/gradients/shse_*.dds
@"C:\Program Files\WinRAR\rar.exe" a -ep -aptextures/effects/gradients "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/textures/effects/gradients/brushedcopperbronze.dds
@"C:\Program Files\WinRAR\rar.exe" a -ep -aptextures/effects/gradients "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/textures/effects/gradients/brushedgold.dds
@"C:\Program Files\WinRAR\rar.exe" a -ep -aptextures/effects/gradients "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/textures/effects/gradients/brushedsilver.dds

@"C:\Program Files\WinRAR\rar.exe" a -ep "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/LICENSE
@"C:\Program Files\WinRAR\rar.exe" a -ep "j:/github/x64/release/Smart Harvest SE-x.x.x.x.rar" j:/github/SmartHarvestSE/Plugin/SmartHarvestSE.esp
