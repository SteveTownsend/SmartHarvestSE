Scriptname SHSE_MCM extends SKI_ConfigBase  

Import SHSE_PluginProxy

SHSE_EventsAlias Property eventScript Auto
GlobalVariable Property g_LootingEnabled Auto

bool initComplete = false

int defaultRadius = 30
float defaultInterval = 1.0
int defaultRadiusIndoors = 15
float defaultIntervalIndoors = 0.5

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
int objType_WhiteList

bool enableHarvest
bool enableLootContainer
bool enableLootDeadbody

bool unencumberedInCombat
bool unencumberedInPlayerHome
bool unencumberedIfWeaponDrawn

bool pushLocationToExcludeList
bool pushCellToExcludeList

int pauseHotkeyCode
int whiteListHotkeyCode
int blackListHotkeyCode
int preventPopulationCenterLooting
string[] s_populationCenterArray

float radius
float interval
float radiusIndoors
float intervalIndoors

int iniSaveLoad
string[] s_iniSaveLoadArray
int whiteListSaveLoad
string[] s_whiteListSaveLoadArray
int blackListSaveLoad
string[] s_blackListSaveLoadArray
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
int valueWeightDefaultDefault = 10
int maxMiningItems
int maxMiningItemsDefault = 15
int[] id_valueWeightArray
float[] valueWeightSettingArray

String[] s_behaviorArray
String[] s_objectTypeNameArray

int[] id_whiteList_array
Form[] whitelist_form_array
String[] whiteList_name_array
bool[] whiteList_flag_array

int[] id_blackList_array
Form[] blacklist_form_array
String[] blackList_name_array
bool[] blackList_flag_array

bool gameReloadLock = false

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
	float[] result = New float[33]
	while (index < 33)
		result[index] = GetSettingToObjectArrayEntry(section1, section2, index)
		;DebugTrace("Config setting " + section1 + "/" + section2 + "/" + index + " = " + result[index])
		index += 1
	endWhile
	return result
endFunction

function SeedDefaults()
	enableHarvest = GetSetting(type_Common, type_Config, "enableHarvest") as bool
	enableLootContainer = GetSetting(type_Common, type_Config, "enableLootContainer") as bool
	enableLootDeadbody = GetSetting(type_Common, type_Config, "enableLootDeadbody") as bool
	unencumberedInCombat = GetSetting(type_Common, type_Config, "unencumberedInCombat") as bool
	unencumberedInPlayerHome = GetSetting(type_Common, type_Config, "unencumberedInPlayerHome") as bool
	unencumberedIfWeaponDrawn = GetSetting(type_Common, type_Config, "unencumberedIfWeaponDrawn") as bool
	;DebugTrace("SeedDefaults - unencumberedIfWeaponDrawn " + unencumberedIfWeaponDrawn)
	pauseHotkeyCode = GetSetting(type_Common, type_Config, "pauseHotkeyCode") as int
	whiteListHotkeyCode = GetSetting(type_Common, type_Config, "whiteListHotkeyCode") as int
	blackListHotkeyCode = GetSetting(type_Common, type_Config, "blackListHotkeyCode") as int
	preventPopulationCenterLooting = GetSetting(type_Common, type_Config, "preventPopulationCenterLooting") as int

	radius = GetSetting(type_Harvest, type_Config, "RadiusFeet") as float
	interval = GetSetting(type_Harvest, type_Config, "IntervalSeconds") as float
	radiusIndoors = GetSetting(type_Harvest, type_Config, "IndoorsRadiusFeet") as float
	intervalIndoors = GetSetting(type_Harvest, type_Config, "IndoorsIntervalSeconds") as float

	questObjectScope = GetSetting(type_Harvest, type_Config, "questObjectScope") as int
	crimeCheckNotSneaking = GetSetting(type_Harvest, type_Config, "crimeCheckNotSneaking") as int
	crimeCheckSneaking = GetSetting(type_Harvest, type_Config, "crimeCheckSneaking") as int

	playerBelongingsLoot = GetSetting(type_Harvest, type_Config, "playerBelongingsLoot") as int
	questObjectLoot = GetSetting(type_Harvest, type_Config, "questObjectLoot") as int
	enchantItemGlow = GetSetting(type_Harvest, type_Config, "enchantItemGlow") as bool
	lockedChestLoot = GetSetting(type_Harvest, type_Config, "lockedChestLoot") as int
	bossChestLoot = GetSetting(type_Harvest, type_Config, "bossChestLoot") as int
	manualLootTargetNotify = GetSetting(type_Harvest, type_Config, "manualLootTargetNotify") as bool

	disableDuringCombat = GetSetting(type_Harvest, type_Config, "disableDuringCombat") as bool
	disableWhileWeaponIsDrawn = GetSetting(type_Harvest, type_Config, "disableWhileWeaponIsDrawn") as bool
	disableWhileConcealed = GetSetting(type_Harvest, type_Config, "disableWhileConcealed") as bool

	playContainerAnimation = GetSetting(type_Harvest, type_Config, "PlayContainerAnimation") as int
	;DebugTrace("SeedDefaults - playContainerAnimation " + playContainerAnimation)

	valueWeightDefault = GetSetting(type_Harvest, type_Config, "valueWeightDefault") as int
	;DebugTrace("SeedDefaults - vw default " + valueWeightDefault)
	updateMaxMiningItems(GetSetting(type_Harvest, type_Config, "maxMiningItems") as int)

	objectSettingArray = GetSettingToObjectArray(type_Harvest, type_ItemObject)
	valueWeightSettingArray = GetSettingToObjectArray(type_Harvest, type_ValueWeight)
