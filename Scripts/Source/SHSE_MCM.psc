Scriptname SHSE_MCM extends SKI_ConfigBase  

Import SHSE_PluginProxy

SHSE_EventsAlias Property eventScript Auto
GlobalVariable Property g_LootingEnabled Auto
Spell AdventurersInstinctPower
Spell FortuneHuntersInstinctPower

; check for first init for this playthrough
GlobalVariable Property g_InitComplete Auto

int type_Common = 1
int type_Harvest = 2
int type_Config = 1
int type_ItemObject = 2
int type_Container = 3
int type_Deadbody = 4
int type_ValueWeight = 5
int type_Glow = 6
int type_Handling
int type_MaxWeight
int type_MaxItems

; object types must be in sync with the native DLL
int objType_Flora
int objType_Critter
int objType_Septim
int objType_Key
int objType_Soulgem
int objType_Gem
int objType_Ingredient
int objType_Weapon
int objType_Armor
int objType_Jewelry
int objType_LockPick
int objType_Ammo
int objType_Mine
int objType_Clutter
int objType_Potion
int objType_Poison
int objType_Scroll
int objType_Food
int objType_Drink

bool enableHarvest
bool enableLootContainer
int enableLootDeadbody
string[] s_lootDeadBodyArray
int excludeNPCAll
int excludeNPCArmour
int excludeNPCNone
int excludeNPCUnderwear

bool unencumberedInCombat
bool unencumberedInPlayerHome
bool unencumberedIfWeaponDrawn

int pauseHotkeyCode
int whiteListHotkeyCode
int blackListHotkeyCode

int preventPopulationCenterLooting
string[] s_populationCenterArray
bool notifyLocationChange

int defaultRadius
float defaultInterval
int defaultRadiusIndoors
float defaultIntervalIndoors
int radius
float interval
int radiusIndoors
float intervalIndoors

float defaultVerticalRadiusFactor
float verticalRadiusFactor
int defaultDoorsPreventLooting
int doorsPreventLooting
bool lootAllowedItemsInSettlement
bool lootAllowedItemsInPlayerHouse

int iniSaveLoad
string[] s_iniSaveLoadArray
int crimeCheckNotSneaking
string[] s_crimeCheckNotSneakingArray
int crimeCheckSneaking
string[] s_crimeCheckSneakingArray
int playerBelongingsLoot
string[] s_specialObjectHandlingArray
string[] s_lockedContainerHandlingArray
string[] s_enchantedObjectHandlingArray
string[] s_questObjectHandlingArray
string[] s_behaviorToggleArray
string[] s_excessDisposalArray
; handle mapping between compact UI list of transfer options and sparse plugin list
int[] s_excessDisposalEnumToChoice
int[] s_excessDisposalChoiceToEnum
int excessDisposalOptions
int excessDisposalSell
int saleValuePercent
bool handleExcessCraftingItems
int craftingItemsExcessHandling
int craftingItemsExcessCount
float craftingItemsExcessWeight
int playContainerAnimation
string[] s_playContainerAnimationArray

int questObjectLoot
int lockedChestLoot
int bossChestLoot
int enchantedItemLoot
bool unknownIngredientLoot

bool manualLootTargetNotify
bool whiteListTargetNotify

bool disableDuringCombat
bool disableWhileWeaponIsDrawn
bool disableWhileConcealed

int[] id_objectSettingArray
float[] objectSettingArray

int valueWeightDefault
int valueWeightDefaultDefault
int maxMiningItems
int maxMiningItemsDefault
bool miningToolsRequired
bool miningToolsRequiredDefault
bool disallowMiningIfSneaking
bool disallowMiningIfSneakingDefault

int excessLeave
int excessSell

int[] id_excessHandlingArray
float[] excessHandlingArray
int[] id_excessCountArray
float[] excessCountArray
int[] id_excessWeightArray
float[] excessWeightArray

int valuableItemLoot
int valuableItemThreshold

; Global
bool collectionsEnabled
int collectionGroupCount
; Group cursor within Collection Group universe
int collectionGroup
string[] collectionGroupNames
string[] collectionGroupFiles
int collectionCount
; Collection cursor within Group
int collectionIndex
string[] collectionNames
string[] collectionDescriptions
string lastKnownPolicy
; Current Collection state
string[] s_collectibleActions
int collectibleAction
bool collectionAddNotify
bool collectDuplicates
int collectionTotal
int collectionObtained
string collectionStatus

; Current Collection Group state
int groupCollectibleAction
bool groupCollectionAddNotify
bool groupCollectDuplicates

bool adventuresEnabled
int adventureTypeCount
; cursor within Adventure Target universe
int adventureType
string[] adventureTypeNames
int worldCount
int worldIndex
string[] worldNames
bool adventureActive

bool fortuneHuntingEnabled
bool fortuneHuntItem
bool fortuneHuntNPC
bool fortuneHuntContainer
bool unlockGlowColours

bool checkWeightlessValue
bool checkWeightlessValueDefault
int weightlessMinimumValue
int weightlessMinimumValueDefault
int[] id_valueWeightArray
float[] valueWeightSettingArray

String[] s_behaviorArray
String[] s_ammoBehaviorArray
String[] s_harvestBehaviorArray
String[] s_objectTypeNameArray

int[] id_whiteList_array
Form[] whiteList_form_array
int whiteListEntries
String[] whiteList_name_array
bool[] whiteList_flag_array

int[] id_blackList_array
Form[] blackList_form_array
int blackListEntries
String[] blackList_name_array
bool[] blackList_flag_array

int[] id_transferList_array
Form[] transferList_form_array
int transferListEntries
String[] transferList_name_array
int[] transferList_index_array
bool[] transferList_flag_array

int[] id_glowReasonArray
int[] glowReasonSettingArray
String[] s_glowReasonArray
String[] s_shaderColourArray

int sagaDayCount
int currentSagaDay
string currentSagaDayName
int currentSagaDayPage
int sagaDayPageCount

Actor thisPlayer
bool logMCM

int Function CycleInt(int num, int max)
    int result = num + 1
    if (result >= max)
        result = 0
    endif
    return result
endFunction

; handles cross-reference from compact MCM list to sparse plugin list
; num is an enum value - we need to cycle though the other choices
int Function CycleExcess(int num, int max)
    ; find the choice that was used to select this enum
    int result = s_excessDisposalEnumToChoice[num]
    result += 1
    if result >= max
        result = 0
    endif
    return s_excessDisposalChoiceToEnum[result]
endFunction

function updateMaxMiningItems(int maxItems)
    maxMiningItems = maxItems
    eventScript.updateMaxMiningItems(maxItems)
endFunction

function updateMiningToolsRequired(bool toolsRequired)
    miningToolsRequired = toolsRequired
    eventScript.updateMiningToolsRequired(toolsRequired)
endFunction

function updateDisallowMiningIfSneaking(bool noSneakyMining)
    disallowMiningIfSneaking = noSneakyMining
    eventScript.updateDisallowMiningIfSneaking(noSneakyMining)
endFunction

float[] function GetSettingToObjectArray(int section1, int section2)
    int index = 0
    int objType = 1
    float[] result = New float[30]
    while (index < 30)
        ; offset by 1 to omit unknown = 0
        result[index] = GetSettingObjectArrayEntry(section1, section2, objType)
        ;DebugTrace("Config setting " + section1 + "/" + section2 + "/" + index + " = " + result[index])
        index += 1
        objType += 1
    endWhile
    return result
endFunction

Function PutSettingObjectArray(int section_first, int section_second, int listLength, float[] values)
    int index = 0
    int objType = 1
    while index < listLength
        ; offset by 1 to omit unknown = 0
        PutSettingObjectArrayEntry(section_first, section_second, objType, values[index])
        index += 1
        objType += 1
    endWhile
EndFunction

float[] function GetSettingToExcessHandlingArray(int section1, int section2)
    int index = 0
    ; offset by 1 to omit unknown = 0
    int objType = 1
    float[] result = New float[30]
    while (index < 30)
        if SupportsExcessHandling(objType)
            float setting = GetSettingObjectArrayEntry(section1, section2, objType)
            ; 64 containers, plus leave/sell/no-limit - 0 based
            if setting > (2 + 64) as float
                AlwaysTrace("Out of range excess handling " + setting + " for objType" + objType)
                ; references unknown transfer target - pick highest valid value
                setting = (2 + 64) as float
            endif
            result[index] = setting
        else
            result[index] = 0.0
        endif
        ;DebugTrace("Config setting " + section1 + "/" + section2 + "/" + index + " = " + result[index])
        index += 1
        objType += 1
    endWhile
    return result
endFunction

Function PutSettingToExcessHandlingArray(int section_first, int section_second, int listLength, float[] values)
    int index = 0
    ; offset by 1 to omit unknown = 0
    int objType = 1
    while index < listLength
        if SupportsExcessHandling(objType)
            PutSettingObjectArrayEntry(section_first, section_second, objType, values[index])
        else
            PutSettingObjectArrayEntry(section_first, section_second, objType, 0.0)
        endif
        index += 1
        objType += 1
    endWhile
EndFunction

int[] function GetSettingToGlowArray(int section1, int section2)
    int index = 0
    int[] result = New Int[8]
    while (index < 8)
        result[index] = GetSettingGlowArrayEntry(section1, section2, index)
        ;DebugTrace("Glow Config setting " + section1 + "/" + section2 + "/" + index + " = " + result[index])
        index += 1
    endWhile
    return result
endFunction

Function PutSettingGlowArray(int section_first, int section_second, int listLength, int[] values)
    int index = 0
    while index < listLength
        PutSettingGlowArrayEntry(section_first, section_second, index, values[index])
        index += 1
    endWhile
EndFunction

function LoadSettingsFromNative()
    enableHarvest = GetSetting(type_Common, type_Config, "EnableHarvest") as bool
    enableLootContainer = GetSetting(type_Common, type_Config, "EnableLootContainer") as bool
    enableLootDeadbody = GetSetting(type_Common, type_Config, "EnableLootDeadbody") as int
    unencumberedInCombat = GetSetting(type_Common, type_Config, "UnencumberedInCombat") as bool
    unencumberedInPlayerHome = GetSetting(type_Common, type_Config, "UnencumberedInPlayerHome") as bool
    unencumberedIfWeaponDrawn = GetSetting(type_Common, type_Config, "UnencumberedIfWeaponDrawn") as bool
    pauseHotkeyCode = GetSetting(type_Common, type_Config, "PauseHotkeyCode") as int
    eventScript.SyncPauseKey(pauseHotkeyCode)
    whiteListHotkeyCode = GetSetting(type_Common, type_Config, "WhiteListHotkeyCode") as int
    eventScript.SyncWhiteListKey(whiteListHotkeyCode)
    blackListHotkeyCode = GetSetting(type_Common, type_Config, "BlackListHotkeyCode") as int
    eventScript.SyncBlackListKey(blackListHotkeyCode)
    preventPopulationCenterLooting = GetSetting(type_Common, type_Config, "PreventPopulationCenterLooting") as int
    notifyLocationChange = GetSetting(type_Common, type_Config, "NotifyLocationChange") as bool

    radius = GetSetting(type_Harvest, type_Config, "RadiusFeet") as int
    interval = GetSetting(type_Harvest, type_Config, "IntervalSeconds")
    radiusIndoors = GetSetting(type_Harvest, type_Config, "IndoorsRadiusFeet") as int
    intervalIndoors = GetSetting(type_Harvest, type_Config, "IndoorsIntervalSeconds")

    disableDuringCombat = GetSetting(type_Harvest, type_Config, "DisableDuringCombat") as bool
    disableWhileWeaponIsDrawn = GetSetting(type_Harvest, type_Config, "DisableWhileWeaponIsDrawn") as bool
    disableWhileConcealed = GetSetting(type_Harvest, type_Config, "DisableWhileConcealed") as bool

    crimeCheckNotSneaking = GetSetting(type_Harvest, type_Config, "CrimeCheckNotSneaking") as int
    crimeCheckSneaking = GetSetting(type_Harvest, type_Config, "CrimeCheckSneaking") as int

    questObjectLoot = GetSetting(type_Harvest, type_Config, "QuestObjectLoot") as int
    lockedChestLoot = GetSetting(type_Harvest, type_Config, "LockedChestLoot") as int
    bossChestLoot = GetSetting(type_Harvest, type_Config, "BossChestLoot") as int
    enchantedItemLoot = GetSetting(type_Harvest, type_Config, "EnchantedItemLoot") as int
    unknownIngredientLoot = GetSetting(type_Harvest, type_Config, "UnknownIngredientLoot") as bool
    valuableItemLoot = GetSetting(type_Harvest, type_Config, "valuableItemLoot") as int
    valuableItemThreshold = GetSetting(type_Harvest, type_Config, "ValuableItemThreshold") as int
    playerBelongingsLoot = GetSetting(type_Harvest, type_Config, "PlayerBelongingsLoot") as int
    playContainerAnimation = GetSetting(type_Harvest, type_Config, "PlayContainerAnimation") as int

    manualLootTargetNotify = GetSetting(type_Harvest, type_Config, "ManualLootTargetNotify") as bool
    whiteListTargetNotify = GetSetting(type_Harvest, type_Config, "WhiteListTargetNotify") as bool
    valueWeightDefault = GetSetting(type_Harvest, type_Config, "ValueWeightDefault") as int
    updateMaxMiningItems(GetSetting(type_Harvest, type_Config, "MaxMiningItems") as int)
    updateMiningToolsRequired(GetSetting(type_Harvest, type_Config, "MiningToolsRequired") as bool)
    updateDisallowMiningIfSneaking(GetSetting(type_Harvest, type_Config, "DisallowMiningIfSneaking") as bool)
    saleValuePercent = GetSetting(type_Harvest, type_Config, "SaleValuePercent") as int

    verticalRadiusFactor = GetSetting(type_Harvest, type_Config, "VerticalRadiusFactor")
    doorsPreventLooting = GetSetting(type_Harvest, type_Config, "DoorsPreventLooting") as int
    lootAllowedItemsInSettlement = GetSetting(type_Harvest, type_Config, "LootAllowedItemsInSettlement") as bool
    lootAllowedItemsInPlayerHouse = GetSetting(type_Harvest, type_Config, "LootAllowedItemsInPlayerHouse") as bool

    objectSettingArray = GetSettingToObjectArray(type_Harvest, type_ItemObject)
    checkWeightlessValue = GetSetting(type_Harvest, type_ValueWeight, "CheckWeightlessValue") as bool
    weightlessMinimumValue = GetSetting(type_Harvest, type_ValueWeight, "WeightlessMinimumValue") as int
    valueWeightSettingArray = GetSettingToObjectArray(type_Harvest, type_ValueWeight)
    excessHandlingArray = GetSettingToExcessHandlingArray(type_Harvest, type_Handling)
    excessCountArray = GetSettingToExcessHandlingArray(type_Harvest, type_MaxItems)
    excessWeightArray = GetSettingToExcessHandlingArray(type_Harvest, type_MaxWeight)

    handleExcessCraftingItems = GetSetting(type_Harvest, type_Config, "HandleExcessCraftingItems") as bool
    craftingItemsExcessHandling = GetSetting(type_Harvest, type_Config, "CraftingItemsExcessHandling") as int
    craftingItemsExcessCount = GetSetting(type_Harvest, type_Config, "CraftingItemsExcessCount") as int
    craftingItemsExcessWeight = GetSetting(type_Harvest, type_Config, "CraftingItemsExcessWeight") as float

    collectionsEnabled = GetSetting(type_Common, type_Config, "CollectionsEnabled") as bool
    ; Adventures are linked to a Lesser Power that needs to be enabled if settings so indicate
    adventuresEnabled = GetSetting(type_Common, type_Config, "AdventuresEnabled") as bool
    CheckAdventuresPower()
    fortuneHuntingEnabled = GetSetting(type_Common, type_Config, "FortuneHuntingEnabled") as bool
    fortuneHuntItem = GetSetting(type_Common, type_Config, "FortuneHuntItem") as bool
    fortuneHuntNPC = GetSetting(type_Common, type_Config, "FortuneHuntNPC") as bool
    fortuneHuntContainer = GetSetting(type_Common, type_Config, "FortuneHuntContainer") as bool
    CheckFortunePower()
    unlockGlowColours = GetSetting(type_Common, type_Config, "UnlockGlowColours") as bool
    glowReasonSettingArray = GetSettingToGlowArray(type_Common, type_Glow)
endFunction

;Seed defaults from the INI file, first time only - not repeated when user starts new game
function CheckFirstTimeEver()
    ;DebugTrace("CheckFirstTimeEver start")
    int doneInit = g_InitComplete.GetValue() as int
    if doneInit == 0
        ;DebugTrace("CheckFirstTimeEver - init required")
        AllocateItemCategoryArrays()
        AllocateExcessHandlingArrays()

        LoadIniFile(True)
        LoadSettingsFromNative()

        g_InitComplete.SetValue(1)
    endif
    ;DebugTrace("FirstTimeEver finished")
endFunction

bool Function ManagesCarryWeight()
    return unencumberedInCombat || unencumberedInPlayerHome || unencumberedIfWeaponDrawn
endFunction

