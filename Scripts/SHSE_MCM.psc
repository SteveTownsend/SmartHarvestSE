Scriptname SHSE_MCM extends SKI_ConfigBase  

Import SHSE_PluginProxy

SHSE_EventsAlias Property eventScript Auto
GlobalVariable Property g_LootingEnabled Auto

; check for first init for this playthrough
GlobalVariable Property g_InitComplete Auto

int type_Common = 1
int type_Harvest = 2
int type_Config = 1
int type_ItemObject = 2
int type_Container = 3
int type_Deadbody = 4
int type_ValueWeight = 5

; object types must be in sync with the native DLL
int objType_Flora
int objType_Critter
int objType_Septim
int objType_Key
int objType_Soulgem
int objType_LockPick
int objType_Ammo
int objType_Mine

bool enableHarvest
bool enableLootContainer
int enableLootDeadbody
string[] s_lootDeadBodyArray

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

int iniSaveLoad
string[] s_iniSaveLoadArray
int questObjectScope
string[] s_questObjectScopeArray
int crimeCheckNotSneaking
string[] s_crimeCheckNotSneakingArray
int crimeCheckSneaking
string[] s_crimeCheckSneakingArray
int playerBelongingsLoot
string[] s_specialObjectHandlingArray
string[] s_behaviorToggleArray
int playContainerAnimation
string[] s_playContainerAnimationArray

int questObjectLoot
int lockedChestLoot
int bossChestLoot

bool enchantItemGlow
bool manualLootTargetNotify

bool disableDuringCombat
bool disableWhileWeaponIsDrawn
bool disableWhileConcealed

int[] id_objectSettingArray
float[] objectSettingArray

int valueWeightDefault
int valueWeightDefaultDefault
int maxMiningItems
int maxMiningItemsDefault

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

; Current Collection Group state
int groupCollectibleAction
bool groupCollectionAddNotify
bool groupCollectDuplicates

int[] id_valueWeightArray
float[] valueWeightSettingArray

String[] s_behaviorArray
String[] s_ammoBehaviorArray
String[] s_objectTypeNameArray

int[] id_whiteList_array
Form[] whitelist_form_array
String[] whiteList_name_array
bool[] whiteList_flag_array

int[] id_blackList_array
Form[] blacklist_form_array
String[] blackList_name_array
bool[] blackList_flag_array

Actor player

int Function CycleInt(int num, int max)
    int result = num + 1
    if (result >= max)
        result = 0
    endif
    return result
endFunction

function updateMaxMiningItems(int maxItems)
    maxMiningItems = maxItems
    eventScript.updateMaxMiningItems(maxItems)
endFunction

float[] function GetSettingToObjectArray(int section1, int section2)
    int index = 0
    float[] result = New float[32]
    while (index < 32)
        result[index] = GetSettingObjectArrayEntry(section1, section2, index)
        ;DebugTrace("Config setting " + section1 + "/" + section2 + "/" + index + " = " + result[index])
        index += 1
    endWhile
    return result
endFunction

Function PutSettingObjectArray(int section_first, int section_second, int listLength, float[] values)
    int index = 1
    while index < listLength
        PutSettingObjectArrayEntry(section_first, section_second, index, values[index])
        index += 1
    endWhile
EndFunction