endFunction

function init()
 	;DebugTrace("init start")
    ; Seed defaults from the INI file

    LoadIniFile()
    SeedDefaults()

	initComplete = true
	;DebugTrace("init finished")
endFunction

Function ApplySetting()

;	DebugTrace("  MCM ApplySetting start")

	PutSetting(type_Common, type_Config, "enableHarvest", enableHarvest as float)
	PutSetting(type_Common, type_Config, "enableLootContainer", enableLootContainer as float)
	PutSetting(type_Common, type_Config, "enableLootDeadbody", enableLootDeadbody as float)
	PutSetting(type_Common, type_Config, "unencumberedInCombat", unencumberedInCombat as float)
	PutSetting(type_Common, type_Config, "unencumberedInPlayerHome", unencumberedInPlayerHome as float)
	PutSetting(type_Common, type_Config, "unencumberedIfWeaponDrawn", unencumberedIfWeaponDrawn as float)

	PutSetting(type_Common, type_Config, "PauseHotkeyCode", pauseHotkeyCode as float)
	PutSetting(type_Common, type_Config, "whiteListHotkeyCode", whiteListHotkeyCode as float)
	PutSetting(type_Common, type_Config, "blackListHotkeyCode", blackListHotkeyCode as float)
	PutSetting(type_Common, type_Config, "preventPopulationCenterLooting", preventPopulationCenterLooting as float)

	PutSetting(type_Harvest, type_Config, "RadiusFeet", radius)
	PutSetting(type_Harvest, type_Config, "IntervalSeconds", interval)
	PutSetting(type_Harvest, type_Config, "IndoorsRadiusFeet", radiusIndoors)
	PutSetting(type_Harvest, type_Config, "IndoorsIntervalSeconds", intervalIndoors)

	PutSetting(type_Harvest, type_Config, "questObjectScope", questObjectScope as float)
	PutSetting(type_Harvest, type_Config, "crimeCheckNotSneaking", crimeCheckNotSneaking as float)
	PutSetting(type_Harvest, type_Config, "crimeCheckSneaking", crimeCheckSneaking as float)
	PutSetting(type_Harvest, type_Config, "playerBelongingsLoot", playerBelongingsLoot as float)
	PutSetting(type_Harvest, type_Config, "PlayContainerAnimation", playContainerAnimation as float)

	PutSetting(type_Harvest, type_Config, "questObjectLoot", questObjectLoot as float)
	PutSetting(type_Harvest, type_Config, "enchantItemGlow", enchantItemGlow as float)

	PutSetting(type_Harvest, type_Config, "lockedChestLoot", lockedChestLoot as float)
	PutSetting(type_Harvest, type_Config, "bossChestLoot", bossChestLoot as float)
	PutSetting(type_Harvest, type_Config, "manualLootTargetNotify", manualLootTargetNotify as float)

	PutSetting(type_Harvest, type_Config, "disableDuringCombat", disableDuringCombat as float)
	PutSetting(type_Harvest, type_Config, "disableWhileWeaponIsDrawn", disableWhileWeaponIsDrawn as float)
	PutSetting(type_Harvest, type_Config, "disableWhileConcealed", disableWhileConcealed as float)

	PutSettingObjectArray(type_Harvest, type_ItemObject, objectSettingArray)

	PutSetting(type_Harvest, type_Config, "valueWeightDefault", valueWeightDefault as float)
	PutSetting(type_Harvest, type_Config, "maxMiningItems", maxMiningItems as float)
	PutSettingObjectArray(type_Harvest, type_ValueWeight, valueWeightSettingArray)

    ; seed looting scan enabled according to configured settings
    bool isEnabled = enableHarvest || enableLootContainer || enableLootDeadbody || unencumberedInCombat || unencumberedInPlayerHome|| unencumberedIfWeaponDrawn 
    ;DebugTrace("result " + isEnabled + "from flags:" + enableHarvest + " " + enableLootContainer + " " + enableLootDeadbody + " " + unencumberedInCombat + " " + unencumberedInPlayerHome + " " + unencumberedIfWeaponDrawn)
    if (isEnabled)
       	g_LootingEnabled.SetValue(1)
	else
       	g_LootingEnabled.SetValue(0)
	endif

	; correct for any weight adjustments saved into this file, plugin will reinstate if/as needed
	; Do this before plugin becomes aware of player home list
	player = Game.GetPlayer()
	eventScript.SetPlayer(player)
    eventScript.RemoveCarryWeightDelta()
	eventScript.ApplySetting()

    ; do this last so plugin state is in sync	
	if (isEnabled)
		Debug.Notification(Replace(GetTranslation("$SHSE_ENABLE"), "{VERSION}", GetPluginVersion()))
		AllowSearch()
	Else
		Debug.Notification(Replace(GetTranslation("$SHSE_DISABLE"), "{VERSION}", GetPluginVersion()))
		DisallowSearch()
	EndIf