Function SaveSettingsToNative()
    PutSetting(type_Common, type_Config, "EnableHarvest", enableHarvest as float)
    PutSetting(type_Common, type_Config, "EnableLootContainer", enableLootContainer as float)
    PutSetting(type_Common, type_Config, "EnableLootDeadbody", enableLootDeadbody as float)
    PutSetting(type_Common, type_Config, "UnencumberedInCombat", unencumberedInCombat as float)
    PutSetting(type_Common, type_Config, "UnencumberedInPlayerHome", unencumberedInPlayerHome as float)
    PutSetting(type_Common, type_Config, "UnencumberedIfWeaponDrawn", unencumberedIfWeaponDrawn as float)

    PutSetting(type_Common, type_Config, "PauseHotkeyCode", pauseHotkeyCode as float)
    PutSetting(type_Common, type_Config, "WhiteListHotkeyCode", whiteListHotkeyCode as float)
    PutSetting(type_Common, type_Config, "BlackListHotkeyCode", blackListHotkeyCode as float)
    PutSetting(type_Common, type_Config, "PreventPopulationCenterLooting", preventPopulationCenterLooting as float)
    PutSetting(type_Common, type_Config, "NotifyLocationChange", notifyLocationChange as float)

    PutSetting(type_Harvest, type_Config, "RadiusFeet", radius as float)
    PutSetting(type_Harvest, type_Config, "IntervalSeconds", interval)
    PutSetting(type_Harvest, type_Config, "IndoorsRadiusFeet", radiusIndoors as float)
    PutSetting(type_Harvest, type_Config, "IndoorsIntervalSeconds", intervalIndoors)

    PutSetting(type_Harvest, type_Config, "CrimeCheckNotSneaking", crimeCheckNotSneaking as float)
    PutSetting(type_Harvest, type_Config, "CrimeCheckSneaking", crimeCheckSneaking as float)
    PutSetting(type_Harvest, type_Config, "PlayerBelongingsLoot", playerBelongingsLoot as float)
    PutSetting(type_Harvest, type_Config, "PlayContainerAnimation", playContainerAnimation as float)

    PutSetting(type_Harvest, type_Config, "QuestObjectLoot", questObjectLoot as float)
    PutSetting(type_Harvest, type_Config, "EnchantedItemLoot", enchantedItemLoot as float)
    PutSetting(type_Harvest, type_Config, "UnknownIngredientLoot", unknownIngredientLoot as float)
    PutSetting(type_Harvest, type_Config, "valuableItemLoot", valuableItemLoot as float)
    PutSetting(type_Harvest, type_Config, "ValuableItemThreshold", valuableItemThreshold as float)
    PutSetting(type_Harvest, type_Config, "LockedChestLoot", lockedChestLoot as float)
    PutSetting(type_Harvest, type_Config, "BossChestLoot", bossChestLoot as float)
    PutSetting(type_Harvest, type_Config, "ManualLootTargetNotify", manualLootTargetNotify as float)
    PutSetting(type_Harvest, type_Config, "WhiteListTargetNotify", whiteListTargetNotify as float)

    PutSetting(type_Harvest, type_Config, "DisableDuringCombat", disableDuringCombat as float)
    PutSetting(type_Harvest, type_Config, "DisableWhileWeaponIsDrawn", disableWhileWeaponIsDrawn as float)
    PutSetting(type_Harvest, type_Config, "DisableWhileConcealed", disableWhileConcealed as float)

    PutSetting(type_Harvest, type_Config, "VerticalRadiusFactor", verticalRadiusFactor)
    PutSetting(type_Harvest, type_Config, "DoorsPreventLooting", doorsPreventLooting as float)
    PutSetting(type_Harvest, type_Config, "LootAllowedItemsInSettlement", lootAllowedItemsInSettlement as float)
    PutSetting(type_Harvest, type_Config, "LootAllowedItemsInPlayerHouse", lootAllowedItemsInPlayerHouse as float)

    PutSettingObjectArray(type_Harvest, type_ItemObject, 30, objectSettingArray)
    PutSettingToExcessHandlingArray(type_Harvest, type_Handling, 30, excessHandlingArray)
    PutSettingToExcessHandlingArray(type_Harvest, type_MaxItems, 30, excessCountArray)
    PutSettingToExcessHandlingArray(type_Harvest, type_MaxWeight, 30, excessWeightArray)

    PutSetting(type_Harvest, type_Config, "HandleExcessCraftingItems", handleExcessCraftingItems as float)
    PutSetting(type_Harvest, type_Config, "CraftingItemsExcessHandling", craftingItemsExcessHandling as float)
    PutSetting(type_Harvest, type_Config, "CraftingItemsExcessCount", craftingItemsExcessCount as float)
    PutSetting(type_Harvest, type_Config, "CraftingItemsExcessWeight", craftingItemsExcessWeight as float)

    PutSetting(type_Harvest, type_Config, "ValueWeightDefault", valueWeightDefault as float)
    PutSetting(type_Harvest, type_Config, "SaleValuePercent", saleValuePercent as float)
    PutSetting(type_Harvest, type_Config, "MaxMiningItems", maxMiningItems as float)
    PutSetting(type_Harvest, type_Config, "MiningToolsRequired", miningToolsRequired as float)
    PutSetting(type_Harvest, type_Config, "DisallowMiningIfSneaking", disallowMiningIfSneaking as float)
    PutSetting(type_Harvest, type_ValueWeight, "CheckWeightlessValue", checkWeightlessValue as float)
    PutSetting(type_Harvest, type_ValueWeight, "WeightlessMinimumValue", weightlessMinimumValue as float)
    PutSettingObjectArray(type_Harvest, type_ValueWeight, 30, valueWeightSettingArray)

    PutSetting(type_Common, type_Config, "CollectionsEnabled", collectionsEnabled as float)
    PutSetting(type_Common, type_Config, "AdventuresEnabled", adventuresEnabled as float)
    PutSetting(type_Common, type_Config, "FortuneHuntingEnabled", fortuneHuntingEnabled as float)
    PutSetting(type_Common, type_Config, "FortuneHuntItem", fortuneHuntItem as float)
    PutSetting(type_Common, type_Config, "FortuneHuntNPC", fortuneHuntNPC as float)
    PutSetting(type_Common, type_Config, "FortuneHuntContainer", fortuneHuntContainer as float)
    PutSetting(type_Common, type_Config, "UnlockGlowColours", unlockGlowColours as float)
    PutSettingGlowArray(type_Common, type_Glow, 8, glowReasonSettingArray)
endFunction

; push current settings to plugin and event handler script
Function ApplySetting()
    ;DebugTrace("  MCM ApplySetting start")
    SaveSettingsToNative()
    ; seed looting scan enabled according to configured settings
    bool isEnabled = enableHarvest || enableLootContainer || enableLootDeadbody > 0 || unencumberedInCombat || unencumberedInPlayerHome || unencumberedIfWeaponDrawn || collectionsEnabled || adventuresEnabled
    ;DebugTrace("isEnabled? " + isEnabled + "from flags:" + enableHarvest + " " + enableLootContainer + " " + enableLootDeadbody + " " + unencumberedInCombat + " " + unencumberedInPlayerHome + " " + unencumberedIfWeaponDrawn)
    ;DebugTrace("Collections enabled? " + " collectionsEnabled + ", Adventures enabled? " + " adventuresEnabled)
    if isEnabled
        g_LootingEnabled.SetValue(1)
    else
        g_LootingEnabled.SetValue(0)
    endif
    eventScript.PushScanActive()

    ; correct for any weight adjustments saved into this file, plugin will reinstate if/as needed
    ; Do this before plugin becomes aware of player home list
    logMCM = LoggingEnabled()
    thisPlayer = Game.GetPlayer()
    eventScript.Prepare(thisPlayer, logMCM)
    ; Only adjust weight if we are in any way responsible for it
    if ManagesCarryWeight()
        eventScript.RemoveCarryWeightDelta()
    endIf
    eventScript.ApplySetting()
    eventScript.SyncShaders(glowReasonSettingArray)

    ; do this last so plugin state is in sync   
    if (isEnabled)
        Debug.Notification(Replace(GetTranslation("$SHSE_ENABLE"), "{VERSION}", GetPluginVersion()))
        AllowSearch()
    Else
        Debug.Notification(Replace(GetTranslation("$SHSE_DISABLE"), "{VERSION}", GetPluginVersion()))
        DisallowSearch()
    EndIf

    ;DebugTrace("  MCM ApplySetting finished")
endFunction

Function SetOreVeinChoices()
    s_behaviorToggleArray = New String[3]
    s_behaviorToggleArray[0] = "$SHSE_DONT_PICK_UP"
    s_behaviorToggleArray[1] = "$SHSE_PICK_UP_IF_NOT_BYOH"
    s_behaviorToggleArray[2] = "$SHSE_PICK_UP"
EndFunction

Function DefaultExcessDisposalChoices()
    ; No Action/Leave/Sell/Container Max (64)
    s_excessDisposalArray = New String[67]
    s_excessDisposalArray[0] = "$SHSE_NO_LIMITS"
    s_excessDisposalArray[1] = "$SHSE_LEAVE_BEHIND"
    s_excessDisposalArray[2] = "$SHSE_AUTO_SELL"
    excessDisposalOptions = 3
    excessDisposalSell = 2
EndFunction

Function AugmentExcessDisposalChoices()
    ; Leave/Sell/Container - max length 3 + 64
    int index = 0
    int choice = 3
    
    ; mappings of fixed entries, augmented by sparse array xref
    s_excessDisposalEnumToChoice = Utility.CreateIntArray(3 + 64, -1)
    s_excessDisposalEnumToChoice[0] = 0
    s_excessDisposalEnumToChoice[1] = 1
    s_excessDisposalEnumToChoice[2] = 2

    s_excessDisposalChoiceToEnum = Utility.CreateIntArray(3 + transferListEntries)
    s_excessDisposalChoiceToEnum[0] = 0
    s_excessDisposalChoiceToEnum[1] = 1
    s_excessDisposalChoiceToEnum[2] = 2

    while index < transferListEntries
        s_excessDisposalArray[choice] = transferList_name_array[index]
        s_excessDisposalEnumToChoice[3 + transferList_index_array[index]] = choice
        s_excessDisposalChoiceToEnum[choice] = 3 + transferList_index_array[index]
        index += 1
        choice += 1
    endWhile
    excessDisposalOptions = choice
EndFunction

Function SetDeadBodyChoices()
    DebugTrace("SetDeadBodyChoices called")
    excludeNPCAll = 0
    excludeNPCArmour = 1
    excludeNPCNone = 2
    excludeNPCUnderwear = 3

    s_lootDeadBodyArray = New String[4]
    ; values match the C++ enumeration and INI file
    s_lootDeadBodyArray[excludeNPCAll] = "$SHSE_DONT_PICK_UP"
    s_lootDeadBodyArray[excludeNPCArmour] = "$SHSE_EXCLUDING_ARMOR"
    s_lootDeadBodyArray[excludeNPCUnderwear] = "$SHSE_EXCLUDING_UNDERWEAR"
    s_lootDeadBodyArray[excludeNPCNone] = "$SHSE_PICK_UP"
EndFunction

Function AllocateItemCategoryArrays()
    id_objectSettingArray = New Int[30]
    objectSettingArray = New float[30]

    id_valueWeightArray = New Int[30]
    valueWeightSettingArray = New float[30]
EndFunction

Function AllocateExcessHandlingArrays()
    id_excessHandlingArray = Utility.CreateIntArray(30)
    excessHandlingArray = Utility.CreateFloatArray(30)

    id_excessCountArray = Utility.CreateIntArray(30)
    excessCountArray = Utility.CreateFloatArray(30)

    id_excessWeightArray = Utility.CreateIntArray(30)
    excessWeightArray = Utility.CreateFloatArray(30)
EndFunction

Function CheckItemCategoryArrays()
    int doneInit = g_InitComplete.GetValue() as int
    if doneInit != 0
        ; arrays should all be in place, if not it's probably a bad save due to now-fixed bug
        if !id_objectSettingArray
            ;DebugTrace("allocate missing id_objectSettingArray")
            id_objectSettingArray = New Int[30]
        endif
        if !objectSettingArray
            ;DebugTrace("allocate missing objectSettingArray")
            objectSettingArray = New float[30]
        endif
        if !id_valueWeightArray
            ;DebugTrace("allocate missing id_valueWeightArray")
            id_valueWeightArray = New Int[30]
        endif
        if !valueWeightSettingArray
            ;DebugTrace("allocate missing valueWeightSettingArray")
            valueWeightSettingArray = New float[30]
        endif
    endIf
EndFunction

Function SetObjectTypeData()
    s_objectTypeNameArray = New String[30]

    s_objectTypeNameArray[0]  = "$SHSE_FLORA"
    s_objectTypeNameArray[1]  = "$SHSE_CRITTER"
    s_objectTypeNameArray[2]  = "$SHSE_INGREDIENT"
    s_objectTypeNameArray[3]  = "$SHSE_SEPTIM"
    s_objectTypeNameArray[4]  = "$SHSE_GEM"
    s_objectTypeNameArray[5]  = "$SHSE_LOCKPICK"
    s_objectTypeNameArray[6]  = "$SHSE_ANIMAL_HIDE"
    s_objectTypeNameArray[7]  = "$SHSE_OREINGOT"
    s_objectTypeNameArray[8]  = "$SHSE_SOULGEM"
    s_objectTypeNameArray[9]  = "$SHSE_KEY"
    s_objectTypeNameArray[10] = "$SHSE_CLUTTER"
    s_objectTypeNameArray[11] = "$SHSE_BOOK"
    s_objectTypeNameArray[12] = "$SHSE_SPELLBOOK"
    s_objectTypeNameArray[13] = "$SHSE_SKILLBOOK"
    s_objectTypeNameArray[14] = "$SHSE_BOOK_READ"
    s_objectTypeNameArray[15] = "$SHSE_SPELLBOOK_READ"
    s_objectTypeNameArray[16] = "$SHSE_SKILLBOOK_READ"
    s_objectTypeNameArray[17] = "$SHSE_SCROLL"
    s_objectTypeNameArray[18] = "$SHSE_AMMO"
    s_objectTypeNameArray[19] = "$SHSE_WEAPON"
    s_objectTypeNameArray[20] = "$SHSE_ENCHANTED_WEAPON"
    s_objectTypeNameArray[21] = "$SHSE_ARMOR"
    s_objectTypeNameArray[22] = "$SHSE_ENCHANTED_ARMOR"
    s_objectTypeNameArray[23] = "$SHSE_JEWELRY"
    s_objectTypeNameArray[24] = "$SHSE_ENCHANTED_JEWELRY"
    s_objectTypeNameArray[25] = "$SHSE_POTION"
    s_objectTypeNameArray[26] = "$SHSE_POISON"
    s_objectTypeNameArray[27] = "$SHSE_FOOD"
    s_objectTypeNameArray[28] = "$SHSE_DRINK"
    s_objectTypeNameArray[29] = "$SHSE_OREVEIN"
EndFunction

Function SetCheckWeightless()
    checkWeightlessValueDefault = false
    checkWeightlessValue = checkWeightlessValueDefault
    weightlessMinimumValueDefault = 10
    weightlessMinimumValue = weightlessMinimumValueDefault
EndFunction

Function SetMiscDefaults(bool firstTime)
    ; New or clarified defaults and constants
    manualLootTargetNotify = true
    whiteListTargetNotify = false

    defaultRadius = 30
    defaultInterval = 1.0
    defaultRadiusIndoors = 15
    defaultIntervalIndoors = 0.5
    if firstTime
        radius = defaultRadius
        interval = defaultInterval
        radiusIndoors = defaultRadiusIndoors
        intervalIndoors = defaultIntervalIndoors
        playContainerAnimation = 2
    endIf        

    valueWeightDefaultDefault = 10

    maxMiningItemsDefault = 8
    miningToolsRequiredDefault = false
    disallowMiningIfSneakingDefault = false
    updateMaxMiningItems(maxMiningItemsDefault)
    updateMiningToolsRequired(miningToolsRequiredDefault)
    updateDisallowMiningIfSneaking(disallowMiningIfSneakingDefault)

    notifyLocationChange = false
    enchantedItemLoot = 1
    unknownIngredientLoot = false
    valuableItemLoot = 1
    valuableItemThreshold = 500
    lootAllowedItemsInSettlement = true
    lootAllowedItemsInPlayerHouse = false

    SetCheckWeightless()

    InstallCollections()
    InstallCollectionGroupPolicy()
    InstallCollectionDescriptionsActions()
    InstallAdventures()
    InstallAdventuresPower()
    InstallFlexibleShaders()
    InstallQuestObjectHandling()
    InstallSagaRendering()
    MigrateFromFormLists()
    InitExcessInventoryHandling()
    InitFortuneHunting()
    InitExcessCraftingItems()
EndFunction

Function InstallCollections()
    collectionsEnabled = false
    collectionGroupNames = new String[128]
    collectionGroupFiles = new String[128]
    collectionGroupCount = 0
    collectionGroup = 0
    collectionNames = new String[128]
    collectionCount = 0
    collectionIndex = 0
    lastKnownPolicy = ""
EndFunction

Function InstallCollectionGroupPolicy()
    ; context-dependent, settings for Collection indexed by collectionGroup/collectionIndex
    groupCollectibleAction = 2
    groupCollectionAddNotify = true
    groupCollectDuplicates = false
EndFunction

Function InstallCollectionDescriptionsActions()
    ; context-dependent, settings for Collection indexed by collectionGroup/collectionIndex
    collectionDescriptions = new String[128]
    ; do not allow Print in MCM
    s_collectibleActions = New String[4]
    s_collectibleActions[0] = "$SHSE_LEAVE_BEHIND"
    s_collectibleActions[1] = "$SHSE_PICK_UP"
    s_collectibleActions[2] = "$SHSE_CONTAINER_GLOW_PERSISTENT"
    s_collectibleActions[3] = "$SHSE_PRINT_MESSAGE"
EndFunction

Function InstallQuestObjectHandling()
    s_questObjectHandlingArray = New String[2]
    s_questObjectHandlingArray[0] = "$SHSE_DONT_PICK_UP"
    s_questObjectHandlingArray[1] = "$SHSE_CONTAINER_GLOW_PERSISTENT"
    if questObjectLoot == 2
        questObjectLoot = 1
    endIf
EndFunction

Function InstallSagaRendering()
    sagaDayCount = 0
    currentSagaDay = 0
    currentSagaDayPage = 0
    sagaDayPageCount = 0
EndFunction

Function InstallAdventures()
    adventureType = 0
    adventureTypeNames = new String[128]
    adventureTypeCount = 0

    worldIndex = 0
    worldNames = new String[128]
    worldCount = 0

    adventureActive = false
EndFunction