function ApplySettingsFromFile()
    enableHarvest = GetSetting(type_Common, type_Config, "EnableHarvest") as bool
    enableLootContainer = GetSetting(type_Common, type_Config, "EnableLootContainer") as bool
    enableLootDeadbody = GetSetting(type_Common, type_Config, "EnableLootDeadbody") as int
    unencumberedInCombat = GetSetting(type_Common, type_Config, "UnencumberedInCombat") as bool
    unencumberedInPlayerHome = GetSetting(type_Common, type_Config, "UnencumberedInPlayerHome") as bool
    unencumberedIfWeaponDrawn = GetSetting(type_Common, type_Config, "UnencumberedIfWeaponDrawn") as bool
    ;DebugTrace("ApplySettingsFromFile - unencumberedIfWeaponDrawn " + unencumberedIfWeaponDrawn)
    pauseHotkeyCode = GetSetting(type_Common, type_Config, "PauseHotkeyCode") as int
    whiteListHotkeyCode = GetSetting(type_Common, type_Config, "WhiteListHotkeyCode") as int
    blackListHotkeyCode = GetSetting(type_Common, type_Config, "BlackListHotkeyCode") as int
    preventPopulationCenterLooting = GetSetting(type_Common, type_Config, "PreventPopulationCenterLooting") as int
    notifyLocationChange = GetSetting(type_Common, type_Config, "NotifyLocationChange") as bool

    radius = GetSetting(type_Harvest, type_Config, "RadiusFeet") as int
    interval = GetSetting(type_Harvest, type_Config, "IntervalSeconds") as float
    radiusIndoors = GetSetting(type_Harvest, type_Config, "IndoorsRadiusFeet") as int
    intervalIndoors = GetSetting(type_Harvest, type_Config, "IndoorsIntervalSeconds") as float

    disableDuringCombat = GetSetting(type_Harvest, type_Config, "DisableDuringCombat") as bool
    disableWhileWeaponIsDrawn = GetSetting(type_Harvest, type_Config, "DisableWhileWeaponIsDrawn") as bool
    disableWhileConcealed = GetSetting(type_Harvest, type_Config, "DisableWhileConcealed") as bool

    crimeCheckNotSneaking = GetSetting(type_Harvest, type_Config, "CrimeCheckNotSneaking") as int
    crimeCheckSneaking = GetSetting(type_Harvest, type_Config, "CrimeCheckSneaking") as int

    questObjectLoot = GetSetting(type_Harvest, type_Config, "QuestObjectLoot") as int
    questObjectScope = GetSetting(type_Harvest, type_Config, "QuestObjectScope") as int
    lockedChestLoot = GetSetting(type_Harvest, type_Config, "LockedChestLoot") as int
    bossChestLoot = GetSetting(type_Harvest, type_Config, "BossChestLoot") as int
    enchantItemGlow = GetSetting(type_Harvest, type_Config, "EnchantItemGlow") as bool
    valuableItemLoot = GetSetting(type_Harvest, type_Config, "valuableItemLoot") as int
    valuableItemThreshold = GetSetting(type_Harvest, type_Config, "ValuableItemThreshold") as int
    playerBelongingsLoot = GetSetting(type_Harvest, type_Config, "PlayerBelongingsLoot") as int
    playContainerAnimation = GetSetting(type_Harvest, type_Config, "PlayContainerAnimation") as int

    manualLootTargetNotify = GetSetting(type_Harvest, type_Config, "ManualLootTargetNotify") as bool
    valueWeightDefault = GetSetting(type_Harvest, type_Config, "ValueWeightDefault") as int
    updateMaxMiningItems(GetSetting(type_Harvest, type_Config, "MaxMiningItems") as int)

    verticalRadiusFactor = GetSetting(type_Harvest, type_Config, "VerticalRadiusFactor") as float
    doorsPreventLooting = GetSetting(type_Harvest, type_Config, "DoorsPreventLooting") as int

    objectSettingArray = GetSettingToObjectArray(type_Harvest, type_ItemObject)
    valueWeightSettingArray = GetSettingToObjectArray(type_Harvest, type_ValueWeight)

    collectionsEnabled = GetSetting(type_Common, type_Config, "CollectionsEnabled") as bool

endFunction

;Seed defaults from the INI file, first time only - not repeated when user starts new game
function CheckFirstTimeEver()
    ;DebugTrace("CheckFirstTimeEver start")
    int doneInit = g_InitComplete.GetValue() as int
    if doneInit == 0
        ;DebugTrace("CheckFirstTimeEver - init required")
        AllocateItemCategoryArrays()

        LoadIniFile(True)
        ApplySettingsFromFile()

        g_InitComplete.SetValue(1)
    endif
    ;DebugTrace("FirstTimeEver finished")
endFunction

bool Function ManagesCarryWeight()
    return unencumberedInCombat || unencumberedInPlayerHome || unencumberedIfWeaponDrawn
endFunction