;	DebugTrace("  MCM ApplySetting finished")
endFunction

Event OnConfigInit()

;	DebugTrace("** OnConfigInit start **")

	ModName = "$SHSE_MOD_NAME"

	Pages = New String[6]
	Pages[0] = "$SHSE_RULES_DEFAULTS_PAGENAME"
	Pages[1] = "$SHSE_SPECIALS_LISTS_PAGENAME"
	Pages[2] = "$SHSE_SHARED_SETTINGS_PAGENAME"
	Pages[3] = "$SHSE_WHITELIST_PAGENAME"
	Pages[4] = "$SHSE_BLACKLIST_PAGENAME"

	s_iniSaveLoadArray = New String[3]
	s_iniSaveLoadArray[0] = "$SHSE_PRESET_DO_NOTHING"
	s_iniSaveLoadArray[1] = "$SHSE_PRESET_RESTORE"
	s_iniSaveLoadArray[2] = "$SHSE_PRESET_STORE"

	s_whiteListSaveLoadArray = New String[3]
	s_whiteListSaveLoadArray[0] = "$SHSE_PRESET_DO_NOTHING"
	s_whiteListSaveLoadArray[1] = "$SHSE_WHITELIST_RESTORE"
	s_whiteListSaveLoadArray[2] = "$SHSE_WHITELIST_STORE"

	s_blackListSaveLoadArray = New String[3]
	s_blackListSaveLoadArray[0] = "$SHSE_PRESET_DO_NOTHING"
	s_blackListSaveLoadArray[1] = "$SHSE_BLACKLIST_RESTORE"
	s_blackListSaveLoadArray[2] = "$SHSE_BLACKLIST_STORE"

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

	s_behaviorToggleArray = New String[2]
	s_behaviorToggleArray[0] = "$SHSE_DONT_PICK_UP"
	s_behaviorToggleArray[1] = "$SHSE_PICK_UP"

	s_behaviorArray = New String[5]
	s_behaviorArray[0] = "$SHSE_DONT_PICK_UP"
	s_behaviorArray[1] = "$SHSE_PICK_UP_W/O_MSG"
	s_behaviorArray[2] = "$SHSE_PICK_UP_W/MSG"
	s_behaviorArray[3] = "$SHSE_PICK_UP_V/W_W/O_MSG"
	s_behaviorArray[4] = "$SHSE_PICK_UP_V/W_W/MSG"

    id_objectSettingArray = New Int[33]
	objectSettingArray = New float[33]

	id_valueWeightArray = New Int[33]
	valueWeightSettingArray = New float[33]

	s_objectTypeNameArray = New String[33]

	eventScript.whitelist_form = Game.GetFormFromFile(0x0333C, "SmartHarvestSE.esp") as Formlist
	eventScript.blacklist_form = Game.GetFormFromFile(0x0333D, "SmartHarvestSE.esp") as Formlist
	
	pushLocationToExcludeList = false
	pushCellToExcludeList= false