Function InstallAdventuresPower()
    ;clear out existing adventure - the fields could be out of date for new logic
    SetAdventuresStatus(False)
EndFunction

Function InstallFortunePower()
    FortuneHuntersInstinctPower = Game.GetFormFromFile(0x818, "SmartHarvestSE.esp") as Spell
    CheckFortunePower()
EndFunction

Function ResetShaders()
    int index = 0
    while index < glowReasonSettingArray.length
        glowReasonSettingArray[index] = index
        index = index + 1
    endWhile
EndFunction

Function InstallFlexibleShaders()
    ;set shader defaults - MCM can alter associations to glow category post facto
    type_Glow = 6
    eventScript.SetDefaultShaders()
    s_glowReasonArray = new String[8]
    s_glowReasonArray[0] = "$SHSE_GLOW_REASON_LOCKED"
    s_glowReasonArray[1] = "$SHSE_GLOW_REASON_BOSS"
    s_glowReasonArray[2] = "$SHSE_GLOW_REASON_QUEST"
    s_glowReasonArray[3] = "$SHSE_GLOW_REASON_COLLECTIBLE"
    s_glowReasonArray[4] = "$SHSE_GLOW_REASON_VALUABLE"
    s_glowReasonArray[5] = "$SHSE_GLOW_REASON_ENCHANTED"
    s_glowReasonArray[6] = "$SHSE_GLOW_REASON_OWNED"
    s_glowReasonArray[7] = "$SHSE_GLOW_REASON_SIMPLE"
    ; MCM constructs
    id_glowReasonArray = new Int[8]
    glowReasonSettingArray = new Int[8]
    ResetShaders()
    s_shaderColourArray = New String[8]
    s_shaderColourArray[0] = "$SHSE_GLOW_SHADER_RED"
    s_shaderColourArray[1] = "$SHSE_GLOW_SHADER_FLAMES"
    s_shaderColourArray[2] = "$SHSE_GLOW_SHADER_PURPLE"
    s_shaderColourArray[3] = "$SHSE_GLOW_SHADER_COPPER"
    s_shaderColourArray[4] = "$SHSE_GLOW_SHADER_GOLD"
    s_shaderColourArray[5] = "$SHSE_GLOW_SHADER_BLUE"
    s_shaderColourArray[6] = "$SHSE_GLOW_SHADER_GREEN"
    s_shaderColourArray[7] = "$SHSE_GLOW_SHADER_SILVER"

EndFunction

Function InstallVerticalRadiusAndDoorRule()
    defaultVerticalRadiusFactor = 1.0
    verticalRadiusFactor = defaultVerticalRadiusFactor
    defaultDoorsPreventLooting = 0
    doorsPreventLooting = defaultDoorsPreventLooting
EndFunction

Function InstallDamageLootOptions()
    s_ammoBehaviorArray = New String[5]
    s_ammoBehaviorArray[0] = "$SHSE_DONT_PICK_UP"
    s_ammoBehaviorArray[1] = "$SHSE_PICK_UP_W/O_MSG"
    s_ammoBehaviorArray[2] = "$SHSE_PICK_UP_W/MSG"
    s_ammoBehaviorArray[3] = "$SHSE_PICK_UP_DAMAGE_W/O_MSG"
    s_ammoBehaviorArray[4] = "$SHSE_PICK_UP_DAMAGE_W/MSG"
EndFunction

Function InstallAestheticLootOptions()
    s_harvestBehaviorArray = New String[2]
    s_harvestBehaviorArray[0] = "$SHSE_DONT_PICK_UP"
    s_harvestBehaviorArray[1] = "$SHSE_PICK_UP"
EndFunction

Function InstallLockedContainerOptions()
    s_lockedContainerHandlingArray = New String[4]
    s_lockedContainerHandlingArray[0] = "$SHSE_DONT_PICK_UP"
    s_lockedContainerHandlingArray[1] = "$SHSE_PICK_UP"
    s_lockedContainerHandlingArray[2] = "$SHSE_CONTAINER_GLOW_PERSISTENT"
    s_lockedContainerHandlingArray[3] = "$SHSE_PICK_UP_AFTER_UNLOCK"
EndFunction

Function InitPages()
    Pages = New String[10]
    Pages[0] = "$SHSE_RULES_DEFAULTS_PAGENAME"
    Pages[1] = "$SHSE_SPECIALS_REALISM_PAGENAME"
    Pages[2] = "$SHSE_SHARED_SETTINGS_PAGENAME"
    Pages[3] = "$SHSE_COLLECTIONS_PAGENAME"
    Pages[4] = Replace(GetTranslation("$SHSE_PLAYER_SAGA_PAGENAME"), "{PLAYERNAME}", thisPlayer.GetBaseObject().GetName())
    Pages[5] = "$SHSE_LOOT_SENSE_PAGENAME"
    Pages[6] = "$SHSE_WHITELIST_PAGENAME"
    Pages[7] = "$SHSE_BLACKLIST_PAGENAME"
    Pages[8] = "$SHSE_INVENTORY_EXCESS_RULES_PAGENAME"
    Pages[9] = "$SHSE_TRANSFER_TARGETS_PAGENAME"
EndFunction

Function InitSettingsFileOptions()
    s_iniSaveLoadArray = New String[4]
    iniSaveLoad = 0
    s_iniSaveLoadArray[0] = "$SHSE_PRESET_DO_NOTHING"
    s_iniSaveLoadArray[1] = "$SHSE_PRESET_RESTORE"
    s_iniSaveLoadArray[2] = "$SHSE_PRESET_STORE"
    s_iniSaveLoadArray[3] = "$SHSE_PRESET_RESET"
EndFunction

Function MigrateFromFormLists()
    eventScript.CreateArraysFromFormLists()
EndFunction

Function InitExcessInventoryHandling()
    eventScript.CreateTransferListArrays()
    AllocateExcessHandlingArrays()
    DefaultExcessDisposalChoices()
    excessLeave = 0
    excessSell = 1
    type_Handling = 7
    type_MaxItems = 8
    type_MaxWeight = 9
EndFunction

Function ExpandExcessInventoryTargets()
    eventScript.MigrateTransferListArrays(16, 64)
EndFunction

Function ResetExcessInventoryTargets()
    ; reset handling if set to container
    bool updated = False
    if craftingItemsExcessHandling > excessDisposalSell
        AlwaysTrace("Reset target container for crafting items")
        craftingItemsExcessHandling = 0
        updated = True
    endIf
    int objType = 0
    while objType < excessHandlingArray.Length
        if excessHandlingArray[objType] > excessDisposalSell
            AlwaysTrace("Reset target container for " + GetObjectTypeNameByType(objType) + " items")
            excessHandlingArray[objType] = 0
            updated = True
        endIf
        objType += 1
    endWhile
    eventScript.ResetExcessInventoryTargets(updated)
EndFunction

Function InitExcessCraftingItems()
    handleExcessCraftingItems = False
    craftingItemsExcessHandling = 0
    craftingItemsExcessCount = 0
    craftingItemsExcessWeight = 0.0
EndFunction

Function InitFortuneHunting()
    fortuneHuntItem = false
    fortuneHuntNPC = false
    fortuneHuntContainer = false
EndFunction

Function RemoveLightCategory()
    ; update script variables needing sync to native
    objType_Flora = GetObjectTypeByName("flora")
    objType_Critter = GetObjectTypeByName("critter")
    objType_Septim = GetObjectTypeByName("septims")
    objType_LockPick = GetObjectTypeByName("lockpick")
    objType_Soulgem = GetObjectTypeByName("soulgem")
    objType_Gem = GetObjectTypeByName("gem")
    objType_Ingredient = GetObjectTypeByName("ingredient")
    objType_Weapon = GetObjectTypeByName("weapon")
    objType_Armor = GetObjectTypeByName("armor")
    objType_Jewelry= GetObjectTypeByName("jewelry")
    objType_Key = GetObjectTypeByName("key")
    objType_Ammo = GetObjectTypeByName("ammo")
    objType_Mine = GetObjectTypeByName("orevein")
    objType_Clutter = GetObjectTypeByName("clutter")
    objType_Potion = GetObjectTypeByName("potion")
    objType_Poison = GetObjectTypeByName("poison")
    objType_Scroll = GetObjectTypeByName("scroll")
    
    eventScript.SyncUpdatedNativeDataTypes()

    ; shuffle arrays
    int doneInit = g_InitComplete.GetValue() as int
    if doneInit != 0
        ; arrays should all be in place, preserve settings that come later than light's old slot
        ; old arrays have two unused settings - 0 for unknown, and an entry for light (after clutter)
        if id_objectSettingArray && id_objectSettingArray.length > 30
            Int[] newId = New Int[30]
            int index = 0
            int oldIndex = 1
            while index < newId.length
                newId[index] = id_objectSettingArray[oldIndex]
                index += 1
                if oldIndex == objType_Clutter
                    oldIndex += 1
                endif
                oldIndex += 1
            endwhile
            id_objectSettingArray = newId
        endif
        if objectSettingArray && objectSettingArray.length > 30
            float[] newObj = New float[30]
            int index = 0
            int oldIndex = 1
            while index < newObj.length
                newObj[index] = objectSettingArray[oldIndex]
                index += 1
                if oldIndex == objType_Clutter
                    oldIndex += 1
                endif
                oldIndex += 1
            endwhile
            objectSettingArray = newObj
        endif
        if id_valueWeightArray && id_valueWeightArray.length > 30
            int[] newidVW = New int[30]
            int index = 0
            int oldIndex = 1
            while index < newidVW.length
                newidVW[index] = id_valueWeightArray[oldIndex]
                index += 1
                if oldIndex == objType_Clutter
                    oldIndex += 1
                endif
                oldIndex += 1
            endwhile
            id_valueWeightArray = newidVW
        endif
        if valueWeightSettingArray && valueWeightSettingArray.length > 30
            float[] newVW = New float[30]
            int index = 0
            int oldIndex = 1
            while index < newVW.length
                newVW[index] = valueWeightSettingArray[oldIndex]
                index += 1
                if oldIndex == objType_Clutter
                    oldIndex += 1
                endif
                oldIndex += 1
            endwhile
            valueWeightSettingArray = newVW
        endif
    endIf
    SetObjectTypeData()

EndFunction

Function AddConsumableObjectTypes()
    objType_Food = GetObjectTypeByName("food")
    objType_Drink = GetObjectTypeByName("drink")

    eventScript.SyncUpdatedNativeDataTypes()
EndFunction

Function InitEnchantedObjects()
    s_enchantedObjectHandlingArray = New String[5]
    s_enchantedObjectHandlingArray[0] = "$SHSE_DONT_PICK_UP"
    s_enchantedObjectHandlingArray[1] = "$SHSE_PICK_UP"
    s_enchantedObjectHandlingArray[2] = "$SHSE_CONTAINER_GLOW_PERSISTENT"
    s_enchantedObjectHandlingArray[3] = "$SHSE_PICK_UP_UNKNOWN"
    s_enchantedObjectHandlingArray[4] = "$SHSE_CONTAINER_GLOW_PERSISTENT_UNKNOWN"
EndFunction

; called when new game started or mod installed mid-playthrough
Event OnConfigInit()
    ;DebugTrace("** OnConfigInit start **")
    CheckFirstTimeEver()

    ModName = "$SHSE_MOD_NAME"
    InitSettingsFileOptions()

    s_populationCenterArray = New String[4]
    s_populationCenterArray[0] = "$SHSE_POPULATION_ALLOW_IN_ALL"
    s_populationCenterArray[1] = "$SHSE_POPULATION_DISALLOW_IN_SETTLEMENTS"
    s_populationCenterArray[2] = "$SHSE_POPULATION_DISALLOW_IN_TOWNS"
    s_populationCenterArray[3] = "$SHSE_POPULATION_DISALLOW_IN_CITIES"

    s_playContainerAnimationArray = New String[3]
    s_playContainerAnimationArray[0] = "$SHSE_CONTAINER_NO_ACTION"
    s_playContainerAnimationArray[1] = "$SHSE_CONTAINER_PLAY_ANIMATION"
    s_playContainerAnimationArray[2] = "$SHSE_CONTAINER_GLOW_TRANSIENT"

    s_crimeCheckNotSneakingArray = New String[3]
    s_crimeCheckNotSneakingArray[0] = "$SHSE_ALLOW_CRIMES"
    s_crimeCheckNotSneakingArray[1] = "$SHSE_PREVENT_CRIMES"
    s_crimeCheckNotSneakingArray[2] = "$SHSE_OWNERLESS_ONLY"

    s_crimeCheckSneakingArray = New String[3]
    s_crimeCheckSneakingArray[0] = "$SHSE_ALLOW_CRIMES"
    s_crimeCheckSneakingArray[1] = "$SHSE_PREVENT_CRIMES"
    s_crimeCheckSneakingArray[2] = "$SHSE_OWNERLESS_ONLY"

    s_specialObjectHandlingArray = New String[3]
    s_specialObjectHandlingArray[0] = "$SHSE_DONT_PICK_UP"
    s_specialObjectHandlingArray[1] = "$SHSE_PICK_UP"
    s_specialObjectHandlingArray[2] = "$SHSE_CONTAINER_GLOW_PERSISTENT"

    InitEnchantedObjects()

    s_behaviorArray = New String[5]
    s_behaviorArray[0] = "$SHSE_DONT_PICK_UP"
    s_behaviorArray[1] = "$SHSE_PICK_UP_W/O_MSG"
    s_behaviorArray[2] = "$SHSE_PICK_UP_W/MSG"
    s_behaviorArray[3] = "$SHSE_PICK_UP_V/W_W/O_MSG"
    s_behaviorArray[4] = "$SHSE_PICK_UP_V/W_W/MSG"

    InstallDamageLootOptions()
    InstallAestheticLootOptions()
    InstallLockedContainerOptions()

    eventScript.whitelist_form = Game.GetFormFromFile(0x801, "SmartHarvestSE.esp") as Formlist
    eventScript.blacklist_form = Game.GetFormFromFile(0x802, "SmartHarvestSE.esp") as Formlist

    SetOreVeinChoices()
    SetDeadBodyChoices()
    DefaultExcessDisposalChoices()
    SetMiscDefaults(true)
    InstallVerticalRadiusAndDoorRule()
    SetObjectTypeData()

    ;DebugTrace("** OnConfigInit finished **")
endEvent

int function GetVersion()
    return 55
endFunction

; called when mod is _upgraded_ mid-playthrough
Event OnVersionUpdate(int a_version)
    ;DebugTrace("OnVersionUpdate start" + a_version)
    if (a_version >= 25 && CurrentVersion < 25)
        ; clean up after release with bad upgrade/install workflow and MCM bugs
        ; logic required to support existing saves, as well as the update per se
        ;DebugTrace(self + ": Updating script to version " + a_version)
        CheckFirstTimeEver()
        SetOreVeinChoices()
        SetMiscDefaults(false)
        SetObjectTypeData()
    endIf
    if (a_version >= 26 && CurrentVersion < 26)
        ;fix up arrays if missed in bad save from prior version
        CheckItemCategoryArrays()
    endIf
    if (a_version >= 30 && CurrentVersion < 30)
        ;defaults for all new settings
        notifyLocationChange = false
        valuableItemLoot = 2
        valuableItemThreshold = 500
        InstallCollections()
        SetDeadBodyChoices()
	
    	; update script variables needing sync to native
    	objType_Flora = GetObjectTypeByName("flora")
    	objType_Critter = GetObjectTypeByName("critter")
    	objType_Septim = GetObjectTypeByName("septims")
    	objType_LockPick = GetObjectTypeByName("lockpick")
    	objType_Soulgem = GetObjectTypeByName("soulgem")
    	objType_Key = GetObjectTypeByName("key")
    	objType_Ammo = GetObjectTypeByName("ammo")
    	objType_Mine = GetObjectTypeByName("orevein")
    	
    	eventScript.SyncUpdatedNativeDataTypes()
    endIf
    if (a_version >= 31 && CurrentVersion < 31)
        ;defaults for all new settings
        InstallVerticalRadiusAndDoorRule()
    endIf
    if (a_version >= 32 && CurrentVersion < 32)
        ;defaults for all new settings
        InstallCollectionGroupPolicy()
    endIf
    if (a_version >= 33 && CurrentVersion < 33)
        ;arrow damage loot options - no V/W
        InstallDamageLootOptions()
    endIf
    if (a_version >= 34 && CurrentVersion < 34)
        ;adds reset-to-defaults
        InitSettingsFileOptions()
    endIf
    if (a_version >= 35 && CurrentVersion < 35)
        ;fixes missing entry in plugin
        eventScript.SyncVeinResourceTypes()
    endIf
    if (a_version >= 36 && CurrentVersion < 36)
        InstallAdventures()
    endIf
    if (a_version >= 37 && CurrentVersion < 37)
        InstallAdventuresPower()
    endIf
    if (a_version >= 38 && CurrentVersion < 38)
        ;formID mess sorted out
        AdventurersInstinctPower = Game.GetFormFromFile(0x817, "SmartHarvestSE.esp") as Spell
    endIf
    if a_version >= 39 && CurrentVersion < 39
        ;adventure types swapped for alphabetical ordering
        if adventureType == 9
            adventureType = 10
        elseif adventureType == 10
            adventureType = 9
        endIf
    endIf
    if a_version >= 40 && CurrentVersion < 40
        InstallFlexibleShaders()
        InstallFortunePower()
    endIf
    if a_version >= 41 && CurrentVersion < 41
        InstallCollectionDescriptionsActions()
    endIf
    if a_version >= 42 && CurrentVersion < 42
        InstallQuestObjectHandling()
    endIf
    if a_version >= 43 && CurrentVersion < 43
        InstallSagaRendering()
    endIf
    if a_version >= 44 && CurrentVersion < 44
        ; update to Leave action string
        InstallCollectionDescriptionsActions()
        ; Nuke enchanted item setting with new default
        enchantedItemLoot = 1
        lootAllowedItemsInSettlement = true
    endIf
    if a_version >= 45 && CurrentVersion < 45
        ; sync to Event script and remove any reuse of a key
        eventScript.SyncPauseKey(pauseHotkeyCode)
        if whiteListHotkeyCode == pauseHotkeyCode
            whiteListHotkeyCode = 0
        endIf
        eventScript.SyncWhiteListKey(whiteListHotkeyCode)
        if blackListHotkeyCode == pauseHotkeyCode || blackListHotkeyCode == whiteListHotkeyCode
            blackListHotkeyCode = 0
        endIf
        eventScript.SyncBlackListKey(blackListHotkeyCode)
    endIf
    if a_version >= 46 && CurrentVersion < 46
        MigrateFromFormLists()
    endIf
    if a_version >= 47 && CurrentVersion < 47
        unknownIngredientLoot = false
        InitEnchantedObjects()
        RemoveLightCategory()
        InitExcessInventoryHandling()
    endIf
    if a_version >= 48 && CurrentVersion < 48
        InitFortuneHunting()
    endif
    if a_version >= 49 && CurrentVersion < 49
        InitExcessCraftingItems()
    endif
    if a_version >= 50 && CurrentVersion < 50
        ;aesthetics loot options - critter/flora
        InstallAestheticLootOptions()
        if objectSettingArray[objType_Critter - 1] > 1
            objectSettingArray[objType_Critter - 1] = 1
        endIf
        if objectSettingArray[objType_Flora - 1] > 1
            objectSettingArray[objType_Flora - 1] = 1
        endIf
    endIf
    if a_version >= 51 && CurrentVersion < 51
        ExpandExcessInventoryTargets()
        InstallLockedContainerOptions()
    endIf
    if a_version >= 52 && CurrentVersion < 52
	ResetExcessInventoryTargets()
        AddConsumableObjectTypes()
    endIf
    if a_version >= 53 && CurrentVersion < 53
        lootAllowedItemsInPlayerHouse = false
    endIf
    if a_version >= 54 && CurrentVersion < 54
        updateMiningToolsRequired(false)
    endIf
    if a_version >= 55 && CurrentVersion < 55
        updateDisallowMiningIfSneaking(False)
        ; this got a new option
        SetDeadBodyChoices()
        SetCheckWeightless()
    endif