; push current settings to plugin and event handler script
Function ApplySetting(bool reload)

    ;DebugTrace("  MCM ApplySetting start")

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

    PutSetting(type_Harvest, type_Config, "QuestObjectScope", questObjectScope as float)
    PutSetting(type_Harvest, type_Config, "CrimeCheckNotSneaking", crimeCheckNotSneaking as float)
    PutSetting(type_Harvest, type_Config, "CrimeCheckSneaking", crimeCheckSneaking as float)
    PutSetting(type_Harvest, type_Config, "PlayerBelongingsLoot", playerBelongingsLoot as float)
    PutSetting(type_Harvest, type_Config, "PlayContainerAnimation", playContainerAnimation as float)

    PutSetting(type_Harvest, type_Config, "QuestObjectLoot", questObjectLoot as float)
    PutSetting(type_Harvest, type_Config, "EnchantItemGlow", enchantItemGlow as float)
    PutSetting(type_Harvest, type_Config, "valuableItemLoot", valuableItemLoot as float)
    PutSetting(type_Harvest, type_Config, "ValuableItemThreshold", valuableItemThreshold as float)
    PutSetting(type_Harvest, type_Config, "LockedChestLoot", lockedChestLoot as float)
    PutSetting(type_Harvest, type_Config, "BossChestLoot", bossChestLoot as float)
    PutSetting(type_Harvest, type_Config, "ManualLootTargetNotify", manualLootTargetNotify as float)

    PutSetting(type_Harvest, type_Config, "DisableDuringCombat", disableDuringCombat as float)
    PutSetting(type_Harvest, type_Config, "DisableWhileWeaponIsDrawn", disableWhileWeaponIsDrawn as float)
    PutSetting(type_Harvest, type_Config, "DisableWhileConcealed", disableWhileConcealed as float)

    PutSetting(type_Harvest, type_Config, "VerticalRadiusFactor", verticalRadiusFactor as float)
    PutSetting(type_Harvest, type_Config, "DoorsPreventLooting", doorsPreventLooting as float)

    PutSettingObjectArray(type_Harvest, type_ItemObject, 32, objectSettingArray)

    PutSetting(type_Harvest, type_Config, "ValueWeightDefault", valueWeightDefault as float)
    PutSetting(type_Harvest, type_Config, "MaxMiningItems", maxMiningItems as float)
    PutSettingObjectArray(type_Harvest, type_ValueWeight, 32, valueWeightSettingArray)

    PutSetting(type_Common, type_Config, "CollectionsEnabled", collectionsEnabled as float)

    ; seed looting scan enabled according to configured settings
    bool isEnabled = enableHarvest || enableLootContainer || enableLootDeadbody > 0 || unencumberedInCombat || unencumberedInPlayerHome || unencumberedIfWeaponDrawn || collectionsEnabled
    ;DebugTrace("isEnabled? " + isEnabled + "from flags:" + enableHarvest + " " + enableLootContainer + " " + enableLootDeadbody + " " + unencumberedInCombat + " " + unencumberedInPlayerHome + " " + unencumberedIfWeaponDrawn)
    ;DebugTrace("Collections enabled? " + " collectionsEnabled)
    if (isEnabled)
        g_LootingEnabled.SetValue(1)
        eventScript.SetScanActive()
    else
        g_LootingEnabled.SetValue(0)
        eventScript.SetScanInactive()
    endif

    ; correct for any weight adjustments saved into this file, plugin will reinstate if/as needed
    ; Do this before plugin becomes aware of player home list
    player = Game.GetPlayer()
    eventScript.SetPlayer(player)
    ; Only adjust weight if we are in any way responsible for it
    if ManagesCarryWeight()
        eventScript.RemoveCarryWeightDelta()
    endIf
    ; hard code for oreVein pickup type, yuck
    ;DebugTrace("oreVein setting " + objectSettingArray[31] as int)
    eventScript.ApplySetting(reload, objectSettingArray[31] as int)

    ; do this last so plugin state is in sync   
    if (isEnabled)
        Debug.Notification(Replace(GetTranslation("$SHSE_ENABLE"), "{VERSION}", GetPluginVersion()))
        AllowSearch(True)
    Else
        Debug.Notification(Replace(GetTranslation("$SHSE_DISABLE"), "{VERSION}", GetPluginVersion()))
        DisallowSearch(True)
    EndIf

    ;DebugTrace("  MCM ApplySetting finished")
endFunction

Function SetOreVeinChoices()
    s_behaviorToggleArray = New String[3]
    s_behaviorToggleArray[0] = "$SHSE_DONT_PICK_UP"
    s_behaviorToggleArray[1] = "$SHSE_PICK_UP_IF_NOT_BYOH"
    s_behaviorToggleArray[2] = "$SHSE_PICK_UP"
EndFunction

Function SetDeadBodyChoices()
    s_lootDeadBodyArray = New String[3]
    s_lootDeadBodyArray[0] = "$SHSE_DONT_PICK_UP"
    s_lootDeadBodyArray[1] = "$SHSE_EXCLUDING_ARMOR"
    s_lootDeadBodyArray[2] = "$SHSE_PICK_UP"
EndFunction

Function AllocateItemCategoryArrays()
    id_objectSettingArray = New Int[32]
    objectSettingArray = New float[32]

    id_valueWeightArray = New Int[32]
    valueWeightSettingArray = New float[32]
EndFunction

Function CheckItemCategoryArrays()
    int doneInit = g_InitComplete.GetValue() as int
    if doneInit != 0
        ; arrays should all be in place, if not it's probably a bad save due to now-fixed bug
        if !id_objectSettingArray
            ;DebugTrace("allocate missing id_objectSettingArray")
            id_objectSettingArray = New Int[32]
        endif
        if !objectSettingArray
            ;DebugTrace("allocate missing objectSettingArray")
            objectSettingArray = New float[32]
        endif
        if !id_valueWeightArray
            ;DebugTrace("allocate missing id_valueWeightArray")
            id_valueWeightArray = New Int[32]
        endif
        if !valueWeightSettingArray
            ;DebugTrace("allocate missing valueWeightSettingArray")
            valueWeightSettingArray = New float[32]
        endif
    endIf
EndFunction

Function SetObjectTypeData()
    s_objectTypeNameArray = New String[32]

    s_objectTypeNameArray[0]  = "$SHSE_UNKNOWN"
    s_objectTypeNameArray[1]  = "$SHSE_FLORA"
    s_objectTypeNameArray[2]  = "$SHSE_CRITTER"
    s_objectTypeNameArray[3]  = "$SHSE_INGREDIENT"
    s_objectTypeNameArray[4]  = "$SHSE_SEPTIM"
    s_objectTypeNameArray[5]  = "$SHSE_GEM"
    s_objectTypeNameArray[6]  = "$SHSE_LOCKPICK"
    s_objectTypeNameArray[7]  = "$SHSE_ANIMAL_HIDE"
    s_objectTypeNameArray[8]  = "$SHSE_OREINGOT"
    s_objectTypeNameArray[9]  = "$SHSE_SOULGEM"
    s_objectTypeNameArray[10] = "$SHSE_KEY"
    s_objectTypeNameArray[11] = "$SHSE_CLUTTER"
    s_objectTypeNameArray[12] = "$SHSE_LIGHT"
    s_objectTypeNameArray[13] = "$SHSE_BOOK"
    s_objectTypeNameArray[14] = "$SHSE_SPELLBOOK"
    s_objectTypeNameArray[15] = "$SHSE_SKILLBOOK"
    s_objectTypeNameArray[16] = "$SHSE_BOOK_READ"
    s_objectTypeNameArray[17] = "$SHSE_SPELLBOOK_READ"
    s_objectTypeNameArray[18] = "$SHSE_SKILLBOOK_READ"
    s_objectTypeNameArray[19] = "$SHSE_SCROLL"
    s_objectTypeNameArray[20] = "$SHSE_AMMO"
    s_objectTypeNameArray[21] = "$SHSE_WEAPON"
    s_objectTypeNameArray[22] = "$SHSE_ENCHANTED_WEAPON"
    s_objectTypeNameArray[23] = "$SHSE_ARMOR"
    s_objectTypeNameArray[24] = "$SHSE_ENCHANTED_ARMOR"
    s_objectTypeNameArray[25] = "$SHSE_JEWELRY"
    s_objectTypeNameArray[26] = "$SHSE_ENCHANTED_JEWELRY"
    s_objectTypeNameArray[27] = "$SHSE_POTION"
    s_objectTypeNameArray[28] = "$SHSE_POISON"
    s_objectTypeNameArray[29] = "$SHSE_FOOD"
    s_objectTypeNameArray[30] = "$SHSE_DRINK"
    s_objectTypeNameArray[31] = "$SHSE_OREVEIN"
EndFunction

Function SetMiscDefaults(bool firstTime)
    ; New or clarified defaults and constants
    manualLootTargetNotify = true

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
    eventScript.UpdateMaxMiningItems(maxMiningItems)

    notifyLocationChange = false
    valuableItemLoot = 2
    valuableItemThreshold = 500
    InstallCollections()
    InstallCollectionGroupPolicy()
    InstallCollectionDescriptionsActions()
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
    s_collectibleActions = New String[3]
    s_collectibleActions[0] = "$SHSE_DONT_PICK_UP"
    s_collectibleActions[1] = "$SHSE_PICK_UP"
    s_collectibleActions[2] = "$SHSE_CONTAINER_GLOW_PERSISTENT"
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

Function InitPages()
    Pages = New String[6]
    Pages[0] = "$SHSE_RULES_DEFAULTS_PAGENAME"
    Pages[1] = "$SHSE_SPECIALS_REALISM_PAGENAME"
    Pages[2] = "$SHSE_SHARED_SETTINGS_PAGENAME"
    Pages[3] = "$SHSE_WHITELIST_PAGENAME"
    Pages[4] = "$SHSE_BLACKLIST_PAGENAME"
    Pages[5] = "$SHSE_COLLECTIONS_PAGENAME"
EndFunction

Function InitSettingsFileOptions()
    s_iniSaveLoadArray = New String[4]
    iniSaveLoad = 0
    s_iniSaveLoadArray[0] = "$SHSE_PRESET_DO_NOTHING"
    s_iniSaveLoadArray[1] = "$SHSE_PRESET_RESTORE"
    s_iniSaveLoadArray[2] = "$SHSE_PRESET_STORE"
    s_iniSaveLoadArray[3] = "$SHSE_PRESET_RESET"
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

    s_questObjectScopeArray = New String[2]
    s_questObjectScopeArray[0] = "$SHSE_QUEST_RELATED"
    s_questObjectScopeArray[1] = "$SHSE_QUEST_FLAG_ONLY"

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

    s_behaviorArray = New String[5]
    s_behaviorArray[0] = "$SHSE_DONT_PICK_UP"
    s_behaviorArray[1] = "$SHSE_PICK_UP_W/O_MSG"
    s_behaviorArray[2] = "$SHSE_PICK_UP_W/MSG"
    s_behaviorArray[3] = "$SHSE_PICK_UP_V/W_W/O_MSG"
    s_behaviorArray[4] = "$SHSE_PICK_UP_V/W_W/MSG"

    InstallDamageLootOptions()

    eventScript.whitelist_form = Game.GetFormFromFile(0x801, "SmartHarvestSE.esp") as Formlist
    eventScript.blacklist_form = Game.GetFormFromFile(0x802, "SmartHarvestSE.esp") as Formlist

    SetOreVeinChoices()
    SetDeadBodyChoices()
    SetMiscDefaults(true)
    InstallVerticalRadiusAndDoorRule()
    SetObjectTypeData()

    ;DebugTrace("** OnConfigInit finished **")
endEvent

int function GetVersion()
    return 35
endFunction

; called when mod is _upgraded_ mid-playthrough
Event OnVersionUpdate(int a_version)
    ;DebugTrace("OnVersionUpdate start" + a_version)
    if (a_version >= 25 && CurrentVersion < 25)
        ; clean up after release with bad upgrade/install workflow and MCM bugs
        ; logic required to support existing saves, as well as the update per se
        Debug.Trace(self + ": Updating script to version " + a_version)
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
    	
    	eventScript.SyncNativeDataTypes()
        eventScript.SetShaders()
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
        InstallCollectionDescriptionsActions()
    endIf
    if (a_version >= 35 && CurrentVersion < 35)
        ;fixes missing entry in plugin
        eventScript.SyncVeinResourceTypes()
    endIf
    ;DebugTrace("OnVersionUpdate finished" + a_version)
endEvent

; when mod is applied mid-playthrough, this gets called after OnVersionUpdate/OnConfigInit
Event OnGameReload()
    parent.OnGameReload()
    ApplySetting(true)
endEvent

Event OnConfigOpen()
    ;DebugTrace("OnConfigOpen")
    InitPages()
    
    int index = 0
    int max_size = 0
    
    max_size = eventScript.whitelist_form.GetSize()
    if (max_size > 0)
        id_whiteList_array = Utility.CreateIntArray(max_size)
        whitelist_form_array = eventScript.whitelist_form.toArray()
        whiteList_name_array = Utility.CreateStringArray(max_size, "")
        whiteList_flag_array = Utility.CreateBoolArray(max_size, false)
        
        index = 0
        while(index < max_size)
            if (whitelist_form_array[index])
                whiteList_name_array[index] = GetNameForListForm(whitelist_form_array[index])
                whiteList_flag_array[index] = true
            endif
            index += 1
        endWhile
    endif

    max_size = eventScript.blacklist_form.GetSize()
    if (max_size > 0)
        id_blackList_array = Utility.CreateIntArray(max_size)
        blacklist_form_array = Utility.CreateFormArray(max_size)
        blackList_name_array = Utility.CreateStringArray(max_size)
        blackList_flag_array = Utility.CreateBoolArray(max_size)
        
        index = 0
        while(index < max_size)
            blacklist_form_array[index] = eventScript.blacklist_form.GetAt(index)
            blackList_name_array[index] = GetNameForListForm(blacklist_form_array[index])
            blackList_flag_array[index] = true
            index += 1
        endWhile
    endif
endEvent

bool Function TidyListUp(Formlist m_list, form[] m_forms, bool[] m_flags, string trans)
    if (!m_list)
        return false
    endif
    int index = m_list.GetSize()
    if (index <= 0)
        return false
    endif
    
    while (index > 0)
        index -= 1
        if (!m_flags[index])
            m_list.RemoveAddedForm(m_forms[index])
            string translation = GetTranslation(trans)
            if (translation)
                translation = Replace(translation, "{ITEMNAME}", GetNameForListForm(m_forms[index]))
                if (translation)
                    Debug.Notification(translation)
                endif
            endif
        endif
    endWhile
    return true
endFunction

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
        string displayCollected = Replace(Replace(GetTranslation("$SHSE_COLLECTION_PROGRESS"), "{TOTAL}", collectionTotal), "{OBTAINED}", collectionObtained)
        SetTextOptionValueST(displayCollected, false, "itemsCollected")
EndFunction

Function GetCollectionPolicy(String collectionName)
    if collectionName != lastKnownPolicy
        collectDuplicates = CollectionAllowsRepeats(collectionGroupNames[collectionGroup], collectionName)
        collectibleAction = CollectionAction(collectionGroupNames[collectionGroup], collectionName)
        collectionAddNotify = CollectionNotifies(collectionGroupNames[collectionGroup], collectionName)
        collectionTotal = CollectionTotal(collectionGroupNames[collectionGroup], collectionName)
        collectionObtained = CollectionObtained(collectionGroupNames[collectionGroup], collectionName)
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

Function PopulateCollectionsForGroup(String groupName)
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
    SetMenuOptionValueST(collectionNames[collectionIndex], false, "chooseCollectionIndex")
    GetCollectionPolicy(collectionNames[collectionIndex])
EndFunction

Event OnConfigClose()
    ;DebugTrace("OnConfigClose")

    TidyListUp(eventScript.whitelist_form, whitelist_form_array, whiteList_flag_array, "$SHSE_WHITELIST_REMOVED")
    TidyListUp(eventScript.blacklist_form, blacklist_form_array, blackList_flag_array, "$SHSE_BLACKLIST_REMOVED")

    iniSaveLoad = 0
    ApplySetting(false)
endEvent

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
        AddKeyMapOptionST("pauseHotkeyCode", "$SHSE_PAUSE_KEY", pauseHotkeyCode)
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
        AddTextOptionST("questObjectLoot", "$SHSE_QUESTOBJECT_LOOT", s_specialObjectHandlingArray[questObjectLoot])
        AddTextOptionST("questObjectScope", "$SHSE_QUESTOBJECT_SCOPE", s_questObjectScopeArray[questObjectScope])
        AddTextOptionST("lockedChestLoot", "$SHSE_LOCKEDCHEST_LOOT", s_specialObjectHandlingArray[lockedChestLoot])
        AddTextOptionST("bossChestLoot", "$SHSE_BOSSCHEST_LOOT", s_specialObjectHandlingArray[bossChestLoot])
        AddTextOptionST("playerBelongingsLoot", "$SHSE_PLAYER_BELONGINGS_LOOT", s_specialObjectHandlingArray[playerBelongingsLoot])
        AddToggleOptionST("enchantItemGlow", "$SHSE_ENCHANTITEM_GLOW", enchantItemGlow)
        AddTextOptionST("valuableItemLoot", "$SHSE_VALUABLE_ITEM_LOOT", s_specialObjectHandlingArray[valuableItemLoot])
        AddSliderOptionST("valuableItemThreshold", "$SHSE_VALUABLE_ITEM_THRESHOLD", valuableItemThreshold as float, "$SHSE_MONEY")
        AddToggleOptionST("manualLootTargetNotify", "$SHSE_MANUAL_LOOT_TARGET_NOTIFY", manualLootTargetNotify)
        AddTextOptionST("playContainerAnimation", "$SHSE_PLAY_CONTAINER_ANIMATION", s_playContainerAnimationArray[playContainerAnimation])

;   ======================== RIGHT ========================
        SetCursorPosition(1)

        AddHeaderOption("$SHSE_MORE_REALISM_HEADER")
        AddSliderOptionST("VerticalRadiusFactorState", "$SHSE_VERTICAL_RADIUS_FACTOR", verticalRadiusFactor as float, "{2}")
        AddToggleOptionST("DoorsPreventLootingState", "$SHSE_DOORS_PREVENT_LOOTING", doorsPreventLooting as bool)

    elseif (currentPage == Pages[2]) ; object harvester
        
;   ======================== LEFT ========================
        SetCursorFillMode(TOP_TO_BOTTOM)

        AddHeaderOption("$SHSE_PICK_UP_ITEM_TYPE_HEADER")
        
        int index = 1
        while index < s_objectTypeNameArray.length ; oreVein is the last
            if (index == objType_Mine)
                id_objectSettingArray[index] = AddTextOption(s_objectTypeNameArray[index], s_behaviorToggleArray[(objectSettingArray[index] as int)])
            elseif (index == objType_Ammo)
                id_objectSettingArray[index] = AddTextOption(s_objectTypeNameArray[index], s_ammoBehaviorArray[(objectSettingArray[index] as int)])
            else
                id_objectSettingArray[index] = AddTextOption(s_objectTypeNameArray[index], s_behaviorArray[(objectSettingArray[index] as int)])
            endif
            index += 1
        endWhile

;   ======================== RIGHT ========================
        SetCursorPosition(1)

        AddHeaderOption("$SHSE_VALUE/WEIGHT_HEADER")

        index = 1
        while index < s_objectTypeNameArray.length ; oreVein is the last
            ; do not request V/W for weightless or unhandleable item types
            if (index == objType_Mine || index == objType_Septim  || index == objType_Key || index == objType_LockPick)
                AddEmptyOption()
            elseif index == objType_Ammo
                ; absolute damage
                id_valueWeightArray[index] = AddSliderOption(s_objectTypeNameArray[index], valueWeightSettingArray[index], "$SHSE_DAMAGE")
            else
                id_valueWeightArray[index] = AddSliderOption(s_objectTypeNameArray[index], valueWeightSettingArray[index], "$SHSE_V/W")
            endif
            index += 1
        endWhile
        
    elseif (currentPage == Pages[3]) ; whiteList
        AddKeyMapOptionST("whiteListHotkeyCode", "$SHSE_WHITELIST_KEY", whiteListHotkeyCode)

        int size = eventScript.whitelist_form.GetSize()
        if (size == 0)
            return
        endif

        int index = 0
        while (index < size)
            if (!whitelist_form_array[index])
                id_whiteList_array[index] = AddToggleOption("$SHSE_UNKNOWN", whiteList_flag_array[index], OPTION_FLAG_DISABLED)
            elseif (!whiteList_name_array[index])
                id_whiteList_array[index] = AddToggleOption("$SHSE_UNKNOWN", whiteList_flag_array[index])
            else
                id_whiteList_array[index] = AddToggleOption(whiteList_name_array[index], whiteList_flag_array[index])
            endif
            index += 1
        endWhile

    elseif (currentPage == Pages[4]) ; blacklist
        AddKeyMapOptionST("blackListHotkeyCode", "$SHSE_BLACKLIST_KEY", blackListHotkeyCode)

        int size = eventScript.blacklist_form.GetSize()
        if (size == 0)
            return
        endif

        int index = 0
        while (index < size)
            if (!blacklist_form_array[index])
                id_blackList_array[index] = AddToggleOption("$SHSE_UNKNOWN", blackList_flag_array[index], OPTION_FLAG_DISABLED)
            elseif (!blackList_name_array[index])
                id_blackList_array[index] = AddToggleOption("$SHSE_UNKNOWN", blackList_flag_array[index])
            else
                id_blackList_array[index] = AddToggleOption(blackList_name_array[index], blackList_flag_array[index])
            endif
            index += 1
        endWhile
        
    elseif currentPage == Pages[5] ; collections
;   ======================== LEFT ========================
        SetCursorFillMode(TOP_TO_BOTTOM)

        PopulateCollectionGroups()
        ; per-group fields only accessible if appropriate
        int flags = OPTION_FLAG_DISABLED
        string initialGroupName = ""
        if collectionsEnabled && collectionGroupCount > 0
            flags = OPTION_FLAG_NONE
            initialGroupName = collectionGroupNames[collectionGroup]
            PopulateCollectionsForGroup(initialGroupName)
        endIf

        AddHeaderOption("$SHSE_COLLECTIONS_GLOBAL_HEADER")
        AddToggleOptionST("collectionsEnabled", "$SHSE_COLLECTIONS_ENABLED", collectionsEnabled)
        AddMenuOptionST("chooseCollectionGroup", "$SHSE_CHOOSE_COLLECTION_GROUP", initialGroupName, flags)
        AddTextOptionST("groupCollectibleAction", "$SHSE_COLLECTIBLE_ACTION", s_collectibleActions[groupCollectibleAction], flags)
        AddToggleOptionST("groupCollectionAddNotify", "$SHSE_COLLECTION_ADD_NOTIFY", groupCollectionAddNotify, flags)
        AddToggleOptionST("groupCollectDuplicates", "$SHSE_COLLECT_DUPLICATES", groupCollectDuplicates, flags)

;   ======================== RIGHT ========================
        SetCursorPosition(1)
        AddHeaderOption("$SHSE_COLLECTION_GROUP_HEADER")
        AddMenuOptionST("chooseCollectionIndex", "$SHSE_CHOOSE_COLLECTION", "", flags)
        AddTextOptionST("collectibleAction", "$SHSE_COLLECTIBLE_ACTION", s_collectibleActions[collectibleAction], flags)
        AddToggleOptionST("collectionAddNotify", "$SHSE_COLLECTION_ADD_NOTIFY", collectionAddNotify, flags)
        AddToggleOptionST("collectDuplicates", "$SHSE_COLLECT_DUPLICATES", collectDuplicates, flags)
        AddTextOptionST("itemsCollected", "", "", flags)
    endif
endEvent

Event OnOptionSelect(int a_option)
    int index = -1

    index = id_objectSettingArray.find(a_option)
    if (index >= 0)
        string keyName = GetObjectTypeNameByType(index)
        if (keyName != "unknown")
            if (index == objType_Mine)
                int size = s_behaviorToggleArray.length
                objectSettingArray[index] = CycleInt(objectSettingArray[index] as int, size)
                SetTextOptionValue(a_option, s_behaviorToggleArray[(objectSettingArray[index] as int)])
            elseif (index == objType_Ammo)
                int size = s_ammoBehaviorArray.length
                objectSettingArray[index] = CycleInt(objectSettingArray[index] as int, size)
                SetTextOptionValue(a_option, s_ammoBehaviorArray[(objectSettingArray[index] as int)])
            else
                int size = s_ammoBehaviorArray.length
                objectSettingArray[index] = CycleInt(objectSettingArray[index] as int, size)
                SetTextOptionValue(a_option, s_behaviorArray[(objectSettingArray[index] as int)])
            endif
;           PutSetting(type_Harvest, type_ItemObject, keyName, objectSettingArray[index])
        endif
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
endEvent

event OnOptionSliderOpen(int a_option)
    SetSliderDialogDefaultValue(0.0)
    SetSliderDialogInterval(1.0)

    ; lookup by object type
    int index = -1
    index = id_valueWeightArray.find(a_option)
    if (index > -1)
        SetSliderDialogStartValue(valueWeightSettingArray[index])
        if index == objtype_ammo
            SetSliderDialogRange(0, 40)
        else
            SetSliderDialogRange(0, 1000)
        endIf
    endif
endEvent

event OnOptionSliderAccept(int a_option, float a_value)
    int index = -1

    index = id_valueWeightArray.find(a_option)
    if (index > -1)
        string keyName = GetObjectTypeNameByType(index)
        if (keyName != "unknown")
            valueWeightSettingArray[index] = a_value
;           PutSetting(type_Harvest, type_ValueWeight, keyName, valueWeightSettingArray[index])
            if index == objtype_ammo
                SetSliderOptionValue(a_option, a_value, "$SHSE_DAMAGE")
            else
                SetSliderOptionValue(a_option, a_value, "$SHSE_V/W")
            endIf
        endif
        return
    endif
endEvent

event OnOptionHighlight(int a_option)
    int index = -1
    
    index = id_objectSettingArray.find(a_option)
    if (index > -1)
        SetInfoText(s_objectTypeNameArray[index])
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
        form item = whitelist_form_array[index]
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

        form item = blacklist_form_array[index]
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

state enableLootDeadbody
    event OnSelectST()
        int size = s_lootDeadBodyArray.length
        enableLootDeadbody = CycleInt(enableLootDeadbody, size)
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

state pauseHotkeyCode
    event OnKeyMapChangeST(int newKeyCode, string conflictControl, string conflictName)
        pauseHotkeyCode = newKeyCode
        SetKeyMapOptionValueST(pauseHotkeyCode)
    endEvent

    event OnDefaultST()
        pauseHotkeyCode = 34
        SetKeyMapOptionValueST(pauseHotkeyCode)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_PAUSE_KEY"))
    endEvent
endState

state whiteListHotkeyCode
    event OnKeyMapChangeST(int newKeyCode, string conflictControl, string conflictName)
        whiteListHotkeyCode = newKeyCode
        SetKeyMapOptionValueST(whiteListHotkeyCode)
    endEvent

    event OnDefaultST()
        whiteListHotkeyCode = 22
        SetKeyMapOptionValueST(whiteListHotkeyCode)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_WHITELIST_KEY"))
    endEvent
endState

state blackListHotkeyCode
    event OnKeyMapChangeST(int newKeyCode, string conflictControl, string conflictName)
        blackListHotkeyCode = newKeyCode
        SetKeyMapOptionValueST(blackListHotkeyCode)
    endEvent

    event OnDefaultST()
        blackListHotkeyCode = 37
        SetKeyMapOptionValueST(blackListHotkeyCode)
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
                ApplySettingsFromFile()
            elseif (iniSaveLoad == 2) ; save to My Custom Settings Files
                SaveIniFile()
            elseif (iniSaveLoad == 3) ; load from Default Settings file
                LoadIniFile(True)
                ApplySettingsFromFile()
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

state questObjectScope
    event OnSelectST()
        int size = s_questObjectScopeArray.length
        questObjectScope = CycleInt(questObjectScope, size)
        SetTextOptionValueST(s_questObjectScopeArray[questObjectScope])
    endEvent

    event OnDefaultST()
        questObjectScope = 1
        SetTextOptionValueST(s_questObjectScopeArray[questObjectScope])
    endEvent

    event OnHighlightST()
        string trans = GetTranslation("$SHSE_DESC_QUESTOBJECT_SCOPE")
        ;DebugTrace("Quest object state helptext " + trans)
        SetInfoText(trans)
    endEvent
endState

state questObjectLoot
    event OnSelectST()
        int size = s_specialObjectHandlingArray.length
        questObjectLoot = CycleInt(questObjectLoot, size)
        SetTextOptionValueST(s_specialObjectHandlingArray[questObjectLoot])
    endEvent

    event OnDefaultST()
        questObjectLoot = 2
        SetTextOptionValueST(s_specialObjectHandlingArray[questObjectLoot])
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_QUESTOBJECT_LOOT"))
    endEvent
endState

state enchantItemGlow
    event OnSelectST()
        enchantItemGlow = !(enchantItemGlow as bool)
        SetToggleOptionValueST(enchantItemGlow)
    endEvent

    event OnDefaultST()
        enchantItemGlow = true
        SetToggleOptionValueST(enchantItemGlow)
    endEvent

    event OnHighlightST()
        SetInfoText(GetTranslation("$SHSE_DESC_ENCHANTITEM_GLOW"))
    endEvent
endState

state valuableItemLoot
    event OnSelectST()
        int size = s_specialObjectHandlingArray.length
        valuableItemLoot = CycleInt(valuableItemLoot, size)
        SetTextOptionValueST(s_specialObjectHandlingArray[valuableItemLoot])
    endEvent

    event OnDefaultST()
        valuableItemLoot = 2
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

state lockedChestLoot
    event OnSelectST()
        int size = s_specialObjectHandlingArray.length
        lockedChestLoot = CycleInt(lockedChestLoot, size)
        SetTextOptionValueST(s_specialObjectHandlingArray[lockedChestLoot])
    endEvent

    event OnDefaultST()
        lockedChestLoot = 2
        SetTextOptionValueST(s_specialObjectHandlingArray[lockedChestLoot])
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

Function SyncCollectionPolicy()

EndFunction

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
        PopulateCollectionsForGroup(collectionGroupNames[index])
    endEvent

    event OnDefaultST()
        collectionGroup = 0
        SetMenuOptionValueST(collectionGroupNames[collectionGroup])
        PopulateCollectionsForGroup(collectionGroupNames[collectionGroup])
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
        int size = s_specialObjectHandlingArray.length
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