;	DebugTrace("** OnConfigInit finished **")
endEvent

int function GetVersion()
	; update script variables needing sync to native
	objType_Flora = GetObjectTypeByName("flora")
	objType_Critter = GetObjectTypeByName("critter")
	objType_Septim = GetObjectTypeByName("septims")
	objType_LockPick = GetObjectTypeByName("lockpick")
	objType_Soulgem = GetObjectTypeByName("soulgem")
	objType_Key = GetObjectTypeByName("key")
	objType_Ammo = GetObjectTypeByName("ammo")
	objType_Mine = GetObjectTypeByName("orevein")
	objType_WhiteList = GetObjectTypeByName("whitelist")
	eventScript.SyncNativeDataTypes()

    ; New or clarified defaults and constants
	manualLootTargetNotify = true
	defaultRadius = 30
	defaultInterval = 1.0
	defaultRadiusIndoors = 15
	defaultIntervalIndoors = 0.5
	valueWeightDefaultDefault = 10
	maxMiningItemsDefault = 15
	playContainerAnimation = 2
	eventScript.UpdateMaxMiningItems(maxMiningItems)

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
	s_objectTypeNameArray[32] = "$SHSE_WHITELIST"

	return 22
endFunction

Event OnVersionUpdate(int a_version)
;	DebugTrace("OnVersionUpdate start" + a_version)

	if (a_version >= 22 && CurrentVersion < 22)
		; Major revision to reduce script dependence and auto-categorize lootables
		Debug.Trace(self + ": Updating script to version " + a_version)
		OnConfigInit()
	endIf

;	DebugTrace("OnVersionUpdate finished" + a_version)
endEvent

Event OnGameReload()
	parent.OnGameReload()
    ;DebugTrace("OnGameReload - vw default " + valueWeightDefault)

	if (gameReloadLock)
		return
	endif
	gameReloadLock = true

    ;DebugTrace("OnGameReload - init-complete " + initComplete)
	if (!initComplete)
        ;DebugTrace("OnGameReload - init required")
		init()
	endif

	ApplySetting()
	
;	DebugTrace("* OnGameReload finished *")
	gameReloadLock = false
endEvent

Event OnConfigOpen()
;	DebugTrace("OnConfigOpen")
	Pages = New String[5]
	Pages[0] = "$SHSE_RULES_DEFAULTS_PAGENAME"
	Pages[1] = "$SHSE_SPECIALS_LISTS_PAGENAME"
	Pages[2] = "$SHSE_SHARED_SETTINGS_PAGENAME"
	Pages[3] = "$SHSE_WHITELIST_PAGENAME"
	Pages[4] = "$SHSE_BLACKLIST_PAGENAME"
	
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

;			DebugTrace("   " + index)
		
			blacklist_form_array[index] = eventScript.blacklist_form.GetAt(index)
			blackList_name_array[index] = GetNameForListForm(blacklist_form_array[index])
			blackList_flag_array[index] = true
			
;			DebugTrace(blackList_name_array[index])

			
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

Event OnConfigClose()
;	DebugTrace("OnConfigClose")

	TidyListUp(eventScript.whitelist_form, whitelist_form_array, whiteList_flag_array, "$SHSE_WHITELIST_REMOVED")
	TidyListUp(eventScript.blacklist_form, blacklist_form_array, blackList_flag_array, "$SHSE_BLACKLIST_REMOVED")

	iniSaveLoad = 0
	eventScript.SyncWhiteList()
	whiteListSaveLoad = 0
	
	
	;--- BlackList File operation --------------------
	if (blackListSaveLoad == 1) ; load/restore
		bool result = LoadBlackList()
		if (result)
			Debug.Notification("$SHSE_BLACKLIST_RESTORE_MSG")
		endif
	elseif (blackListSaveLoad == 2) ; save/store
		bool result = SaveBlackList()
		if (result)
			Debug.Notification("$SHSE_BLACKLIST_STORE_MSG")
		endif
	endif
	eventScript.SyncBlackList()
	blackListSaveLoad = 0
	
	if (pushLocationToExcludeList)
		form locForm = player.GetCurrentLocation() as form
		if (locForm)
			eventScript.ManageBlackList(locForm)
		endif
		pushLocationToExcludeList = false
	endif

	if (pushCellToExcludeList)
		form cellForm = player.GetParentCell() as form
		if (cellForm)
			eventScript.ManageBlackList(cellForm)
		endif
		pushCellToExcludeList = false
	endif

	ApplySetting()