endEvent

; when mod is applied mid-playthrough, this gets called after OnVersionUpdate/OnConfigInit
Event OnGameReload()
    AlwaysTrace("SHSE_MCM.OnGameReload starting")
    parent.OnGameReload()
    ApplySetting()

    ; ensure event script and plugin have accurate form lists
    PopulateLists()
    eventScript.UpdateWhiteList(whiteListEntries, whiteList_form_array, whiteList_flag_array, "$SHSE_WHITELIST_REMOVED")
    eventScript.UpdateBlackList(blackListEntries, blackList_form_array, blackList_flag_array, "$SHSE_BLACKLIST_REMOVED")
    UpdateTransferTargets()
    eventScript.SyncLists(True, True)
    
    ; tell plugin settings are good
    GameIsReady()
    AlwaysTrace("SHSE_MCM.OnGameReload complete")
endEvent

Function PopulateLists()
    int max_size = eventScript.GetWhiteListSize()
    if max_size > 0
        Form[] currentList = eventScript.GetWhiteList()
        ; assume max size initially, resize if bad entries are found
        int validSize = 0
        int index = max_size
        while index > 0
            index -= 1
            Form nextEntry = currentList[index]
            string name = GetNameForListForm(nextEntry)
            if nextEntry && StringUtil.GetLength(name) > 0
                validSize += 1
            else
                AlwaysTrace("Skip bad WhiteList Form (" + nextEntry + ")")
            endIf
        endWhile
        ; copy in only the valid forms
        if validSize > 0
            whiteList_form_array = Utility.CreateFormArray(validSize)
            id_whiteList_array = Utility.CreateIntArray(validSize)
            whiteList_name_array = Utility.CreateStringArray(validSize)
            whiteList_flag_array = Utility.CreateBoolArray(validSize)
            index = max_size
            ; iterate forwards, to preserve order
            index = 0
            int entry = 0
            while index < max_size
                Form nextEntry = currentList[index]
                string name = GetNameForListForm(nextEntry)
                if nextEntry && StringUtil.GetLength(name) > 0
                    whiteList_form_array[entry] = nextEntry
                    whiteList_name_array[entry] = name
                    whiteList_flag_array[entry] = true
                    entry += 1
                endIf
                index += 1
            endWhile
        endIf
        AlwaysTrace("Whitelist has " + validSize + " valid entries, " + max_size + " in Form[]")
        whiteListEntries = validSize
    else
        AlwaysTrace("Whitelist is empty")
        whiteListEntries = 0
    endIf

    max_size = eventScript.GetBlackListSize()
    if max_size > 0
        ; assume max size initially, resize if bad entries are found
        Form[] currentList = eventScript.GetBlackList()
        int validSize = 0
        int index = max_size
        while index > 0
            index -= 1
            Form nextEntry = currentList[index]
            string name = GetNameForListForm(nextEntry)
            if nextEntry && StringUtil.GetLength(name) > 0
                validSize += 1
            else
                AlwaysTrace("Skip bad BlackList Form (" + nextEntry + ")")
            endIf
        endWhile
        ; copy in only the valid forms
        if validSize > 0
            blackList_form_array = Utility.CreateFormArray(validSize)
            id_blackList_array = Utility.CreateIntArray(validSize)
            blackList_name_array = Utility.CreateStringArray(validSize)
            blackList_flag_array = Utility.CreateBoolArray(validSize)
            index = max_size
            ; iterate forwards, to preserve order
            index = 0
            int entry = 0
            while index < max_size
                Form nextEntry = currentList[index]
                string name = GetNameForListForm(nextEntry)
                if nextEntry && StringUtil.GetLength(name) > 0
                    blackList_form_array[entry] = nextEntry
                    blackList_name_array[entry] = name
                    blackList_flag_array[entry] = true
                    entry += 1
                endIf
                index += 1
            endWhile
        endIf
        AlwaysTrace("BlackList has " + validSize + " valid entries, " + max_size + " in Form[]")
        blackListEntries = validSize
    else
        AlwaysTrace("BlackList is empty")
        blackListEntries = 0
    endif

    ; Transfer List can be sparse
    Form[] currentList = eventScript.GetTransferList()
    string[] currentNames = eventScript.GetTransferNames()
    int validSize = 0
    int index = 0
    max_size = 64
    while index < max_size
        Form nextEntry = currentList[index]
        if nextEntry && StringUtil.GetLength(currentNames[index]) > 0
            ;DebugTrace("TransferList Form index " + index + " for " + currentNames[index])
            validSize += 1
        else
            ;DebugTrace("Skip empty TransferList Form index " + index)
        endIf
        index += 1
    endWhile
    ; copy in only the valid forms
    if validSize > 0
        transferList_form_array = Utility.CreateFormArray(validSize)
        id_transferList_array = Utility.CreateIntArray(validSize)
        transferList_name_array = Utility.CreateStringArray(validSize)
        transferList_index_array = Utility.CreateIntArray(validSize)
        transferList_flag_array = Utility.CreateBoolArray(validSize)
        ; iterate forwards, order must be preserved to ensure correct linkage to target
        index = 0
        int entry = 0
        while index < max_size
            Form nextEntry = currentList[index]
            if nextEntry && StringUtil.GetLength(currentNames[index]) > 0
                transferList_form_array[entry] = nextEntry
                transferList_name_array[entry] = currentNames[index]
                transferList_index_array[entry] = index
                transferList_flag_array[entry] = True
                ;DebugTrace("TransferList entry #" + entry + " name " + currentNames[index] + " xrefindex " + index)
                entry += 1
            endIf
            index += 1
        endWhile
    endIf
    AlwaysTrace("TransferList has " + validSize + " valid entries, " + max_size + " in Form[]")
    transferListEntries = validSize
EndFunction

Event OnConfigOpen()
    ;DebugTrace("OnConfigOpen")
    InitPages()
    PopulateLists()
    eventScript.OnMCMOpen()
endEvent

Function PopulateCollectionGroups()
    collectionGroupCount = CollectionGroups()
    int index = 0
    while index < collectionGroupCount 
        collectionGroupNames[index] = CollectionGroupName(index)
        collectionGroupFiles[index] = CollectionGroupFile(index)
        index = index + 1
    endWhile
    while index < 128
        collectionGroupNames[index] = ""
        collectionGroupFiles[index] = ""
        index = index + 1
    endWhile

    ; Make sure we point to a valid group
    if collectionGroup > collectionGroupCount
        collectionGroup = 0
    endIf
EndFunction

Function SyncCollectionPolicyUI()
        SetToggleOptionValueST(collectDuplicates, false, "collectDuplicates")
        SetTextOptionValueST(s_collectibleActions[collectibleAction], false, "collectibleAction")
        SetToggleOptionValueST(collectionAddNotify, false, "collectionAddNotify")
        string displayCollected = Replace(Replace(GetTranslation(collectionStatus), "{TOTAL}", collectionTotal), "{OBTAINED}", collectionObtained)
        SetTextOptionValueST(displayCollected, false, "itemsCollected")
EndFunction

Function GetCollectionPolicy(String collectionName)
    if collectionName != lastKnownPolicy
        collectDuplicates = CollectionAllowsRepeats(collectionGroupNames[collectionGroup], collectionName)
        collectibleAction = CollectionAction(collectionGroupNames[collectionGroup], collectionName)
        collectionAddNotify = CollectionNotifies(collectionGroupNames[collectionGroup], collectionName)
        collectionTotal = CollectionTotal(collectionGroupNames[collectionGroup], collectionName)
        collectionObtained = CollectionObtained(collectionGroupNames[collectionGroup], collectionName)
        collectionStatus = CollectionStatus(collectionGroupNames[collectionGroup], collectionName)
        lastKnownPolicy = collectionName
        SyncCollectionPolicyUI()
    endIf
EndFunction

Function SyncCollectionGroupPolicyUI()
    SetToggleOptionValueST(groupCollectDuplicates, false, "groupCollectDuplicates")
    SetTextOptionValueST(s_collectibleActions[groupCollectibleAction], false, "groupCollectibleAction")
    SetToggleOptionValueST(groupCollectionAddNotify, false, "groupCollectionAddNotify")
EndFunction

Function GetCollectionGroupPolicy()
    groupCollectDuplicates = CollectionGroupAllowsRepeats(collectionGroupNames[collectionGroup])
    groupCollectibleAction = CollectionGroupAction(collectionGroupNames[collectionGroup])
    groupCollectionAddNotify = CollectionGroupNotifies(collectionGroupNames[collectionGroup])
    SyncCollectionGroupPolicyUI()
EndFunction

Function PopulateCollectionsForGroup(bool inMCM, String groupName)
    GetCollectionGroupPolicy()
    collectionCount = CollectionsInGroup(groupName)
    lastKnownPolicy = ""
    int index = 0
    while index < collectionCount
        collectionNames[index] = CollectionNameByIndexInGroup(groupName, index)
        collectionDescriptions[index] = CollectionDescriptionByIndexInGroup(groupName, index)
        index = index + 1
    endWhile
    while index < 128
        collectionNames[index] = ""
        index = index + 1
    endWhile

    ; Make sure we point to a valid group
    if collectionIndex > collectionCount
        collectionIndex = 0
    endIf
    ; reset Collection list for new Group
    if inMCM
        SetMenuOptionValueST(collectionNames[collectionIndex], false, "chooseCollectionIndex")
    endIf
    GetCollectionPolicy(collectionNames[collectionIndex])
EndFunction

Function PopulateAdventureTypes()
    adventureTypeCount = AdventureTypeCount()
    int index = 0
    while index < adventureTypeCount 
        adventureTypeNames[index] = AdventureTypeName(index)
        index = index + 1
    endWhile
    while index < 128
        adventureTypeNames[index] = ""
        index = index + 1
    endWhile

    ; Make sure we point to a valid type
    if adventureType > adventureTypeCount
        adventureType = 0
    endIf
EndFunction

Function PopulateAdventureWorlds()
    worldCount = ViableWorldsByType(adventureType)
    int index = 0
    while index < worldCount
        worldNames[index] = WorldNameByIndex(index)
        index = index + 1
    endWhile
    while index < 128
        worldNames[index] = ""
        index = index + 1
    endWhile

    ; Make sure we point to a valid group
    if worldIndex > worldCount
        worldIndex = 0
    endIf
EndFunction

Event OnConfigClose()
    ;DebugTrace("OnConfigClose")

    iniSaveLoad = 0
    ApplySetting()

    eventScript.UpdateWhiteList(whiteListEntries, whiteList_form_array, whiteList_flag_array, "$SHSE_WHITELIST_REMOVED")
    eventScript.UpdateBlackList(blackListEntries, blackList_form_array, blackList_flag_array, "$SHSE_BLACKLIST_REMOVED")
    UpdateTransferTargets()
    eventScript.SyncLists(False, True)
    eventScript.OnMCMClose()
endEvent

Function UpdateTransferTargets()
    ; Need to avoid hotkey deletion of in-use targets
    bool[] transferListInUse = Utility.CreateBoolArray(transferListEntries)
    int index = 0
    while index < transferListEntries
        transferListInUse[index] = TargetInUse(3 + transferList_index_array[index])
        index += 1
    endWhile

    eventScript.UpdateTransferList(transferListEntries, transferListInUse, transferList_form_array, transferList_index_array, transferList_name_array, transferList_flag_array, "$SHSE_TRANSFERLIST_REMOVED")
EndFunction

Bool Function IsVWRelevant(int objType)
    if objType == objType_Mine || objType == objType_Septim  || objType == objType_Key
        return False
    elseIf objType == objType_LockPick || objType == objType_Critter || objType == objType_Flora
        return False
    endIf
    return True
EndFunction

Function SetVWOptionStatus(int index)
    int objType = index + 1
    if !IsVWRelevant(objType)
        return
    endIf
    int handling = objectSettingArray[index] as int    
    ; V/W slider only valid if option is set to require it
    if handling == 3 || handling == 4
        SetOptionFlags(id_valueWeightArray[index], OPTION_FLAG_NONE)
    else
        SetOptionFlags(id_valueWeightArray[index], OPTION_FLAG_DISABLED)
    endIf
EndFunction

event OnPageReset(string currentPage)
    ;DebugTrace("OnPageReset")

    If (currentPage == "")
        LoadCustomContent("towawot/SmartHarvestSE.dds")
        Return
    Else
        UnloadCustomContent()
    EndIf

    if (currentPage == Pages[0])

;   ======================== LEFT ========================
        SetCursorFillMode(TOP_TO_BOTTOM)

        AddHeaderOption("$SHSE_LOOTING_RULES_HEADER")
        AddToggleOptionST("enableHarvest", "$SHSE_ENABLE_HARVEST", enableHarvest)
        AddToggleOptionST("enableLootContainer", "$SHSE_ENABLE_LOOT_CONTAINER", enableLootContainer)
        AddTextOptionST("enableLootDeadbody", "$SHSE_ENABLE_LOOT_DEADBODY", s_lootDeadBodyArray[enableLootDeadbody])
        AddToggleOptionST("disableDuringCombat", "$SHSE_DISABLE_DURING_COMBAT", disableDuringCombat)
        AddToggleOptionST("disableWhileWeaponIsDrawn", "$SHSE_DISABLE_IF_WEAPON_DRAWN", disableWhileWeaponIsDrawn)
        AddToggleOptionST("disableWhileConcealed", "$SHSE_DISABLE_IF_CONCEALED", disableWhileConcealed)
        AddTextOptionST("preventPopulationCenterLooting", "$SHSE_POPULATION_CENTER_PREVENTION", s_populationCenterArray[preventPopulationCenterLooting])
        AddToggleOptionST("notifyLocationChange", "$SHSE_NOTIFY_LOCATION_CHANGE", notifyLocationChange)
        AddTextOptionST("crimeCheckNotSneaking", "$SHSE_CRIME_CHECK_NOT_SNEAKING", s_crimeCheckNotSneakingArray[crimeCheckNotSneaking])
        AddTextOptionST("crimeCheckSneaking", "$SHSE_CRIME_CHECK_SNEAKING", s_crimeCheckSneakingArray[crimeCheckSneaking])

;   ======================== RIGHT ========================
        SetCursorPosition(1)

        AddHeaderOption("$SHSE_ITEM_HARVEST_DEFAULT_HEADER")
        AddMenuOptionST("iniSaveLoad", "$SHSE_SETTINGS_FILE_OPERATION", s_iniSaveLoadArray[iniSaveLoad])
        AddKeyMapOptionST("pauseHotkeyCode", "$SHSE_PAUSE_KEY", pauseHotkeyCode, OPTION_FLAG_WITH_UNMAP)
        AddSliderOptionST("ValueWeightDefault", "$SHSE_VW_DEFAULT", valueWeightDefault as float)
        AddSliderOptionST("Radius", "$SHSE_RADIUS", radius as float, "$SHSE_DISTANCE")
        AddSliderOptionST("Interval", "$SHSE_INTERVAL", interval, "$SHSE_ELAPSED_TIME")
        AddSliderOptionST("RadiusIndoors", "$SHSE_RADIUS_INDOORS", radiusIndoors as float, "$SHSE_DISTANCE")
        AddSliderOptionST("IntervalIndoors", "$SHSE_INTERVAL_INDOORS", intervalIndoors, "$SHSE_ELAPSED_TIME")
        AddSliderOptionST("MaxMiningItems", "$SHSE_MAX_MINING_ITEMS", maxMiningItems as float)
        AddToggleOptionST("unencumberedInCombat", "$SHSE_UNENCUMBERED_COMBAT", unencumberedInCombat)
        AddToggleOptionST("unencumberedInPlayerHome", "$SHSE_UNENCUMBERED_PLAYER_HOME", unencumberedInPlayerHome)
        AddToggleOptionST("unencumberedIfWeaponDrawn", "$SHSE_UNENCUMBERED_IF_WEAPON_DRAWN", unencumberedIfWeaponDrawn)
        
    elseif (currentPage == Pages[1])