endEvent

event OnPageReset(string currentPage)
;	DebugTrace("OnPageReset")

	If (currentPage == "")
		LoadCustomContent("towawot/SmartHarvestSE.dds")
		Return
	Else
		UnloadCustomContent()
	EndIf

	if (currentPage == Pages[0])

; 	======================== LEFT ========================
		SetCursorFillMode(TOP_TO_BOTTOM)

		AddHeaderOption("$SHSE_LOOTING_RULES_HEADER")
		AddToggleOptionST("enableHarvest", "$SHSE_ENABLE_HARVEST", enableHarvest)
		AddToggleOptionST("enableLootContainer", "$SHSE_ENABLE_LOOT_CONTAINER", enableLootContainer)
		AddToggleOptionST("enableLootDeadbody", "$SHSE_ENABLE_LOOT_DEADBODY", enableLootDeadbody)
		AddToggleOptionST("disableDuringCombat", "$SHSE_DISABLE_DURING_COMBAT", disableDuringCombat)
		AddToggleOptionST("disableWhileWeaponIsDrawn", "$SHSE_DISABLE_IF_WEAPON_DRAWN", disableWhileWeaponIsDrawn)
		AddToggleOptionST("disableWhileConcealed", "$SHSE_DISABLE_IF_CONCEALED", disableWhileConcealed)
		AddTextOptionST("preventPopulationCenterLooting", "$SHSE_POPULATION_CENTER_PREVENTION", s_populationCenterArray[preventPopulationCenterLooting])
		AddTextOptionST("crimeCheckNotSneaking", "$SHSE_CRIME_CHECK_NOT_SNEAKING", s_crimeCheckNotSneakingArray[crimeCheckNotSneaking])
		AddTextOptionST("crimeCheckSneaking", "$SHSE_CRIME_CHECK_SNEAKING", s_crimeCheckSneakingArray[crimeCheckSneaking])

; 	======================== RIGHT ========================
		SetCursorPosition(1)

		AddHeaderOption("$SHSE_ITEM_HARVEST_DEFAULT_HEADER")
		AddMenuOptionST("iniSaveLoad", "$SHSE_SETTINGS_FILE_OPERATION", s_iniSaveLoadArray[iniSaveLoad])
		AddKeyMapOptionST("pauseHotkeyCode", "$SHSE_PAUSE_KEY", pauseHotkeyCode)
	    AddSliderOptionST("ValueWeightDefault", "$SHSE_VW_DEFAULT", valueWeightDefault)
		AddSliderOptionST("Radius", "$SHSE_RADIUS", radius, "$SHSE{0}UNIT")
		AddSliderOptionST("Interval", "$SHSE_INTERVAL", interval, "$SHSE{1}SEC")
		AddSliderOptionST("RadiusIndoors", "$SHSE_RADIUS_INDOORS", radiusIndoors, "$SHSE{0}UNIT")
		AddSliderOptionST("IntervalIndoors", "$SHSE_INTERVAL_INDOORS", intervalIndoors, "$SHSE{1}SEC")
	    AddSliderOptionST("MaxMiningItems", "$SHSE_MAX_MINING_ITEMS", maxMiningItems)
		AddToggleOptionST("unencumberedInCombat", "$SHSE_UNENCUMBERED_COMBAT", unencumberedInCombat)
		AddToggleOptionST("unencumberedInPlayerHome", "$SHSE_UNENCUMBERED_PLAYER_HOME", unencumberedInPlayerHome)
		AddToggleOptionST("unencumberedIfWeaponDrawn", "$SHSE_UNENCUMBERED_IF_WEAPON_DRAWN", unencumberedIfWeaponDrawn)
		
	elseif (currentPage == Pages[1])