;   ======================== LEFT ========================
        SetCursorFillMode(TOP_TO_BOTTOM)

        AddHeaderOption("$SHSE_SPECIAL_OBJECT_BEHAVIOR_HEADER")
        AddTextOptionST("questObjectLoot", "$SHSE_QUESTOBJECT_LOOT", s_questObjectHandlingArray[questObjectLoot])
        AddTextOptionST("lockedChestLoot", "$SHSE_LOCKEDCHEST_LOOT", s_lockedContainerHandlingArray[lockedChestLoot])
        AddTextOptionST("bossChestLoot", "$SHSE_BOSSCHEST_LOOT", s_specialObjectHandlingArray[bossChestLoot])
        AddTextOptionST("playerBelongingsLoot", "$SHSE_PLAYER_BELONGINGS_LOOT", s_specialObjectHandlingArray[playerBelongingsLoot])
        AddTextOptionST("enchantedItemLootState", "$SHSE_ENCHANTED_ITEM_LOOT", s_enchantedObjectHandlingArray[enchantedItemLoot])
        AddTextOptionST("valuableItemLoot", "$SHSE_VALUABLE_ITEM_LOOT", s_specialObjectHandlingArray[valuableItemLoot])
        AddSliderOptionST("valuableItemThreshold", "$SHSE_VALUABLE_ITEM_THRESHOLD", valuableItemThreshold as float, "$SHSE_MONEY")
        AddToggleOptionST("unknownIngredientLootState", "$SHSE_UNKNOWN_INGREDIENT_LOOT", unknownIngredientLoot as bool)
        AddToggleOptionST("manualLootTargetNotify", "$SHSE_MANUAL_LOOT_TARGET_NOTIFY", manualLootTargetNotify)
        AddToggleOptionST("whiteListTargetNotify", "$SHSE_WHITELIST_TARGET_NOTIFY", whiteListTargetNotify)
        AddTextOptionST("playContainerAnimation", "$SHSE_PLAY_CONTAINER_ANIMATION", s_playContainerAnimationArray[playContainerAnimation])

;   ======================== RIGHT ========================
        SetCursorPosition(1)

        AddHeaderOption("$SHSE_MORE_REALISM_HEADER")
        AddSliderOptionST("VerticalRadiusFactorState", "$SHSE_VERTICAL_RADIUS_FACTOR", verticalRadiusFactor as float, "{2}")
        AddToggleOptionST("DoorsPreventLootingState", "$SHSE_DOORS_PREVENT_LOOTING", doorsPreventLooting as bool)
        AddToggleOptionST("LootAllowedItemsInSettlementState", "$SHSE_LOOT_ALLOWED_ITEMS_IN_SETTLEMENT", lootAllowedItemsInSettlement as bool)
        AddToggleOptionST("LootAllowedItemsInPlayerHouseState", "$SHSE_LOOT_ALLOWED_ITEMS_IN_PLAYER_HOUSE", lootAllowedItemsInPlayerHouse as bool)
        AddToggleOptionST("MiningToolsRequired", "$SHSE_MINING_TOOLS_REQUIRED", miningToolsRequired)
        AddToggleOptionST("DisallowMiningIfSneaking", "$SHSE_NO_SNEAKY_MINING", disallowMiningIfSneaking)

    elseif (currentPage == Pages[2]) ; object harvester
        
;   ======================== LEFT ========================
        SetCursorFillMode(TOP_TO_BOTTOM)

        AddHeaderOption("$SHSE_PICK_UP_ITEM_TYPE_HEADER")
        AddToggleOptionST("CheckWeightlessValue", "$SHSE_CHECK_WEIGHTLESS_VALUE", checkWeightlessValue as bool)
        
        int index = 0
        int objType = 1
        while index < s_objectTypeNameArray.length ; oreVein is the last
            if (objType == objType_Mine)
                id_objectSettingArray[index] = AddTextOption(s_objectTypeNameArray[index], s_behaviorToggleArray[(objectSettingArray[index] as int)])
            elseif (objType == objType_Critter || objType == objType_Flora)
                id_objectSettingArray[index] = AddTextOption(s_objectTypeNameArray[index], s_harvestBehaviorArray[(objectSettingArray[index] as int)])
            elseif (objType == objType_Ammo)
                id_objectSettingArray[index] = AddTextOption(s_objectTypeNameArray[index], s_ammoBehaviorArray[(objectSettingArray[index] as int)])
            else
                id_objectSettingArray[index] = AddTextOption(s_objectTypeNameArray[index], s_behaviorArray[(objectSettingArray[index] as int)])
            endif
            index += 1
            objType += 1 
        endWhile

;   ======================== RIGHT ========================
        SetCursorPosition(1)

        AddHeaderOption("$SHSE_VALUE/WEIGHT_HEADER")
        int checkFlags = OPTION_FLAG_NONE
        if !checkWeightlessValue
            checkFlags = OPTION_FLAG_DISABLED
        endif
        AddSliderOptionST("WeightlessMinimumValue", "$SHSE_WEIGHTLESS_MINIMUM_VALUE", weightlessMinimumValue as float, "$SHSE_MONEY", checkFlags)

        index = 0
        objType = 1
        while index < s_objectTypeNameArray.length ; oreVein is the last
            ; do not request V/W for weightless or unhandleable item types
            if !IsVWRelevant(objType)
                AddEmptyOption()
            else
                int flags = OPTION_FLAG_NONE
                int handling = objectSettingArray[index] as int    
                ; V/W slider only valid if option is set to require it
                if handling != 3 && handling != 4
                    flags = OPTION_FLAG_DISABLED
                endIf
                if objType == objType_Ammo
                    ; absolute damage
                    id_valueWeightArray[index] = AddSliderOption(s_objectTypeNameArray[index], valueWeightSettingArray[index], "$SHSE_DAMAGE", flags)
                else
                    id_valueWeightArray[index] = AddSliderOption(s_objectTypeNameArray[index], valueWeightSettingArray[index], "$SHSE_V/W", flags)
                endif
            endif
            index += 1
            objType += 1 
        endWhile
        
    elseif currentPage == Pages[3] ; collections
;   ======================== LEFT ========================
        SetCursorFillMode(TOP_TO_BOTTOM)

        PopulateCollectionGroups()
        ; per-group fields only accessible if appropriate
        int flags = OPTION_FLAG_DISABLED
        string initialGroupName = ""
        if collectionsEnabled && collectionGroupCount > 0
            flags = OPTION_FLAG_NONE
            initialGroupName = collectionGroupNames[collectionGroup]
            PopulateCollectionsForGroup(False, initialGroupName)
        endIf

        AddHeaderOption("$SHSE_COLLECTIONS_GLOBAL_HEADER")
        AddToggleOptionST("collectionsEnabled", "$SHSE_COLLECTIONS_ENABLED", collectionsEnabled)
        AddMenuOptionST("chooseCollectionGroup", "$SHSE_CHOOSE_COLLECTION_GROUP", initialGroupName, flags)
        AddTextOptionST("groupCollectibleAction", "$SHSE_COLLECTIBLE_ACTION", s_collectibleActions[groupCollectibleAction], flags)
        AddToggleOptionST("groupCollectionAddNotify", "$SHSE_COLLECTION_ADD_NOTIFY", groupCollectionAddNotify, flags)
        AddToggleOptionST("groupCollectDuplicates", "$SHSE_COLLECT_DUPLICATES", groupCollectDuplicates, flags)

        AddHeaderOption("$SHSE_COLLECTION_GROUP_HEADER")
        AddMenuOptionST("chooseCollectionIndex", "$SHSE_CHOOSE_COLLECTION", "", flags)
        AddTextOptionST("collectibleAction", "$SHSE_COLLECTIBLE_ACTION", s_collectibleActions[collectibleAction], flags)
        AddToggleOptionST("collectionAddNotify", "$SHSE_COLLECTION_ADD_NOTIFY", collectionAddNotify, flags)
        AddToggleOptionST("collectDuplicates", "$SHSE_COLLECT_DUPLICATES", collectDuplicates, flags)
        AddTextOptionST("itemsCollected", "", "", flags)

;   ======================== RIGHT ========================
        SetCursorPosition(1)
        ; adventure fields only accessible if enabled
        PopulateAdventureTypes()
        int adventureTypeFlags = OPTION_FLAG_DISABLED
        string initialAdventureType = ""
        if adventuresEnabled
            adventureTypeFlags = OPTION_FLAG_NONE
        endIf
        int adventureFlags = OPTION_FLAG_DISABLED
        string initialAdventureWorld = ""
        ; sync script state from plugin, in case adventure completed
        adventureActive = HasAdventureTarget()
        if adventureActive
            adventureFlags = OPTION_FLAG_NONE
            PopulateAdventureWorlds()
            initialAdventureType = AdventureTypeName(adventureType)
            initialAdventureWorld = worldNames[worldIndex]
        endIf
        AddHeaderOption("$SHSE_CHOOSE_ADVENTURE_HEADER")
        AddToggleOptionST("adventuresEnabled", "$SHSE_ADVENTURE_ENABLED", adventuresEnabled)
        AddMenuOptionST("chooseAdventureType", "$SHSE_CHOOSE_ADVENTURE_TYPE", initialAdventureType, adventureTypeFlags)
        AddMenuOptionST("chooseAdventureWorld", "$SHSE_CHOOSE_ADVENTURE_WORLD", initialAdventureWorld, adventureFlags)
        AddToggleOptionST("chooseAdventureActive", "$SHSE_CHOOSE_ADVENTURE_ACTIVE", adventureActive, adventureFlags)

    elseif (currentPage == Pages[4]) ; Player saga
        SetCursorFillMode(TOP_TO_BOTTOM)
        sagaDayCount = GetTimelineDays()
        currentSagaDay = sagaDayCount
        AddSliderOptionST("SagaDayState", "$SHSE_SAGA_DAY", currentSagaDay)
        AddSliderOptionST("SagaDayPageState", "$SHSE_SAGA_DAY_PAGE", currentSagaDayPage, "{0}", OPTION_FLAG_DISABLED)

    elseif (currentPage == Pages[5]) ; Fortune Hunter's Instinct and Glow Config
;   ======================== LEFT ========================
        SetCursorFillMode(TOP_TO_BOTTOM)

        AddToggleOptionST("fortuneHuntingEnabled", "$SHSE_LOOT_SENSE_ENABLED", fortuneHuntingEnabled)
        int fortuneHuntFlags = OPTION_FLAG_DISABLED
        if fortuneHuntingEnabled
            fortuneHuntFlags = OPTION_FLAG_NONE
        endIf

        AddToggleOptionST("fortuneHuntItemState", "$SHSE_LOOT_SENSE_ITEM", fortuneHuntItem, fortuneHuntFlags)
        AddToggleOptionST("fortuneHuntNPCState", "$SHSE_LOOT_SENSE_NPC", fortuneHuntNPC, fortuneHuntFlags)
        AddToggleOptionST("fortuneHuntContainerState", "$SHSE_LOOT_SENSE_CONTAINER", fortuneHuntContainer, fortuneHuntFlags)

;   ======================== RIGHT ========================
        SetCursorPosition(1)

        AddHeaderOption("$SHSE_GLOW_COLOUR_HEADER")
        
        AddToggleOptionST("unlockGlowColours", "$SHSE_GLOW_COLOURS_UNLOCKED", unlockGlowColours)
        int glowColourFlags = OPTION_FLAG_DISABLED
        if unlockGlowColours
            glowColourFlags = OPTION_FLAG_NONE
        endIf

        int index = 0
        while index < s_glowReasonArray.length
            id_glowReasonArray[index] = AddTextOption(s_glowReasonArray[index], s_shaderColourArray[(glowReasonSettingArray[index] as int)], glowColourFlags)
            index += 1
        endWhile
        
    elseif (currentPage == Pages[6]) ; whiteList
        AddKeyMapOptionST("whiteListHotkeyCode", "$SHSE_WHITELIST_KEY", whiteListHotkeyCode, OPTION_FLAG_WITH_UNMAP)

        if whiteListEntries == 0
            return
        endif

        int index = 0
        while index < whiteListEntries
            id_whiteList_array[index] = AddToggleOption(whiteList_name_array[index], whiteList_flag_array[index])
            index += 1
        endWhile

    elseif (currentPage == Pages[7]) ; blacklist
        AddKeyMapOptionST("blackListHotkeyCode", "$SHSE_BLACKLIST_KEY", blackListHotkeyCode, OPTION_FLAG_WITH_UNMAP)

        if blackListEntries == 0
            return
        endif

        int index = 0
        while index < blackListEntries
            id_blackList_array[index] = AddToggleOption(blackList_name_array[index], blackList_flag_array[index])
            index += 1
        endWhile

    elseif (currentPage == Pages[8]) ; excess inventory loot handling rules
        ;   ======================== LEFT ========================
        SetCursorFillMode(TOP_TO_BOTTOM)

        ; add the list of current valid targets to the defaults for excess loot handling
        AugmentExcessDisposalChoices()

        AddSliderOptionST("SaleValuePercentState", "$SHSE_SALE_VALUE_PERCENT", saleValuePercent as float)

        int index = 0
        int objType = 1
        int supported = 0
        while index < s_objectTypeNameArray.length && supported < 10 ; put the other half on RHS
            if SupportsExcessHandling(objType)
                id_excessHandlingArray[index] = AddTextOption(s_objectTypeNameArray[index], s_excessDisposalArray[s_excessDisposalEnumToChoice[(excessHandlingArray[index] as int)]])
                id_excessCountArray[index] = AddSliderOption("", excessCountArray[index], "$SHSE_ITEMS")
                id_excessWeightArray[index] = AddSliderOption("", excessWeightArray[index], "$SHSE_WEIGHT")
                supported += 1
            endif
            index += 1
            objType += 1
        endWhile

        ;   ======================== RIGHT ========================
        SetCursorPosition(1)

        AddToggleOptionST("handleExcessCraftingItemsState", "$SHSE_HANDLE_EXCESS_CRAFTING_ITEMS", handleExcessCraftingItems)
        int craftingItemFlags = OPTION_FLAG_DISABLED
        if handleExcessCraftingItems
            craftingItemFlags = OPTION_FLAG_NONE
        endIf
        AddTextOptionST("craftingItemsExcessHandlingState", "$SHSE_CRAFTING_ITEMS", s_excessDisposalArray[s_excessDisposalEnumToChoice[craftingItemsExcessHandling]], craftingItemFlags)
        AddSliderOptionST("craftingItemsExcessCountState", "", craftingItemsExcessCount as float, "$SHSE_ITEMS", craftingItemFlags)
        AddSliderOptionST("craftingItemsExcessWeightState", "", craftingItemsExcessWeight as float, "$SHSE_WEIGHT", craftingItemFlags)

        while index < s_objectTypeNameArray.length ; oreVein is the last
            if SupportsExcessHandling(objType)
                id_excessHandlingArray[index] = AddTextOption(s_objectTypeNameArray[index], s_excessDisposalArray[s_excessDisposalEnumToChoice[(excessHandlingArray[index] as int)]])
                id_excessCountArray[index] = AddSliderOption("", excessCountArray[index], "$SHSE_ITEMS")
                id_excessWeightArray[index] = AddSliderOption("", excessWeightArray[index], "$SHSE_WEIGHT")
            endif
            index += 1
            objType += 1
        endWhile

    elseif (currentPage == Pages[9]) ; transferlist
        if transferListEntries == 0
            return
        endif

        int index = 0
        while index < transferListEntries
            int flags = OPTION_FLAG_NONE
            ; grey this out if it is in use as a loot transfer target
            if TargetInUse(3 + transferList_index_array[index])
                flags = OPTION_FLAG_DISABLED
            endif
            id_transferList_array[index] = AddToggleOption(transferList_name_array[index], transferList_flag_array[index], flags)
            index += 1
        endWhile
    endif
endEvent

bool Function TargetInUse(int handlerId)
    if handlerId == CraftingItemsExcessHandling
        return True
    endIf
    int index = 0
    while index < excessHandlingArray.length
        if excessHandlingArray[index] == handlerId
            return True
        endIf
        index += 1
    endWhile
    return False
EndFunction

Event OnOptionSelect(int a_option)
    int index = -1

    index = id_objectSettingArray.find(a_option)
    if (index >= 0)
        ; offset to skip unknown = 0
        int objType = index + 1
        if (objType == objType_Mine)
            int size = s_behaviorToggleArray.length
            objectSettingArray[index] = CycleInt(objectSettingArray[index] as int, size)
            SetTextOptionValue(a_option, s_behaviorToggleArray[(objectSettingArray[index] as int)])
        elseif (objType == objType_Critter || objType == objType_Flora)
            int size = s_harvestBehaviorArray.length
            objectSettingArray[index] = CycleInt(objectSettingArray[index] as int, size)
            SetTextOptionValue(a_option, s_harvestBehaviorArray[(objectSettingArray[index] as int)])
        elseif (objType == objType_Ammo)
            int size = s_ammoBehaviorArray.length
            objectSettingArray[index] = CycleInt(objectSettingArray[index] as int, size)
            SetTextOptionValue(a_option, s_ammoBehaviorArray[(objectSettingArray[index] as int)])
        else
            int size = s_behaviorArray.length
            objectSettingArray[index] = CycleInt(objectSettingArray[index] as int, size)
            SetTextOptionValue(a_option, s_behaviorArray[(objectSettingArray[index] as int)])
        endif
        SetVWOptionStatus(index)
        return
    endif

    index = id_excessHandlingArray.find(a_option)
    if (index >= 0)
        int objType = index + 1
        bool foundValid = false
        while !foundValid
            excessHandlingArray[index] = CycleExcess(excessHandlingArray[index] as int, excessDisposalOptions)
            ; do not try to sell septims, for obvious reasons
            if objType == objType_Septim && (excessHandlingArray[index] as int) == excessDisposalSell
            ; skip any target that user has unchecked on the Transfer List page
            elseif (excessHandlingArray[index] as int) <= excessDisposalSell || transferList_flag_array[s_excessDisposalEnumToChoice[(excessHandlingArray[index] as int)] - 3]
                foundValid = True
            endif
        endwhile
        SetTextOptionValue(a_option, s_excessDisposalArray[s_excessDisposalEnumToChoice[(excessHandlingArray[index] as int)]])
        return
    endif

    index = id_glowReasonArray.find(a_option)
    if (index >= 0)
        int size = s_shaderColourArray.length
        glowReasonSettingArray[index] = CycleInt(glowReasonSettingArray[index] as int, size)
        SetTextOptionValue(a_option, s_shaderColourArray[(glowReasonSettingArray[index] as int)])
        return
    endif
    
    index = id_whiteList_array.find(a_option)
    if (index >= 0)
        whiteList_flag_array[index] = !whiteList_flag_array[index]
        SetToggleOptionValue(a_option, whiteList_flag_array[index])
        return
    endif

    index = id_blackList_array.find(a_option)
    if (index >= 0)
        blackList_flag_array[index] = !blackList_flag_array[index]
        SetToggleOptionValue(a_option, blackList_flag_array[index])
        return
    endif

    index = id_transferList_array.find(a_option)
    if (index >= 0)
        transferList_flag_array[index] = !transferList_flag_array[index]
        SetToggleOptionValue(a_option, transferList_flag_array[index])
        return
    endif
endEvent

event OnOptionSliderOpen(int a_option)
    SetSliderDialogDefaultValue(0.0)
    SetSliderDialogInterval(1.0)

    ; lookup by object type
    int index = -1
    index = id_valueWeightArray.find(a_option)
    if (index > -1)
        SetSliderDialogStartValue(valueWeightSettingArray[index])
        int objType = index+1
        if objType == objtype_ammo
            SetSliderDialogRange(0, 40)
        else
            SetSliderDialogRange(0, 1000)
        endIf
        return
    endif
    index = id_excessCountArray.find(a_option)
    if (index > -1)
        SetSliderDialogStartValue(excessCountArray[index])
        int objType = index+1
        if objType == objType_Septim
            ; only useful if they have weight
            SetSliderDialogInterval(500)
            SetSliderDialogRange(0, 100000)
        elseif objType == objType_Ammo || objType == objType_LockPick
            ; light consumables
            SetSliderDialogInterval(1)
            SetSliderDialogRange(0, 250)
        elseif objType == objType_Soulgem || objType == objType_Gem || objType == objType_Ingredient || objType == objType_Food || objType == objType_Drink
            ; medium weight crafting, mundane consumables - high for needs mods
            SetSliderDialogInterval(1)
            SetSliderDialogRange(0, 100)
        elseif objType == objType_Weapon || objType == objType_Armor || objType == objType_Key
            ; apparel or keys
            SetSliderDialogInterval(1)
            SetSliderDialogRange(0, 5)
        elseif objType == objType_Potion || objType == objType_Poison || objType == objType_Scroll
            ; consumables with effects
            SetSliderDialogInterval(1)
            SetSliderDialogRange(0, 50)
        else
            ; clutter, animalHide, oreIngot, book, jewelry
            SetSliderDialogInterval(1)
            SetSliderDialogRange(0, 20)
        endif
        return
    endif
    index = id_excessWeightArray.find(a_option)
    if (index > -1)
        SetSliderDialogStartValue(excessWeightArray[index])
        SetSliderDialogRange(0, 500)
        return
    endif
endEvent

event OnOptionSliderAccept(int a_option, float a_value)
    int index = -1

    index = id_valueWeightArray.find(a_option)
    if (index > -1)
        int objType = index+1
        valueWeightSettingArray[index] = a_value
        if objType == objtype_ammo
            SetSliderOptionValue(a_option, a_value, "$SHSE_DAMAGE")
        else
            SetSliderOptionValue(a_option, a_value, "$SHSE_V/W")
        endIf
        return
    endif
    index = id_excessCountArray.find(a_option)
    if (index > -1)
        excessCountArray[index] = a_value
        SetSliderOptionValue(a_option, a_value, "$SHSE_ITEMS")
        return
    endif
    index = id_excessWeightArray.find(a_option)
    if (index > -1)
        excessWeightArray[index] = a_value
        SetSliderOptionValue(a_option, a_value, "$SHSE_WEIGHT")
        return
    endif
endEvent

event OnOptionHighlight(int a_option)
    int index = -1
    
    index = id_objectSettingArray.find(a_option)
    if (index > -1)
        int objType = index + 1
        if objType == objType_Critter || objType == objType_Flora
            SetInfoText(GetTranslation("$SHSE_DESC_HARVESTABLE_TOGGLE"))
        else
            SetInfoText(s_objectTypeNameArray[index])
        endIf
        return
    endif

    index = id_valueWeightArray.find(a_option)
    if (index > -1)
        SetInfoText(s_objectTypeNameArray[index])
        return
    endif
    
    index = id_whiteList_array.find(a_option)
    if (index > -1)
        string translation = GetTranslation("$SHSE_DESC_WHITELIST")
        if (!translation)
            return
        endif
        form item = whiteList_form_array[index]
        if (!item)
            return
        endif

        string[] targets = New String[3]
        targets[0] = "{OBJ_TYPE}"
        targets[1] = "{FROM_ID}"
        targets[2] = "{ESP_NAME}"

        string[] replacements = New String[3]
        replacements[0] = GetTextObjectType(item)
        replacements[1] = PrintFormID(item.GetFormID())
        replacements[2] = GetPluginName(item)
        
        translation = ReplaceArray(translation, targets, replacements)
        if (translation)
            SetInfoText(translation)
        endif
        return
    endif
    
    index = id_blackList_array.find(a_option)
    if (index > -1)
        string translation = GetTranslation("$SHSE_DESC_BLACKLIST")
        if (!translation)
            return
        endif

        form item = blackList_form_array[index]
        if (!item)
            return
        endif

        string[] targets = New String[3]
        targets[0] = "{OBJ_TYPE}"
        targets[1] = "{FROM_ID}"
        targets[2] = "{ESP_NAME}"

        string[] replacements = New String[3]
        replacements[0] = GetTextObjectType(item)
        replacements[1] = PrintFormID(item.GetFormID())
        replacements[2] = GetPluginName(item)
        
        translation = ReplaceArray(translation, targets, replacements)
        if (translation)
            SetInfoText(translation)
        endif
        return
    endif
endEvent

state enableHarvest
    event OnSelectST()
        enableHarvest = !(enableHarvest as bool)
        SetToggleOptionValueST(enableHarvest)
    endEvent

    event OnDefaultST()
        enableHarvest = true
        SetToggleOptionValueST(enableHarvest)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_ENABLE_HARVEST"))
    endEvent
endState

state enableLootContainer
    event OnSelectST()
        enableLootContainer = !(enableLootContainer as bool)
        SetToggleOptionValueST(enableLootContainer)
    endEvent

    event OnDefaultST()
        enableLootContainer = true
        SetToggleOptionValueST(enableLootContainer)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_ENABLE_LOOT_CONTAINER"))
    endEvent
endState

; Custom logic: handle late addition of the 'leave underwear' option while keeping order in UI logical
int Function CycleLootDeadbody(int num)
    if num == excludeNPCUnderwear
        return excludeNPCNone
    elseif num == excludeNPCArmour
        ; only enable the underwear option if there are relevant mods present
        if UseUnderwear()
            return excludeNPCUnderwear
        else
            return excludeNPCNone
        endif
    elseif num == excludeNPCNone
        return excludeNPCAll
    else ; excludeNPCNone
        return excludeNPCArmour
    endif
endFunction

state enableLootDeadbody
    event OnSelectST()
        enableLootDeadbody = CycleLootDeadbody(enableLootDeadbody)
        SetTextOptionValueST(s_lootDeadBodyArray[enableLootDeadbody])
    endEvent

    event OnDefaultST()
        enableLootDeadbody = 1
        SetTextOptionValueST(s_lootDeadBodyArray[enableLootDeadbody])
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_ENABLE_LOOT_DEADBODY"))
    endEvent
endState

state unencumberedInCombat
    event OnSelectST()
        unencumberedInCombat = !(unencumberedInCombat as bool)
        SetToggleOptionValueST(unencumberedInCombat)
    endEvent

    event OnDefaultST()
        unencumberedInCombat = false
        SetToggleOptionValueST(unencumberedInCombat)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_UNENCUMBERED_COMBAT"))
    endEvent
endState

state unencumberedInPlayerHome
    event OnSelectST()
        unencumberedInPlayerHome = !(unencumberedInPlayerHome as bool)
        SetToggleOptionValueST(unencumberedInPlayerHome)
    endEvent

    event OnDefaultST()
        unencumberedInPlayerHome = false
        SetToggleOptionValueST(unencumberedInPlayerHome)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_UNENCUMBERED_PLAYER_HOME"))
    endEvent
endState

state unencumberedIfWeaponDrawn
    event OnSelectST()
        unencumberedIfWeaponDrawn = !(unencumberedIfWeaponDrawn as bool)
        SetToggleOptionValueST(unencumberedIfWeaponDrawn)
    endEvent

    event OnDefaultST()
        unencumberedIfWeaponDrawn = false
        SetToggleOptionValueST(unencumberedIfWeaponDrawn)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_UNENCUMBERED_IF_WEAPON_DRAWN"))
    endEvent
endState

state Radius
    event OnSliderOpenST()
        SetSliderDialogStartValue(radius as float)
        SetSliderDialogDefaultValue(defaultRadius as float)
        SetSliderDialogRange(1.0, 100.0)
        SetSliderDialogInterval(1.0)
    endEvent

    event OnSliderAcceptST(float value)
        radius = value as int
        SetSliderOptionValueST(value, "$SHSE_DISTANCE")
    endEvent

    event OnDefaultST()
        radius = defaultRadius
        SetSliderOptionValueST(radius as int, "$SHSE_DISTANCE")
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_RADIUS"))
    endEvent
endState

state Interval
    event OnSliderOpenST()
        SetSliderDialogStartValue(interval)
        SetSliderDialogDefaultValue(defaultInterval)
        SetSliderDialogRange(0.1, 3.0)
        SetSliderDialogInterval(0.1)
    endEvent

    event OnSliderAcceptST(float value)
        interval = value
        SetSliderOptionValueST(interval, "$SHSE_ELAPSED_TIME")
    endEvent

    event OnDefaultST()
        interval = defaultInterval
        SetSliderOptionValueST(interval, "$SHSE_ELAPSED_TIME")
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_INTERVAL"))
    endEvent
endState

state RadiusIndoors
    event OnSliderOpenST()
        SetSliderDialogStartValue(radiusIndoors as float)
        SetSliderDialogDefaultValue(defaultRadiusIndoors as float)
        SetSliderDialogRange(1.0, 100.0)
        SetSliderDialogInterval(1.0)
    endEvent

    event OnSliderAcceptST(float value)
        radiusIndoors = value as int
        SetSliderOptionValueST(value, "$SHSE_DISTANCE")
    endEvent

    event OnDefaultST()
        radiusIndoors = defaultRadiusIndoors
        SetSliderOptionValueST(radiusIndoors as float, "$SHSE_DISTANCE")
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_RADIUS_INDOORS"))
    endEvent
endState

state IntervalIndoors
    event OnSliderOpenST()
        SetSliderDialogStartValue(intervalIndoors)
        SetSliderDialogDefaultValue(defaultIntervalIndoors)
        SetSliderDialogRange(0.1, 3.0)
        SetSliderDialogInterval(0.1)
    endEvent

    event OnSliderAcceptST(float value)
        intervalIndoors = value
        SetSliderOptionValueST(interval, "$SHSE_ELAPSED_TIME")
    endEvent

    event OnDefaultST()
        intervalIndoors = defaultIntervalIndoors
        SetSliderOptionValueST(intervalIndoors, "$SHSE_ELAPSED_TIME")
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_INTERVAL_INDOORS"))
    endEvent
endState

string Function Round(float number, int precision)
    string result = number as int
    number -= number as int
    if precision > 0
        result += "."
    endif
    while precision > 0
        number *= 10
        precision -= 1
        if precision == 0
            number += 0.5
        endif
        result += number as int
        number -= number as int
    endwhile
    return result
EndFunction

state VerticalRadiusFactorState
    event OnSliderOpenST()
        SetSliderDialogStartValue(verticalRadiusFactor)
        SetSliderDialogDefaultValue(defaultVerticalRadiusFactor)
        SetSliderDialogRange(0.2, 1.0)
        SetSliderDialogInterval(0.05)
    endEvent

    event OnSliderAcceptST(float value)
        verticalRadiusFactor = value
        SetSliderOptionValueST(value, "{2}")
        string result = Replace(GetTranslation("$SHSE_MSG_VERTICAL_RADIUS_FACTOR"), "{INDOORS}", Round(radiusIndoors as float * verticalRadiusFactor, 2))
        ShowMessage(Replace(result, "{OUTDOORS}", Round(radius as float * verticalRadiusFactor, 2)), false)
    endEvent

    event OnDefaultST()
        verticalRadiusFactor = defaultVerticalRadiusFactor
        SetSliderOptionValueST(verticalRadiusFactor, "{2}")
        string result = Replace(GetTranslation("$SHSE_MSG_VERTICAL_RADIUS_FACTOR"), "{INDOORS}", Round(radiusIndoors as float * verticalRadiusFactor, 2))
        ShowMessage(Replace(result, "{OUTDOORS}", Round(radius as float * verticalRadiusFactor, 2)), false)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_VERTICAL_RADIUS_FACTOR"))
    endEvent
endState

state DoorsPreventLootingState
    event OnSelectST()
        doorsPreventLooting = (!(doorsPreventLooting as bool)) as int
        SetToggleOptionValueST(doorsPreventLooting as bool)
    endEvent

    event OnDefaultST()
        doorsPreventLooting = defaultDoorsPreventLooting
        SetToggleOptionValueST(doorsPreventLooting as bool)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_DOORS_PREVENT_LOOTING"))
    endEvent
endState

state LootAllowedItemsInSettlementState
    event OnSelectST()
        lootAllowedItemsInSettlement = (!(lootAllowedItemsInSettlement as bool)) as int
        SetToggleOptionValueST(lootAllowedItemsInSettlement as bool)
    endEvent

    event OnDefaultST()
        lootAllowedItemsInSettlement = true
        SetToggleOptionValueST(lootAllowedItemsInSettlement as bool)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_LOOT_ALLOWED_ITEMS_IN_SETTLEMENT"))
    endEvent
endState

state LootAllowedItemsInPlayerHouseState
    event OnSelectST()
        lootAllowedItemsInPlayerHouse = (!(lootAllowedItemsInPlayerHouse as bool)) as int
        SetToggleOptionValueST(lootAllowedItemsInPlayerHouse as bool)
    endEvent

    event OnDefaultST()
        lootAllowedItemsInPlayerHouse = false
        SetToggleOptionValueST(lootAllowedItemsInPlayerHouse as bool)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_LOOT_ALLOWED_ITEMS_IN_PLAYER_HOUSE"))
    endEvent
endState

state ValueWeightDefault
    event OnSliderOpenST()
        SetSliderDialogStartValue(valueWeightDefault)
        SetSliderDialogDefaultValue(valueWeightDefaultDefault)
        SetSliderDialogRange(0, 1000)
        SetSliderDialogInterval(1)
    endEvent

    event OnSliderAcceptST(float value)
        valueWeightDefault = value as int
        SetSliderOptionValueST(valueWeightDefault)
    endEvent

    event OnDefaultST()
        valueWeightDefault = valueWeightDefaultDefault
        SetSliderOptionValueST(valueWeightDefault)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_VW_DEFAULT"))
    endEvent
endState

state MaxMiningItems
    event OnSliderOpenST()
        SetSliderDialogStartValue(maxMiningItems)
        SetSliderDialogDefaultValue(maxMiningItemsDefault)
        SetSliderDialogRange(1, 100)
        SetSliderDialogInterval(1)
    endEvent

    event OnSliderAcceptST(float value)
        updateMaxMiningItems(value as int)
        SetSliderOptionValueST(maxMiningItems)
    endEvent

    event OnDefaultST()
        updateMaxMiningItems(maxMiningItemsDefault)
        SetSliderOptionValueST(maxMiningItems)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_MAX_MINING_ITEMS"))
    endEvent
endState

state MiningToolsRequired
    event OnSelectST()
        updateMiningToolsRequired(!(miningToolsRequired as bool) as int)
        SetToggleOptionValueST(miningToolsRequired as bool)
    endEvent

    event OnDefaultST()
        updateMiningToolsRequired(miningToolsRequiredDefault)
        SetToggleOptionValueST(miningToolsRequired as bool)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_MINING_TOOLS_REQUIRED"))
    endEvent
endState

state DisallowMiningIfSneaking
    event OnSelectST()
        updateDisallowMiningIfSneaking(!(disallowMiningIfSneaking as bool) as int)
        SetToggleOptionValueST(disallowMiningIfSneaking as bool)
    endEvent

    event OnDefaultST()
        updateDisallowMiningIfSneaking(disallowMiningIfSneaking)
        SetToggleOptionValueST(disallowMiningIfSneaking as bool)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_NO_SNEAKY_MINING"))
    endEvent
endState

state SaleValuePercentState
    event OnSliderOpenST()
        SetSliderDialogStartValue(saleValuePercent)
        SetSliderDialogDefaultValue(25)
        SetSliderDialogRange(0, 100)
        SetSliderDialogInterval(1)
    endEvent

    event OnSliderAcceptST(float value)
        saleValuePercent = value as int
        SetSliderOptionValueST(saleValuePercent)
    endEvent

    event OnDefaultST()
        saleValuePercent = 25
        SetSliderOptionValueST(saleValuePercent)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_SALE_VALUE_PERCENT"))
    endEvent
endState