; 	======================== LEFT ========================
		SetCursorFillMode(TOP_TO_BOTTOM)

		AddHeaderOption("$SHSE_SPECIAL_OBJECT_BEHAVIOR_HEADER")
		AddTextOptionST("questObjectLoot", "$SHSE_QUESTOBJECT_LOOT", s_specialObjectHandlingArray[questObjectLoot])
		AddTextOptionST("questObjectScope", "$SHSE_QUESTOBJECT_SCOPE", s_questObjectScopeArray[questObjectScope])
		AddTextOptionST("lockedChestLoot", "$SHSE_LOCKEDCHEST_LOOT", s_specialObjectHandlingArray[lockedChestLoot])
		AddTextOptionST("bossChestLoot", "$SHSE_BOSSCHEST_LOOT", s_specialObjectHandlingArray[bossChestLoot])
		AddTextOptionST("playerBelongingsLoot", "$SHSE_PLAYER_BELONGINGS_LOOT", s_specialObjectHandlingArray[playerBelongingsLoot])
		AddToggleOptionST("enchantItemGlow", "$SHSE_ENCHANTITEM_GLOW", enchantItemGlow)
		AddToggleOptionST("manualLootTargetNotify", "$SHSE_MANUAL_LOOT_TARGET_NOTIFY", manualLootTargetNotify)
		AddTextOptionST("playContainerAnimation", "$SHSE_PLAY_CONTAINER_ANIMATION", s_playContainerAnimationArray[playContainerAnimation])

; 	======================== RIGHT ========================
		SetCursorPosition(1)

		AddHeaderOption("$SHSE_LIST_MANAGEMENT_HEADER")

		AddKeyMapOptionST("whiteListHotkeyCode", "$SHSE_WHITELIST_KEY", whiteListHotkeyCode)
		; TODO do we need file support here?
		;AddMenuOptionST("whiteListSaveLoad", "$SHSE_WHITELIST_FILE_OPERATION", s_whiteListSaveLoadArray[whiteListSaveLoad])
		Form locForm = player.GetCurrentLocation() as Form
		int flagLocation = OPTION_FLAG_NONE
		if (!locForm)
			flagLocation = OPTION_FLAG_DISABLED
		endif
		AddToggleOptionST("pushLocationToExcludeList", "$SHSE_LOCATION_TO_BLACKLIST", pushLocationToExcludeList, flagLocation)

		Form cellForm = player.GetParentCell() as Form
		int flagCell = OPTION_FLAG_NONE
		if (!cellForm)
			flagCell = OPTION_FLAG_DISABLED
		endif
		
		AddToggleOptionST("pushCellToExcludeList", "$SHSE_CELL_TO_BLACKLIST", pushCellToExcludeList, flagCell)
		AddKeyMapOptionST("blackListHotkeyCode", "$SHSE_BLACKLIST_KEY", blackListHotkeyCode)
		; TODO do we need file support here?
		;AddMenuOptionST("blackListSaveLoad", "$SHSE_BLACKLIST_FILE_OPERATION", s_blackListSaveLoadArray[blackListSaveLoad])
		
	elseif (currentPage == Pages[2]) ; object harvester
		
; 	======================== LEFT ========================
		SetCursorFillMode(TOP_TO_BOTTOM)

		AddHeaderOption("$SHSE_PICK_UP_ITEM_TYPE_HEADER")
		
		int index = 1
		while (index <= (s_objectTypeNameArray.length - 2))	; whitelist is the last
			if (index == objType_Mine)
				id_objectSettingArray[index] = AddTextOption(s_objectTypeNameArray[index], s_behaviorToggleArray[(objectSettingArray[index] as int)])
			else
				id_objectSettingArray[index] = AddTextOption(s_objectTypeNameArray[index], s_behaviorArray[(objectSettingArray[index] as int)])
			endif
			index += 1
		endWhile

; 	======================== RIGHT ========================
		SetCursorPosition(1)

		AddHeaderOption("$SHSE_VALUE/WEIGHT_HEADER")

		index = 1
		while (index <= (s_objectTypeNameArray.length - 2))	; whitelist is the last
			; do not request V/W for weightless or unhandleable item types
			if (index == objType_Mine || index == objType_Ammo || index == objType_Septim  || index == objType_Key || index == objType_LockPick || index == objType_WhiteList)
				AddEmptyOption()
			else
				id_valueWeightArray[index] = AddSliderOption(s_objectTypeNameArray[index], valueWeightSettingArray[index], "$SHSE_V/W{1}")
			endif
			index += 1
		endWhile
		
	elseif (currentPage == Pages[3]) ; whiteList

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
	endif
endEvent

Event OnOptionSelect(int a_option)
	int index = -1

	index = id_objectSettingArray.find(a_option)
	if (index >= 0)
		string keyName = GetObjectTypeNameByType(index)
		if (keyName != "unknown")
			if (index != objType_Mine)
				int size = s_behaviorArray.length
				objectSettingArray[index] = CycleInt(objectSettingArray[index] as int, size)
				SetTextOptionValue(a_option, s_behaviorArray[(objectSettingArray[index] as int)])
			else
				int size = s_behaviorToggleArray.length
				objectSettingArray[index] = CycleInt(objectSettingArray[index] as int, size)
				SetTextOptionValue(a_option, s_behaviorToggleArray[(objectSettingArray[index] as int)])
			endif
;			PutSetting(type_Harvest, type_ItemObject, keyName, objectSettingArray[index])
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
    	if (index == objType_Ammo)
			SetSliderDialogRange(0, 30)
		else
			SetSliderDialogRange(0, 1000)
		endif
		return
	endif

endEvent

event OnOptionSliderAccept(int a_option, float a_value)
	int index = -1

	index = id_valueWeightArray.find(a_option)
	if (index > -1)
		string keyName = GetObjectTypeNameByType(index)
		if (keyName != "unknown")
			valueWeightSettingArray[index] = a_value
;			PutSetting(type_Harvest, type_ValueWeight, keyName, valueWeightSettingArray[index])
			SetSliderOptionValue(a_option, a_value, "$SHSE_V/W{1}")
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

state pushLocationToExcludeList
	event OnSelectST()
		pushLocationToExcludeList = !(pushLocationToExcludeList as bool)
		SetToggleOptionValueST(pushLocationToExcludeList)
	endEvent

	event OnDefaultST()
		pushLocationToExcludeList = false
		SetToggleOptionValueST(pushLocationToExcludeList)
	endEvent

	event OnHighlightST()
		SetInfoText(GetTranslation("$SHSE_DESC_LOCATION_TO_BLACKLIST"))
	endEvent
endState

state pushCellToExcludeList
	event OnSelectST()
		pushCellToExcludeList = !(pushCellToExcludeList as bool)
		SetToggleOptionValueST(pushCellToExcludeList)
	endEvent

	event OnDefaultST()
		pushCellToExcludeList = false
		SetToggleOptionValueST(pushCellToExcludeList)
	endEvent

	event OnHighlightST()
		SetInfoText(GetTranslation("$SHSE_DESC_CELL_TO_BLACKLIST"))
	endEvent
endState

;Int function GetStateOptionIndex(String a_stateName)
;	return parent.GetStateOptionIndex(a_stateName)
;endFunction

state enableHarvest
;	Event OnBeginState()
;		int result = GetStateOptionIndex(self.GetState())
;		DebugTrace("Entered the running state! " + result)
;	EndEvent

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

;	event OnEndState()
;		DebugTrace("Leaving the running state!")
;	endEvent
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
		enableLootDeadbody = !(enableLootDeadbody as bool)
		SetToggleOptionValueST(enableLootDeadbody)
	endEvent

	event OnDefaultST()
		enableLootDeadbody = true
		SetToggleOptionValueST(enableLootDeadbody)
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
		SetSliderDialogStartValue(radius)
		SetSliderDialogDefaultValue(defaultRadius)
		SetSliderDialogRange(1, 100)
		SetSliderDialogInterval(1)
	endEvent

	event OnSliderAcceptST(float value)
		radius = value
		SetSliderOptionValueST(radius, "$SHSE{0}UNIT")
	endEvent

	event OnDefaultST()
		radius = defaultRadius
		SetSliderOptionValueST(radius, "$SHSE{0}UNIT")
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
		SetSliderOptionValueST(interval, "$SHSE{1}SEC")
	endEvent

	event OnDefaultST()
		interval = defaultInterval
		SetSliderOptionValueST(interval, "$SHSE{1}SEC")
	endEvent

	event OnHighlightST()
		SetInfoText(GetTranslation("$SHSE_DESC_INTERVAL"))
	endEvent
endState