Function SetExcessCraftingUIFlags()
    if handleExcessCraftingItems
        SetOptionFlagsST(OPTION_FLAG_NONE, false, "craftingItemsExcessHandlingState")
        SetOptionFlagsST(OPTION_FLAG_NONE, false, "craftingItemsExcessCountState")
        SetOptionFlagsST(OPTION_FLAG_NONE, false, "craftingItemsExcessWeightState")
    else
        SetOptionFlagsST(OPTION_FLAG_DISABLED, false, "craftingItemsExcessHandlingState")
        SetOptionFlagsST(OPTION_FLAG_DISABLED, false, "craftingItemsExcessCountState")
        SetOptionFlagsST(OPTION_FLAG_DISABLED, false, "craftingItemsExcessWeightState")
    endIf
EndFunction

state handleExcessCraftingItemsState
    event OnSelectST()
        handleExcessCraftingItems = !handleExcessCraftingItems
        SetToggleOptionValueST(handleExcessCraftingItems)
        SetExcessCraftingUIFlags()
    endEvent

    event OnDefaultST()
        handleExcessCraftingItems = false
        SetToggleOptionValueST(handleExcessCraftingItems)
        SetExcessCraftingUIFlags()
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_HANDLE_EXCESS_CRAFTING_ITEMS"))
    endEvent
endState

state craftingItemsExcessHandlingState
    event OnSelectST()
        bool foundValid = false
        while !foundValid
            craftingItemsExcessHandling = CycleExcess(craftingItemsExcessHandling, excessDisposalOptions)
            ; disallow targets unchecked by user
            if (craftingItemsExcessHandling as int) <= excessDisposalSell || transferList_flag_array[s_excessDisposalEnumToChoice[(craftingItemsExcessHandling as int)] - 3]
                foundValid = True
            endif
        endwhile
        SetTextOptionValueST(s_excessDisposalArray[s_excessDisposalEnumToChoice[craftingItemsExcessHandling]])
    endEvent

    event OnDefaultST()
        craftingItemsExcessHandling = 0
        SetTextOptionValueST(s_excessDisposalArray[s_excessDisposalEnumToChoice[craftingItemsExcessHandling]])
    endEvent
endState

state craftingItemsExcessCountState
    event OnSliderOpenST()
        SetSliderDialogStartValue(craftingItemsExcessCount)
        SetSliderDialogDefaultValue(0)
        SetSliderDialogRange(0, 100)
        SetSliderDialogInterval(1)
    endEvent

    event OnSliderAcceptST(float value)
        craftingItemsExcessCount = value as int
        SetSliderOptionValueST(craftingItemsExcessCount, "$SHSE_ITEMS")
    endEvent

    event OnDefaultST()
        craftingItemsExcessCount = 0
        SetSliderOptionValueST(craftingItemsExcessCount, "$SHSE_ITEMS")
    endEvent
endState

state craftingItemsExcessWeightState
    event OnSliderOpenST()
        SetSliderDialogStartValue(craftingItemsExcessWeight)
        SetSliderDialogDefaultValue(0)
        SetSliderDialogRange(0, 100)
        SetSliderDialogInterval(1)
    endEvent

    event OnSliderAcceptST(float value)
        craftingItemsExcessWeight = value
        SetSliderOptionValueST(craftingItemsExcessWeight, "$SHSE_WEIGHT")
    endEvent

    event OnDefaultST()
        craftingItemsExcessWeight = 0
        SetSliderOptionValueST(craftingItemsExcessWeight, "$SHSE_WEIGHT")
    endEvent
endState

; hotkey conflict detection
string Function GetCustomControl(int keyCode)
    if keyCode == pauseHotkeyCode
        return "SmartHarvest Pause"
    elseif keyCode == whiteListHotkeyCode
        return "SmartHarvest WhiteList"
    elseif keyCode == blackListHotkeyCode
        return "SmartHarvest BlackList"
    else
        return ""
    endIf
endFunction

bool Function AllowMapping(int newKeyCode, string conflictControl, string conflictName)
    ; unmap always allowed
    if newKeyCode == -1
        return True
    endIf
    if (conflictControl != "")
        string msg = GetTranslation("$SHSE_KEY_CONFLICT")
        msg = Replace(msg, "{CONFLICT}", "\"" + conflictControl + "\"")
        ShowMessage(msg, false)
        return False
    endIf
    return True
endFunction

state pauseHotkeyCode
    event OnKeyMapChangeST(int newKeyCode, string conflictControl, string conflictName)
        if conflictControl == "SmartHarvest Pause" || AllowMapping(newKeyCode, conflictControl, conflictName)
            pauseHotkeyCode = newKeyCode
            SetKeyMapOptionValueST(pauseHotkeyCode)
            eventScript.SyncPauseKey(pauseHotkeyCode)
        endIf
    endEvent

    event OnDefaultST()
        OnKeyMapChangeST(34, "SmartHarvest Pause", "")
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_PAUSE_KEY"))
    endEvent
endState

state whiteListHotkeyCode
    event OnKeyMapChangeST(int newKeyCode, string conflictControl, string conflictName)
        if conflictControl == "SmartHarvest WhiteList" || AllowMapping(newKeyCode, conflictControl, conflictName)
            whiteListHotkeyCode = newKeyCode
            SetKeyMapOptionValueST(whiteListHotkeyCode)
            eventScript.SyncWhiteListKey(whiteListHotkeyCode)
        endIf
    endEvent

    event OnDefaultST()
        OnKeyMapChangeST(22, "SmartHarvest WhiteList", "")
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_WHITELIST_KEY"))
    endEvent
endState

state blackListHotkeyCode
    event OnKeyMapChangeST(int newKeyCode, string conflictControl, string conflictName)
        if conflictControl == "SmartHarvest BlackList" || AllowMapping(newKeyCode, conflictControl, conflictName)
            blackListHotkeyCode = newKeyCode
            SetKeyMapOptionValueST(blackListHotkeyCode)
            eventScript.SyncBlackListKey(blackListHotkeyCode)
        endIf
    endEvent

    event OnDefaultST()
        OnKeyMapChangeST(37, "SmartHarvest BlackList", "")
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_BLACKLIST_KEY"))
    endEvent
endState

state iniSaveLoad
    event OnMenuOpenST()
        SetMenuDialogStartIndex(iniSaveLoad)
        SetMenuDialogDefaultIndex(0)
        SetMenuDialogOptions(s_iniSaveLoadArray)
    endEvent

    event OnMenuAcceptST(int index)
        string list_warning
        if (index == 1)
            list_warning = "$SHSE_LOAD_WARNING_MSG"
        elseif (index == 2)
            list_warning = "$SHSE_SAVE_WARNING_MSG"
        elseif (index == 3)
            list_warning = "$SHSE_RESET_WARNING_MSG"
        else
            ; no op - user changed their mind
            return
        endif
        bool continue = ShowMessage(list_warning, true, "$SHSE_OK", "$SHSE_CANCEL")
        ;DebugTrace("response to msg= " + continue)
        if (continue)
            SetMenuOptionValueST(s_iniSaveLoadArray[iniSaveLoad])
            iniSaveLoad = index
            if (iniSaveLoad == 1) ; load from My Custom Settings file
                LoadIniFile(False)
                LoadSettingsFromNative()
            elseif (iniSaveLoad == 2) ; save to My Custom Settings Files after pushing current values
                SaveSettingsToNative()
                SaveIniFile()
            elseif (iniSaveLoad == 3) ; load from Default Settings file
                LoadIniFile(True)
                LoadSettingsFromNative()
            endif
        endif
    endEvent

    event OnDefaultST()
        iniSaveLoad = 0
        SetMenuOptionValueST(s_iniSaveLoadArray[iniSaveLoad])
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_FILE_OPERATION"))
    endEvent
endState

state crimeCheckNotSneaking
    event OnSelectST()
        int size = s_crimeCheckNotSneakingArray.length
        crimeCheckNotSneaking = CycleInt(crimeCheckNotSneaking, size)
        SetTextOptionValueST(s_crimeCheckNotSneakingArray[crimeCheckNotSneaking])
    endEvent

    event OnDefaultST()
        crimeCheckNotSneaking = 2
        SetTextOptionValueST(s_crimeCheckNotSneakingArray[crimeCheckNotSneaking])
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_CRIME_CHECK_NOT_SNEAKING"))
    endEvent
endState

state crimeCheckSneaking
    event OnSelectST()
        int size = s_crimeCheckSneakingArray.length
        crimeCheckSneaking = CycleInt(crimeCheckSneaking, size)
        SetTextOptionValueST(s_crimeCheckSneakingArray[crimeCheckSneaking])
    endEvent

    event OnDefaultST()
        crimeCheckSneaking = 1
        SetTextOptionValueST(s_crimeCheckSneakingArray[crimeCheckSneaking])
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_CRIME_CHECK_SNEAKING"))
    endEvent
endState

state playerBelongingsLoot
    event OnSelectST()
        int size = s_specialObjectHandlingArray.length
        playerBelongingsLoot = CycleInt(playerBelongingsLoot, size)
        SetTextOptionValueST(s_specialObjectHandlingArray[playerBelongingsLoot])
    endEvent

    event OnDefaultST()
        playerBelongingsLoot = 2
        SetTextOptionValueST(s_specialObjectHandlingArray[playerBelongingsLoot])
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_PLAYER_BELONGINGS_LOOT"))
    endEvent
endState

state questObjectLoot
    event OnSelectST()
        int size = s_questObjectHandlingArray.length
        questObjectLoot = CycleInt(questObjectLoot, size)
        SetTextOptionValueST(s_questObjectHandlingArray[questObjectLoot])
    endEvent

    event OnDefaultST()
        questObjectLoot = 1
        SetTextOptionValueST(s_questObjectHandlingArray[questObjectLoot])
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_QUESTOBJECT_LOOT"))
    endEvent
endState

state enchantedItemLootState
    event OnSelectST()
        int size = s_enchantedObjectHandlingArray.length
        enchantedItemLoot = CycleInt(enchantedItemLoot, size)
        SetTextOptionValueST(s_enchantedObjectHandlingArray[enchantedItemLoot])
    endEvent

    event OnDefaultST()
        enchantedItemLoot = 1
        SetTextOptionValueST(s_enchantedObjectHandlingArray[enchantedItemLoot])
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_ENCHANTED_ITEM_LOOT"))
    endEvent
endState

state unknownIngredientLootState
    event OnSelectST()
        unknownIngredientLoot = !(unknownIngredientLoot as bool)
        SetToggleOptionValueST(unknownIngredientLoot)
    endEvent

    event OnDefaultST()
        unknownIngredientLoot = false
        SetToggleOptionValueST(unknownIngredientLoot)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_UNKNOWN_INGREDIENT_LOOT"))
    endEvent
endState

state valuableItemLoot
    event OnSelectST()
        int size = s_specialObjectHandlingArray.length
        valuableItemLoot = CycleInt(valuableItemLoot, size)
        SetTextOptionValueST(s_specialObjectHandlingArray[valuableItemLoot])
    endEvent

    event OnDefaultST()
        valuableItemLoot = 1
        SetTextOptionValueST(s_specialObjectHandlingArray[valuableItemLoot])
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_VALUABLE_ITEM_LOOT"))
    endEvent
endState

state valuableItemThreshold
    event OnSliderOpenST()
        SetSliderDialogStartValue(valuableItemThreshold)
        SetSliderDialogDefaultValue(500)
        SetSliderDialogRange(0, 5000)
        SetSliderDialogInterval(100)
    endEvent

    event OnSliderAcceptST(float value)
        valuableItemThreshold = value as int
        SetSliderOptionValueST(valuableItemThreshold)
    endEvent

    event OnDefaultST()
        valuableItemThreshold = 500
        SetSliderOptionValueST(valuableItemThreshold)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_VALUABLE_ITEM_THRESHOLD"))
    endEvent
endState

Function UpdateWeightlessItemValueSlider()
    if checkWeightlessValue
        SetOptionFlagsST(OPTION_FLAG_NONE, false, "WeightlessMinimumValue")
    else
        SetOptionFlagsST(OPTION_FLAG_DISABLED, false, "WeightlessMinimumValue")
    endIf
EndFunction

state CheckWeightlessValue
    event OnSelectST()
        checkWeightlessValue = !(checkWeightlessValue as bool)
        SetToggleOptionValueST(checkWeightlessValue)
        UpdateWeightlessItemValueSlider()
    endEvent

    event OnDefaultST()
        checkWeightlessValue = checkWeightlessValueDefault
        SetToggleOptionValueST(checkWeightlessValue)
        UpdateWeightlessItemValueSlider()
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_CHECK_WEIGHTLESS_VALUE_DESC"))
    endEvent
endState

state WeightlessMinimumValue
    event OnSliderOpenST()
        SetSliderDialogStartValue(weightlessMinimumValue)
        SetSliderDialogDefaultValue(weightlessMinimumValueDefault)
        SetSliderDialogRange(0, 200)
        SetSliderDialogInterval(1)
    endEvent

    event OnSliderAcceptST(float value)
        weightlessMinimumValue = value as int
        SetSliderOptionValueST(weightlessMinimumValue)
    endEvent

    event OnDefaultST()
        weightlessMinimumValue = weightlessMinimumValueDefault
        SetSliderOptionValueST(weightlessMinimumValue)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_WEIGHTLESS_MINIMUM_VALUE_DESC"))
    endEvent
endState

state lockedChestLoot
    event OnSelectST()
        int size = s_lockedContainerHandlingArray.length
        lockedChestLoot = CycleInt(lockedChestLoot, size)
        SetTextOptionValueST(s_lockedContainerHandlingArray[lockedChestLoot])
    endEvent

    event OnDefaultST()
        lockedChestLoot = 2
        SetTextOptionValueST(s_lockedContainerHandlingArray[lockedChestLoot])
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_LOCKEDCHEST_LOOT"))
    endEvent
endState

state bossChestLoot
    event OnSelectST()
        int size = s_specialObjectHandlingArray.length
        bossChestLoot = CycleInt(bossChestLoot, size)
        SetTextOptionValueST(s_specialObjectHandlingArray[bossChestLoot])
    endEvent

    event OnDefaultST()
        bossChestLoot = 2
        SetTextOptionValueST(s_specialObjectHandlingArray[bossChestLoot])
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_BOSSCHEST_LOOT"))
    endEvent
endState

state manualLootTargetNotify
    event OnSelectST()
        manualLootTargetNotify = !(manualLootTargetNotify as bool)
        SetToggleOptionValueST(manualLootTargetNotify)
    endEvent

    event OnDefaultST()
        manualLootTargetNotify = true
        SetToggleOptionValueST(manualLootTargetNotify)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_MANUAL_LOOT_TARGET_NOTIFY"))
    endEvent
endState

state whiteListTargetNotify
    event OnSelectST()
        whiteListTargetNotify = !(whiteListTargetNotify as bool)
        SetToggleOptionValueST(whiteListTargetNotify)
    endEvent

    event OnDefaultST()
        whiteListTargetNotify = false
        SetToggleOptionValueST(whiteListTargetNotify)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_WHITELIST_TARGET_NOTIFY"))
    endEvent
endState

state disableDuringCombat
    event OnSelectST()
        disableDuringCombat = !disableDuringCombat
        SetToggleOptionValueST(disableDuringCombat)
    endEvent

    event OnDefaultST()
        disableDuringCombat = false
        SetToggleOptionValueST(disableDuringCombat)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_DISABLE_INCOMBAT"))
    endEvent
endState

state disableWhileWeaponIsDrawn
    event OnSelectST()
        disableWhileWeaponIsDrawn = !disableWhileWeaponIsDrawn
        SetToggleOptionValueST(disableWhileWeaponIsDrawn)
    endEvent

    event OnDefaultST()
        disableWhileWeaponIsDrawn = false
        SetToggleOptionValueST(disableWhileWeaponIsDrawn)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_DISABLE_DRAWINGWEAPON"))
    endEvent
endState

state disableWhileConcealed
    event OnSelectST()
        disableWhileConcealed = !disableWhileConcealed
        SetToggleOptionValueST(disableWhileConcealed)
    endEvent

    event OnDefaultST()
        disableWhileConcealed = false
        SetToggleOptionValueST(disableWhileConcealed)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_DISABLE_IF_CONCEALED"))
    endEvent
endState

state playContainerAnimation
    event OnSelectST()
        int size = s_playContainerAnimationArray.length
        playContainerAnimation = CycleInt(playContainerAnimation, size)
        SetTextOptionValueST(s_playContainerAnimationArray[playContainerAnimation])
    endEvent

    event OnDefaultST()
        playContainerAnimation = 2
        SetTextOptionValueST(s_playContainerAnimationArray[playContainerAnimation])
    endEvent

    event OnHighlightST()
        string trans = GetTranslation("$SHSE_DESC_CONTAINER_ANIMATION")
        SetInfoText(trans)
    endEvent
endState

state preventPopulationCenterLooting
    event OnSelectST()
        int size = s_populationCenterArray.length
        preventPopulationCenterLooting = CycleInt(preventPopulationCenterLooting, size)
        SetTextOptionValueST(s_populationCenterArray[preventPopulationCenterLooting])
    endEvent

    event OnDefaultST()
        preventPopulationCenterLooting = 0
        SetTextOptionValueST(s_populationCenterArray[preventPopulationCenterLooting])
    endEvent

    event OnHighlightST()
        string trans = GetTranslation("$SHSE_DESC_POPULATION_CENTER_PREVENTION")
        SetInfoText(trans)
    endEvent
endState

state notifyLocationChange
    event OnSelectST()
        notifyLocationChange = !notifyLocationChange
        SetToggleOptionValueST(notifyLocationChange)
    endEvent

    event OnDefaultST()
        notifyLocationChange = false
        SetToggleOptionValueST(notifyLocationChange)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_NOTIFY_LOCATION_CHANGE"))
    endEvent
endState