state RadiusIndoors
	event OnSliderOpenST()
		SetSliderDialogStartValue(radiusIndoors)
		SetSliderDialogDefaultValue(defaultRadiusIndoors)
		SetSliderDialogRange(1, 100)
		SetSliderDialogInterval(1)
	endEvent

	event OnSliderAcceptST(float value)
		radiusIndoors = value
		SetSliderOptionValueST(radiusIndoors, "$SHSE{0}UNIT")
	endEvent

	event OnDefaultST()
		radiusIndoors = defaultRadiusIndoors
		SetSliderOptionValueST(radius, "$SHSE{0}UNIT")
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
		SetSliderOptionValueST(interval, "$SHSE{1}SEC")
	endEvent

	event OnDefaultST()
		intervalIndoors = defaultIntervalIndoors
		SetSliderOptionValueST(intervalIndoors, "$SHSE{1}SEC")
	endEvent

	event OnHighlightST()
		SetInfoText(GetTranslation("$SHSE_DESC_INTERVAL_INDOORS"))
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
	    ;DebugTrace("OnSliderAcceptST - vw default " + valueWeightDefault)
		SetSliderOptionValueST(valueWeightDefault)
	endEvent

	event OnDefaultST()
		valueWeightDefault = valueWeightDefaultDefault
	    ;DebugTrace("OnDefaultST - vw default " + valueWeightDefault)
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
    	;DebugTrace("OnSliderAcceptST: MaxMiningItems set to " + maxMiningItems)
		SetSliderOptionValueST(maxMiningItems)
	endEvent

	event OnDefaultST()
		updateMaxMiningItems(maxMiningItemsDefault)
    	;DebugTrace("OnDefaultST: MaxMiningItems set to " + maxMiningItems)
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
		else
			return
		endif
		bool continue = ShowMessage(list_warning, true, "$SHSE_OK", "$SHSE_CANCEL")
	    ;DebugTrace("response to msg= " + continue)
		if (continue)
		    SetMenuOptionValueST(s_iniSaveLoadArray[iniSaveLoad])
			iniSaveLoad = index
			if (iniSaveLoad == 1) ; load/restore
			    ;DebugTrace("loading from file")
				LoadIniFile()
				SeedDefaults()
			elseif (iniSaveLoad == 2) ; save/store
				SaveIniFile()
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

state whiteListSaveLoad
	event OnMenuOpenST()
		SetMenuDialogStartIndex(whiteListSaveLoad)
		SetMenuDialogDefaultIndex(0)
		SetMenuDialogOptions(s_whiteListSaveLoadArray)
	endEvent

	event OnMenuAcceptST(int index)
		string list_warning
		if (index == 1)
			list_warning = "$SHSE_LOAD_WARNING_MSG"
		elseif (index == 2)
			list_warning = "$SHSE_SAVE_WARNING_MSG"
		else
			return
		endif
		bool continue = ShowMessage(list_warning, true, "$SHSE_OK", "$SHSE_CANCEL")
		if (continue)
			whiteListSaveLoad = index
			SetMenuOptionValueST(s_whiteListSaveLoadArray[whiteListSaveLoad])

			;--- WhiteList File operation --------------------
			if (whiteListSaveLoad == 1) ; load/restore
				bool result = LoadWhiteList()
				if (result)
					Debug.Notification("$SHSE_WHITELIST_RESTORE_MSG")
				endif
			elseif (whiteListSaveLoad == 2) ; save/store
				bool result = SaveWhiteList()
				if (result)
					Debug.Notification("$SHSE_WHITELIST_STORE_MSG")
				endif
			endif
		endif
	endEvent

	event OnDefaultST()
		whiteListSaveLoad = 0
		SetMenuOptionValueST(s_whiteListSaveLoadArray[whiteListSaveLoad])
	endEvent

	event OnHighlightST()
		SetInfoText(GetTranslation("$SHSE_DESC_WHITELIST_FILE_OPERATION"))
	endEvent
endState

state blackListSaveLoad
	event OnMenuOpenST()
		SetMenuDialogStartIndex(blackListSaveLoad)
		SetMenuDialogDefaultIndex(0)
		SetMenuDialogOptions(s_blackListSaveLoadArray)
	endEvent

	event OnMenuAcceptST(int index)
		if (index == 0)
			return
		endif
		
		string list_warning
		if (index == 1)
			list_warning = "$SHSE_LOAD_WARNING_MSG"
		elseif (index == 2)
			list_warning = "$SHSE_SAVE_WARNING_MSG"
		endif
		bool continue = ShowMessage(list_warning, true, "$SHSE_OK", "$SHSE_CANCEL")
		if (continue)
			blackListSaveLoad = index
			SetMenuOptionValueST(s_blackListSaveLoadArray[blackListSaveLoad])
		endif
	endEvent

	event OnDefaultST()
		blackListSaveLoad = 0
		SetMenuOptionValueST(s_blackListSaveLoadArray[blackListSaveLoad])
	endEvent

	event OnHighlightST()
		SetInfoText(GetTranslation("$SHSE_DESC_BLACKLIST_FILE_OPERATION"))
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