Function SetCollectionsUIFlags()
    if collectionsEnabled
        SetOptionFlagsST(OPTION_FLAG_NONE, false, "chooseCollectionGroup")
        SetOptionFlagsST(OPTION_FLAG_NONE, false, "groupCollectibleAction")
        SetOptionFlagsST(OPTION_FLAG_NONE, false, "groupCollectionAddNotify")
        SetOptionFlagsST(OPTION_FLAG_NONE, false, "groupCollectDuplicates")
        SetOptionFlagsST(OPTION_FLAG_NONE, false, "chooseCollectionIndex")
        SetOptionFlagsST(OPTION_FLAG_NONE, false, "collectibleAction")
        SetOptionFlagsST(OPTION_FLAG_NONE, false, "collectionAddNotify")
        SetOptionFlagsST(OPTION_FLAG_NONE, false, "collectDuplicates")
        SetOptionFlagsST(OPTION_FLAG_NONE, false, "itemsCollected")
    else
        SetOptionFlagsST(OPTION_FLAG_DISABLED, false, "chooseCollectionGroup")
        SetOptionFlagsST(OPTION_FLAG_DISABLED, false, "groupCollectibleAction")
        SetOptionFlagsST(OPTION_FLAG_DISABLED, false, "groupCollectionAddNotify")
        SetOptionFlagsST(OPTION_FLAG_DISABLED, false, "groupCollectDuplicates")
        SetOptionFlagsST(OPTION_FLAG_DISABLED, false, "chooseCollectionIndex")
        SetOptionFlagsST(OPTION_FLAG_DISABLED, false, "collectibleAction")
        SetOptionFlagsST(OPTION_FLAG_DISABLED, false, "collectionAddNotify")
        SetOptionFlagsST(OPTION_FLAG_DISABLED, false, "collectDuplicates")
        SetOptionFlagsST(OPTION_FLAG_DISABLED, false, "itemsCollected")
    endif
EndFunction

state collectionsEnabled
    event OnSelectST()
        collectionsEnabled = !collectionsEnabled
        SetToggleOptionValueST(collectionsEnabled)
        SetCollectionsUIFlags()
    endEvent

    event OnDefaultST()
        collectionsEnabled = false
        SetToggleOptionValueST(collectionsEnabled)
        SetCollectionsUIFlags()
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_COLLECTIONS_ENABLED"))
        SetCollectionsUIFlags()
    endEvent
endState

state chooseCollectionGroup
    event OnMenuOpenST()
        SetMenuDialogStartIndex(collectionGroup)
        SetMenuDialogDefaultIndex(collectionGroup)
        SetMenuDialogOptions(collectionGroupNames)
        SetMenuOptionValueST(collectionGroupNames[collectionGroup])
    endEvent

    event OnMenuAcceptST(int index)
        SetMenuOptionValueST(collectionGroupNames[index])
        collectionGroup = index
        PopulateCollectionsForGroup(True, collectionGroupNames[index])
    endEvent

    event OnDefaultST()
        collectionGroup = 0
        SetMenuOptionValueST(collectionGroupNames[collectionGroup])
        PopulateCollectionsForGroup(True, collectionGroupNames[collectionGroup])
    endEvent

    event OnHighlightST()
        SetInfoText(collectionGroupFiles[collectionGroup])
    endEvent
endState

state chooseCollectionIndex
    event OnMenuOpenST()
        SetMenuDialogStartIndex(collectionIndex)
        SetMenuDialogDefaultIndex(collectionIndex)
        SetMenuDialogOptions(collectionNames)
        SetMenuOptionValueST(collectionNames[collectionIndex])
        GetCollectionPolicy(collectionNames[collectionIndex])
    endEvent

    event OnMenuAcceptST(int index)
        collectionIndex = index
        SetMenuOptionValueST(collectionNames[collectionIndex])
        GetCollectionPolicy(collectionNames[collectionIndex])
    endEvent

    event OnDefaultST()
        collectionIndex = 0
        SetMenuOptionValueST(collectionNames[collectionIndex])
        GetCollectionPolicy(collectionNames[collectionIndex])
    endEvent

    event OnHighlightST()
        SetInfoText(collectionDescriptions[collectionIndex])
    endEvent
endState

state groupCollectibleAction
    event OnSelectST()
        int size = s_collectibleActions.length
        groupCollectibleAction = CycleInt(groupCollectibleAction, size)
        SetTextOptionValueST(s_collectibleActions[groupCollectibleAction])
        PutCollectionGroupAction(collectionGroupNames[collectionGroup], groupCollectibleAction)
    endEvent

    event OnDefaultST()
        groupCollectibleAction = 2
        SetTextOptionValueST(s_collectibleActions[groupCollectibleAction])
        PutCollectionGroupAction(collectionGroupNames[collectionGroup], groupCollectibleAction)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_GROUP_COLLECTIBLE_ACTION"))
    endEvent
endState

state groupCollectionAddNotify
    event OnSelectST()
        groupCollectionAddNotify = !groupCollectionAddNotify
        SetToggleOptionValueST(groupCollectionAddNotify)
        PutCollectionGroupNotifies(collectionGroupNames[collectionGroup], groupCollectionAddNotify)
    endEvent

    event OnDefaultST()
        groupCollectionAddNotify = false
        SetToggleOptionValueST(groupCollectionAddNotify)
        PutCollectionGroupNotifies(collectionGroupNames[collectionGroup], groupCollectionAddNotify)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_GROUP_COLLECTION_ADD_NOTIFY"))
    endEvent
endState

state groupCollectDuplicates
    event OnSelectST()
        groupCollectDuplicates = !groupCollectDuplicates
        SetToggleOptionValueST(groupCollectDuplicates)
        PutCollectionGroupAllowsRepeats(collectionGroupNames[collectionGroup], groupCollectDuplicates)
    endEvent

    event OnDefaultST()
        groupCollectDuplicates = false
        SetToggleOptionValueST(groupCollectDuplicates)
        PutCollectionGroupAllowsRepeats(collectionGroupNames[collectionGroup], groupCollectDuplicates)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_GROUP_COLLECT_DUPLICATES"))
    endEvent
endState

state collectibleAction
    event OnSelectST()
        int size = s_collectibleActions.length
        collectibleAction = CycleInt(collectibleAction, size)
        SetTextOptionValueST(s_collectibleActions[collectibleAction])
        PutCollectionAction(collectionGroupNames[collectionGroup], collectionNames[collectionIndex], collectibleAction)
    endEvent

    event OnDefaultST()
        collectibleAction = 2
        SetTextOptionValueST(s_collectibleActions[collectibleAction])
        PutCollectionAction(collectionGroupNames[collectionGroup], collectionNames[collectionIndex], collectibleAction)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_COLLECTIBLE_ACTION"))
    endEvent
endState

state collectionAddNotify
    event OnSelectST()
        collectionAddNotify = !collectionAddNotify
        SetToggleOptionValueST(collectionAddNotify)
        PutCollectionNotifies(collectionGroupNames[collectionGroup], collectionNames[collectionIndex], collectionAddNotify)
    endEvent

    event OnDefaultST()
        collectionAddNotify = false
        SetToggleOptionValueST(collectionAddNotify)
        PutCollectionNotifies(collectionGroupNames[collectionGroup], collectionNames[collectionIndex], collectionAddNotify)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_COLLECTION_ADD_NOTIFY"))
    endEvent
endState

state collectDuplicates
    event OnSelectST()
        collectDuplicates = !collectDuplicates
        SetToggleOptionValueST(collectDuplicates)
        PutCollectionAllowsRepeats(collectionGroupNames[collectionGroup], collectionNames[collectionIndex], collectDuplicates)
    endEvent

    event OnDefaultST()
        collectDuplicates = false
        SetToggleOptionValueST(collectDuplicates)
        PutCollectionAllowsRepeats(collectionGroupNames[collectionGroup], collectionNames[collectionIndex], collectDuplicates)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_COLLECT_DUPLICATES"))
    endEvent
endState

state itemsCollected
    event OnSelectST()
    endEvent

    event OnDefaultST()
    endEvent

    event OnHighlightST()
    endEvent
endState

Function CheckAdventuresPower()
    if !thisPlayer
        return
    endif
    if adventuresEnabled
        thisPlayer.AddSpell(AdventurersInstinctPower)
    else
        thisPlayer.RemoveSpell(AdventurersInstinctPower)
    endIf
EndFunction

Function SetAdventuresStatus(bool inMCM)
    CheckAdventuresPower()
    ResetAdventureType(inMCM)
EndFunction

Function CheckFortunePower()
    if !thisPlayer
        return
    endif
    if fortuneHuntingEnabled
        thisPlayer.AddSpell(FortuneHuntersInstinctPower)
    else
        thisPlayer.RemoveSpell(FortuneHuntersInstinctPower)
    endIf
EndFunction

state adventuresEnabled
    event OnSelectST()
        adventuresEnabled = !adventuresEnabled
        SetToggleOptionValueST(adventuresEnabled)
        SetAdventuresStatus(True)
    endEvent

    event OnDefaultST()
        adventuresEnabled = false
        SetToggleOptionValueST(adventuresEnabled)
        SetAdventuresStatus(True)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_ADVENTURE_ENABLED"))
    endEvent
endState

state chooseAdventureType
    event OnMenuOpenST()
        SetMenuDialogStartIndex(adventureType)
        SetMenuDialogDefaultIndex(adventureType)
        SetMenuDialogOptions(adventureTypeNames)
        SetMenuOptionValueST(adventureTypeNames[adventureType])
    endEvent

    event OnMenuAcceptST(int index)
        adventureType = index
        SetMenuOptionValueST(adventureTypeNames[adventureType])
        PopulateAdventureWorlds()
        ResetAdventureWorld(True)
    endEvent

    event OnDefaultST()
        adventureType = 0
        SetMenuOptionValueST(adventureTypeNames[adventureType])
        PopulateAdventureWorlds()
        ResetAdventureWorld(True)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_CHOOSE_ADVENTURE_TYPE"))
    endEvent
endState

Function ResetAdventureType(bool inMCM)
    if inMCM
        if adventuresEnabled
            SetOptionFlagsST(OPTION_FLAG_NONE, false, "chooseAdventureType")
        else
            SetOptionFlagsST(OPTION_FLAG_DISABLED, false, "chooseAdventureType")
        endIf
        SetMenuOptionValueST("", false, "chooseAdventureType")
    endif
    adventureType = 0
    worldCount = 0
    ResetAdventureWorld(inMCM)
EndFunction

Function ResetAdventureWorld(bool inMCM)
    if inMCM
        if worldCount > 0
            SetOptionFlagsST(OPTION_FLAG_NONE, false, "chooseAdventureWorld")
            SetMenuOptionValueST(worldNames[worldIndex], false, "chooseAdventureWorld")
        else
            SetOptionFlagsST(OPTION_FLAG_DISABLED, false, "chooseAdventureWorld")
            SetMenuOptionValueST("", false, "chooseAdventureWorld")
        endIf
    endIf
    ResetAdventureActive(inMCM, OPTION_FLAG_DISABLED)
EndFunction

Function ResetAdventureActive(bool inMCM, int flags)
    adventureActive = false
    ClearAdventureTarget()
    if inMCM
        SetOptionFlagsST(flags, false, "chooseAdventureActive")
        SetToggleOptionValueST(adventureActive, false, "chooseAdventureActive")
    endif
EndFunction

state chooseAdventureWorld
    event OnMenuOpenST()
        SetMenuDialogStartIndex(worldIndex)
        SetMenuDialogDefaultIndex(worldIndex)
        SetMenuDialogOptions(worldNames)
        SetMenuOptionValueST(worldNames[worldIndex])
    endEvent

    event OnMenuAcceptST(int index)
        worldIndex = index
        ResetAdventureActive(True, OPTION_FLAG_NONE)
        SetMenuOptionValueST(worldNames[worldIndex])
    endEvent

    event OnDefaultST()
        worldIndex = 0
        ResetAdventureActive(True, OPTION_FLAG_NONE)
        SetMenuOptionValueST(worldNames[worldIndex])
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_CHOOSE_ADVENTURE_WORLD"))
    endEvent
endState

state chooseAdventureActive
    event OnSelectST()
        adventureActive = !adventureActive
        SetToggleOptionValueST(adventureActive)
        if adventureActive
            SetAdventureTarget(worldIndex)
        else
            ClearAdventureTarget()
        endIf
    endEvent

    event OnDefaultST()
        adventureActive = false
        SetToggleOptionValueST(adventureActive)
        ClearAdventureTarget()
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_CHOOSE_ADVENTURE_ACTIVE"))
    endEvent
endState

Function SetFortuneHuntOptions()
    int flags = OPTION_FLAG_NONE
    ; reset to default if now locked
    if !fortuneHuntingEnabled
        flags = OPTION_FLAG_DISABLED
    endIf
    SetOptionFlagsST(flags, false, "fortuneHuntItemState")
    SetOptionFlagsST(flags, false, "fortuneHuntNPCState")
    SetOptionFlagsST(flags, false, "fortuneHuntContainerState")
EndFunction

state fortuneHuntingEnabled
    event OnSelectST()
        fortuneHuntingEnabled = !fortuneHuntingEnabled
        SetToggleOptionValueST(fortuneHuntingEnabled)
        CheckFortunePower()
        SetFortuneHuntOptions()
    endEvent

    event OnDefaultST()
        fortuneHuntingEnabled = false
        SetToggleOptionValueST(fortuneHuntingEnabled)
        CheckFortunePower()
        SetFortuneHuntOptions()
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_LOOT_SENSE_ENABLED"))
    endEvent
endState

state fortuneHuntItemState
    event OnSelectST()
        fortuneHuntItem = !fortuneHuntItem
        SetToggleOptionValueST(fortuneHuntItem)
    endEvent

    event OnDefaultST()
        fortuneHuntItem = false
        SetToggleOptionValueST(fortuneHuntItem)
    endEvent
endState

state fortuneHuntNPCState
    event OnSelectST()
        fortuneHuntNPC = !fortuneHuntNPC
        SetToggleOptionValueST(fortuneHuntNPC)
    endEvent

    event OnDefaultST()
        fortuneHuntNPC = false
        SetToggleOptionValueST(fortuneHuntNPC)
    endEvent
endState

state fortuneHuntContainerState
    event OnSelectST()
        fortuneHuntContainer = !fortuneHuntContainer
        SetToggleOptionValueST(fortuneHuntContainer)
    endEvent

    event OnDefaultST()
        fortuneHuntContainer = false
        SetToggleOptionValueST(fortuneHuntContainer)
    endEvent
endState

Function SetGlowColourStatus()
    int flags = OPTION_FLAG_NONE
    ; reset to default if now locked
    if !unlockGlowColours
        flags = OPTION_FLAG_DISABLED
        ResetShaders()
    endIf
    int index = 0
    while index < s_glowReasonArray.length
        SetOptionFlags(id_glowReasonArray[index], flags)
        ; reset to default if now locked
        if !unlockGlowColours
            SetTextOptionValue(id_glowReasonArray[index], s_shaderColourArray[(glowReasonSettingArray[index] as int)])
        endIf
        index += 1
    endWhile
EndFunction

state unlockGlowColours
    event OnSelectST()
        unlockGlowColours = !unlockGlowColours
        SetToggleOptionValueST(unlockGlowColours)
        SetGlowColourStatus()
    endEvent

    event OnDefaultST()
        unlockGlowColours = false
        SetToggleOptionValueST(unlockGlowColours)
        SetGlowColourStatus()
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_GLOW_COLOURS_UNLOCKED"))
    endEvent
endState

Function CreateSagaForDay()
    currentSagaDayName = TimelineDayName(currentSagaDay)
    sagaDayPageCount = PageCountForDay()
    currentSagaDayPage = 1
    ;enable 'open at page' for current day excerpt
    SetOptionFlagsST(OPTION_FLAG_NONE, false, "SagaDayPageState")
EndFunction

Function DisplaySagaFromPage(int pageNumber)
    while pageNumber <= sagaDayPageCount
        string okPrompt = Replace(GetTranslation("$SHSE_NEXT_PAGE"), "{PAGEDESCRIPTOR}", pageNumber + "/" + sagaDayPageCount)
        bool result = ShowMessage(GetSagaDayPage(pageNumber), true, okPrompt, "$SHSE_CLOSE_BOOK")
        if result
            pageNumber += 1
            if pageNumber > sagaDayPageCount
                pageNumber = 1
            endIf
        else
            return
        endIf
    endWhile
EndFunction

Function ResetSagaDay()
    SetOptionFlagsST(OPTION_FLAG_DISABLED, false, "SagaDayPageState")
    SetSliderOptionValueST(1, "{0}", false, "SagaDayPageState")
    currentSagaDayPage = 1
EndFunction

state SagaDayState
    event OnSliderOpenST()
        SetSliderDialogStartValue(currentSagaDay)
        SetSliderDialogDefaultValue(sagaDayCount)
        SetSliderDialogRange(1, sagaDayCount)
        SetSliderDialogInterval(1)
        ResetSagaDay()
    endEvent

    event OnSliderAcceptST(float value)
        currentSagaDay = value as int
        SetSliderOptionValueST(currentSagaDay)
        CreateSagaForDay()
    endEvent

    event OnDefaultST()
        currentSagaDay = sagaDayCount
        SetSliderOptionValueST(currentSagaDay)
        CreateSagaForDay()
    endEvent

    event OnHighlightST()
        SetInfoText(currentSagaDayName)
    endEvent
endState

state SagaDayPageState
    event OnSliderOpenST()
        SetSliderDialogStartValue(currentSagaDayPage)
        SetSliderDialogDefaultValue(1)
        SetSliderDialogRange(1, sagaDayPageCount)
        SetSliderDialogInterval(1)
    endEvent

    event OnSliderAcceptST(float value)
        currentSagaDayPage = value as int
        SetSliderOptionValueST(currentSagaDayPage)
        DisplaySagaFromPage(currentSagaDayPage)
    endEvent

    event OnDefaultST()
        currentSagaDayPage = 1
        SetSliderOptionValueST(currentSagaDayPage)
        DisplaySagaFromPage(currentSagaDayPage)
    endEvent

    event OnHighlightST()
        SetInfoText(currentSagaDayName)
    endEvent
endState
