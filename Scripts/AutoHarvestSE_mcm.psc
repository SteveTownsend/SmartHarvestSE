Scriptname AutoHarvestSE_mcm extends SKI_ConfigBase  

Import AutoHarvestSE

AutoHarvestSE_Events_Alias Property eventScript Auto
GlobalVariable Property g_EnableLaunched Auto

int defaultRadius = 15
float defaultInterval = 0.3

int type_Common = 1
int type_AutoHarvest = 2
int type_Spell = 3
int type_Config = 1
int type_ItemObject = 2
int type_Container = 3
int type_Deadbody = 4
int type_ValueWeight = 5
int type_MaxItemCount = 6
int objType_Ammo = 23
int objType_Mine = 34

int location_type_user = 1
int location_type_excluded = 2

Formlist userlist_form
Formlist excludelist_form
Formlist blockLocationForm

bool enableAutoHarvest
bool enableLootContainer
bool enableLootDeadbody

bool unencumberedInCombat
bool unencumberedInPlayerHouse

bool pushLocationToExcludelist
bool pushCellToExcludelist

int pauseHotkeyCode
int userlistHotkeyCode
int excludelistHotkeyCode

bool consolidateSettings

float radius
float interval

int iniSaveLoad
string[] s_iniSaveLoadArray
int userlistSaveLoad
string[] s_userlistSaveLoadArray
int excludelistSaveLoad
string[] s_excludelistSaveLoadArray
int questObjectDefinition
string[] s_questObjectDefinitionArray
int crimeCheckNotSneaking
string[] s_crimeCheckNotSneakingArray
int crimeCheckSneaking
string[] s_crimeCheckSneakingArray
int playerBelongingsLoot
string[] s_behaviorToggleArray
int containerAnimation
string[] s_containerAnimationArray

bool questObjectLoot
bool lockedChestLoot
bool bossChestLoot

bool questObjectGlow
bool lockedChestGlow
bool bossChestGlow
bool enchantItemGlow

bool disableInCombat
bool disableDrawingWeapon

int[] id_objectSettingArray
float[] objectSettingArray

int[] id_containerArray
float[] containerSettingArray

int[] id_deadbodyArray
float[] deadbodySettingArray

int valueWeightDefault
int valueWeightDefaultDefault = 100
int maxMiningItems
int maxMiningItemsDefault = 10
int[] id_valueWeightArray
float[] valueWeightSettingArray

String[] s_behaviorArray
String[] s_objectTypeNameArray

int[] id_userlist_array
Form[] userlist_form_array
String[] userlist_name_array
bool[] userlist_flag_array

int[] id_excludelist_array
Form[] excludelist_form_array
String[] excludelist_name_array
bool[] excludelist_flag_array

bool gameReloadLock = false

; message flag
bool objpage_read_once = true


int Function CycleInt(int num, int max)
	int result = num + 1
	if (result >= max)
		result = 0
	endif
	return result
endFunction

function Load_mcm()
	enableAutoHarvest = GetSetting(type_Common, type_Config, "enableAutoHarvest") as bool
	enableLootContainer = GetSetting(type_Common, type_Config, "enableLootContainer") as bool
	enableLootDeadbody = GetSetting(type_Common, type_Config, "enableLootDeadbody") as bool
	unencumberedInCombat = GetSetting(type_Common, type_Config, "unencumberedInCombat") as bool
	unencumberedInPlayerHouse = GetSetting(type_Common, type_Config, "unencumberedInPlayerHouse") as bool
	pauseHotkeyCode = GetSetting(type_Common, type_Config, "pauseHotkeyCode") as int
	userlistHotkeyCode = GetSetting(type_Common, type_Config, "userlistHotkeyCode") as int
	excludelistHotkeyCode = GetSetting(type_Common, type_Config, "excludelistHotkeyCode") as int
	consolidateSettings = GetSetting(type_Common, type_Config, "consolidateSettings") as bool

	radius = GetSetting(type_AutoHarvest, type_Config, "RadiusFeet") as float
	interval = GetSetting(type_AutoHarvest, type_Config, "IntervalSeconds") as float

	questObjectDefinition = GetSetting(type_AutoHarvest, type_Config, "questObjectDefinition") as int
	crimeCheckNotSneaking = GetSetting(type_AutoHarvest, type_Config, "crimeCheckNotSneaking") as int
	crimeCheckSneaking = GetSetting(type_AutoHarvest, type_Config, "crimeCheckSneaking") as int
	playerBelongingsLoot = GetSetting(type_AutoHarvest, type_Config, "playerBelongingsLoot") as int

	questObjectLoot = GetSetting(type_AutoHarvest, type_Config, "questObjectLoot") as bool
	questObjectGlow = GetSetting(type_AutoHarvest, type_Config, "questObjectGlow") as bool
	enchantItemGlow = GetSetting(type_AutoHarvest, type_Config, "enchantItemGlow") as bool

	lockedChestLoot = GetSetting(type_AutoHarvest, type_Config, "lockedChestLoot") as bool
	lockedChestGlow = GetSetting(type_AutoHarvest, type_Config, "lockedChestGlow") as bool
	bossChestLoot = GetSetting(type_AutoHarvest, type_Config, "bossChestLoot") as bool
	bossChestGlow = GetSetting(type_AutoHarvest, type_Config, "bossChestGlow") as bool

	disableInCombat = GetSetting(type_AutoHarvest, type_Config, "disableInCombat") as bool
	disableDrawingWeapon = GetSetting(type_AutoHarvest, type_Config, "disableDrawingWeapon") as bool
	containerAnimation = GetSetting(type_AutoHarvest, type_Config, "containerAnimation") as int

	valueWeightDefault = GetSetting(type_AutoHarvest, type_Config, "valueWeightDefault") as int
	maxMiningItems = GetSetting(type_AutoHarvest, type_Config, "maxMiningItems") as int
	;DebugTrace("load_mcm: MaxMiningItems set to " + maxMiningItems)

	GetSettingToObjectArray(type_AutoHarvest, type_ItemObject, objectSettingArray)
	GetSettingToObjectArray(type_AutoHarvest, type_Container, containerSettingArray)
	GetSettingToObjectArray(type_AutoHarvest, type_Deadbody, deadbodySettingArray)
	GetSettingToObjectArray(type_AutoHarvest, type_ValueWeight, valueWeightSettingArray)
endFunction

function init()
 	;DebugTrace("init start")

	enableAutoHarvest = true
	enableLootContainer = true
	enableLootDeadbody = true

	unencumberedInCombat = false
	unencumberedInPlayerHouse = false

	pauseHotkeyCode = 34
	userlistHotkeyCode = 34
	excludelistHotkeyCode = 34

	radius = defaultRadius
	interval = defaultInterval

	questObjectDefinition = 0
	crimeCheckNotSneaking = 2
	crimeCheckSneaking = 1
	playerBelongingsLoot = 1
	containerAnimation = 0

	consolidateSettings = true

	questObjectLoot = 0
	questObjectGlow = 1
	enchantItemGlow = 1

	lockedChestLoot = 0
	lockedChestGlow = 1
	bossChestLoot = 0
	bossChestGlow = 1

	disableInCombat = 0
	disableDrawingWeapon = 0

	objectSettingArray[1] = 1
	objectSettingArray[2] = 1
	objectSettingArray[3] = 1
	objectSettingArray[4] = 1
	objectSettingArray[5] = 1
	objectSettingArray[6] = 2
	objectSettingArray[7] = 1
	objectSettingArray[8] = 0
	objectSettingArray[9]= 1
	objectSettingArray[10] = 1
	objectSettingArray[11] = 2
	objectSettingArray[12] = 0
	objectSettingArray[13] = 0
	objectSettingArray[14] = 0
	objectSettingArray[15] = 1
	objectSettingArray[16] = 2
	objectSettingArray[17] = 2
	objectSettingArray[18] = 2
	objectSettingArray[19] = 0
	objectSettingArray[20] = 0
	objectSettingArray[21] = 0
	objectSettingArray[22] = 1
	objectSettingArray[23] = 1
	objectSettingArray[24] = 0
	objectSettingArray[25] = 2
	objectSettingArray[26] = 0
	objectSettingArray[27] = 2
	objectSettingArray[28] = 1
	objectSettingArray[29] = 2
	objectSettingArray[30] = 1
	objectSettingArray[31] = 1
	objectSettingArray[32] = 0
	objectSettingArray[33] = 1
	objectSettingArray[34] = 1
	objectSettingArray[35] = 2
	
	containerSettingArray[1] = 0
	containerSettingArray[2] = 0
	containerSettingArray[3] = 1
	containerSettingArray[4] = 1
	containerSettingArray[5] = 2
	containerSettingArray[6] = 1
	containerSettingArray[7] = 1
	containerSettingArray[8] = 0
	containerSettingArray[9] = 1
	containerSettingArray[10] = 1
	containerSettingArray[11] = 2
	containerSettingArray[12] = 0
	containerSettingArray[13] = 0
	containerSettingArray[14] = 0
	containerSettingArray[15] = 1
	containerSettingArray[16] = 0
	containerSettingArray[17] = 2
	containerSettingArray[18] = 2
	containerSettingArray[19] = 0
	containerSettingArray[20] = 0
	containerSettingArray[21] = 0
	containerSettingArray[22] = 1
	containerSettingArray[23] = 1
	containerSettingArray[24] = 0
	containerSettingArray[25] = 2
	containerSettingArray[26] = 0
	containerSettingArray[27] = 2
	containerSettingArray[28] = 1
	containerSettingArray[29] = 2
	containerSettingArray[30] = 1
	containerSettingArray[31] = 1
	containerSettingArray[32] = 0
	containerSettingArray[33] = 0
	containerSettingArray[34] = 0
	containerSettingArray[35] = 1

	deadbodySettingArray[1] = 0
	deadbodySettingArray[2] = 0
	deadbodySettingArray[3] = 1
	deadbodySettingArray[4] = 1
	deadbodySettingArray[5] = 2
	deadbodySettingArray[6] = 1
	deadbodySettingArray[7] = 1
	deadbodySettingArray[8] = 0
	deadbodySettingArray[9] = 1
	deadbodySettingArray[10] = 1
	deadbodySettingArray[11] = 2
	deadbodySettingArray[12] = 0
	deadbodySettingArray[13] = 0
	deadbodySettingArray[14] = 0
	deadbodySettingArray[15] = 1
	deadbodySettingArray[16] = 0
	deadbodySettingArray[17] = 2
	deadbodySettingArray[18] = 2
	deadbodySettingArray[19] = 0
	deadbodySettingArray[20] = 0
	deadbodySettingArray[21] = 0
	deadbodySettingArray[22] = 1
	deadbodySettingArray[23] = 1
	deadbodySettingArray[24] = 0
	deadbodySettingArray[25] = 2
	deadbodySettingArray[26] = 0
	deadbodySettingArray[27] = 2
	deadbodySettingArray[28] = 1
	deadbodySettingArray[29] = 2
	deadbodySettingArray[30] = 1
	deadbodySettingArray[31] = 1
	deadbodySettingArray[32] = 0
	deadbodySettingArray[33] = 0
	deadbodySettingArray[34] = 0
	deadbodySettingArray[35] = 1

    valueWeightDefault = 100
    maxMiningItems = 10
	;DebugTrace("init: MaxMiningItems set to " + maxMiningItems)

	valueWeightSettingArray[1] = 0
	valueWeightSettingArray[2] = 0
	valueWeightSettingArray[3] = 0
	valueWeightSettingArray[4] = 0
	valueWeightSettingArray[5] = 0
	valueWeightSettingArray[6] = 0
	valueWeightSettingArray[7] = 0
	valueWeightSettingArray[8] = 0
	valueWeightSettingArray[9] = 0
	valueWeightSettingArray[10] = 0
	valueWeightSettingArray[11] = 0
	valueWeightSettingArray[12] = 0
	valueWeightSettingArray[13] = 0
	valueWeightSettingArray[14] = 0
	valueWeightSettingArray[15] = 0
	valueWeightSettingArray[16] = 0
	valueWeightSettingArray[17] = 0
	valueWeightSettingArray[18] = 0
	valueWeightSettingArray[19] = 0
	valueWeightSettingArray[20] = 0
	valueWeightSettingArray[21] = 0
	valueWeightSettingArray[22] = 0
	valueWeightSettingArray[23] = 0
	valueWeightSettingArray[24] = 0
	valueWeightSettingArray[25] = 0
	valueWeightSettingArray[26] = 0
	valueWeightSettingArray[27] = 0
	valueWeightSettingArray[28] = 0
	valueWeightSettingArray[29] = 0
	valueWeightSettingArray[30] = 0
	valueWeightSettingArray[31] = 0
	valueWeightSettingArray[32] = 0
	valueWeightSettingArray[33] = 0
	valueWeightSettingArray[34] = 0
	valueWeightSettingArray[35] = 0

    int existingFlags = g_EnableLaunched.GetValue() as int
	g_EnableLaunched.SetValue(existingFlags + 2)
	;DebugTrace("init finished")
endFunction

Function ApplySetting()

;	DebugTrace("  MCM ApplySetting start")

	PutSetting(type_Common, type_Config, "enableAutoHarvest", enableAutoHarvest as float)
	PutSetting(type_Common, type_Config, "enableLootContainer", enableLootContainer as float)
	PutSetting(type_Common, type_Config, "enableLootDeadbody", enableLootDeadbody as float)

	PutSetting(type_Common, type_Config, "unencumberedInCombat", unencumberedInCombat as float)
	PutSetting(type_Common, type_Config, "unencumberedInPlayerHouse", unencumberedInPlayerHouse as float)

	PutSetting(type_Common, type_Config, "PauseHotkeyCode", pauseHotkeyCode as float)
	PutSetting(type_Common, type_Config, "userlistHotkeyCode", userlistHotkeyCode as float)
	PutSetting(type_Common, type_Config, "excludelistHotkeyCode", excludelistHotkeyCode as float)
	PutSetting(type_Common, type_Config, "consolidateSettings", consolidateSettings as float)

	PutSetting(type_AutoHarvest, type_Config, "RadiusFeet", radius)
	PutSetting(type_AutoHarvest, type_Config, "IntervalSeconds", interval)

	PutSetting(type_AutoHarvest, type_Config, "questObjectDefinition", questObjectDefinition as float)
	PutSetting(type_AutoHarvest, type_Config, "crimeCheckNotSneaking", crimeCheckNotSneaking as float)
	PutSetting(type_AutoHarvest, type_Config, "crimeCheckSneaking", crimeCheckSneaking as float)
	PutSetting(type_AutoHarvest, type_Config, "playerBelongingsLoot", playerBelongingsLoot as float)
	PutSetting(type_AutoHarvest, type_Config, "containerAnimation", containerAnimation as float)

	PutSetting(type_AutoHarvest, type_Config, "questObjectLoot", questObjectLoot as float)
	PutSetting(type_AutoHarvest, type_Config, "questObjectGlow", questObjectGlow as float)
	PutSetting(type_AutoHarvest, type_Config, "enchantItemGlow", enchantItemGlow as float)

	PutSetting(type_AutoHarvest, type_Config, "lockedChestLoot", lockedChestLoot as float)
	PutSetting(type_AutoHarvest, type_Config, "lockedChestGlow", lockedChestGlow as float)
	PutSetting(type_AutoHarvest, type_Config, "bossChestLoot", bossChestLoot as float)
	PutSetting(type_AutoHarvest, type_Config, "bossChestGlow", bossChestGlow as float)

	PutSetting(type_AutoHarvest, type_Config, "disableInCombat", disableInCombat as float)
	PutSetting(type_AutoHarvest, type_Config, "disableDrawingWeapon", disableDrawingWeapon as float)

	PutSettingObjectArray(type_AutoHarvest, type_ItemObject, objectSettingArray)
	if (consolidateSettings)
		PutSettingObjectArray(type_AutoHarvest, type_Container, objectSettingArray)
		PutSettingObjectArray(type_AutoHarvest, type_Deadbody, objectSettingArray)
	else
		PutSettingObjectArray(type_AutoHarvest, type_Container, containerSettingArray)
		PutSettingObjectArray(type_AutoHarvest, type_Deadbody, deadbodySettingArray)
	endif

	PutSetting(type_AutoHarvest, type_Config, "valueWeightDefault", valueWeightDefault as float)
	PutSetting(type_AutoHarvest, type_Config, "maxMiningItems", maxMiningItems as float)
	PutSettingObjectArray(type_AutoHarvest, type_ValueWeight, valueWeightSettingArray)

    ; seed looting enabled according to configured settings
    bool isEnabled = enableAutoHarvest || enableLootContainer || enableLootDeadbody
    int existingFlags = g_EnableLaunched.GetValue() as int
    if (isEnabled)
        if (existingFlags >= 2)
        	g_EnableLaunched.SetValue(3)
        else
        	g_EnableLaunched.SetValue(1)
        endif
	else
        if (existingFlags >= 2)
        	g_EnableLaunched.SetValue(2)
        else
        	g_EnableLaunched.SetValue(0)
        endif
	endif

;	DebugTrace("  MCM ApplySetting finished")
endFunction

Event OnConfigInit()

;	DebugTrace("** OnConfigInit start **")

	ModName = "$AHSE_MOD_NAME"

	Pages = New String[6]
	Pages[0] = "$AHSE_COMMON1_PAGENAME"
	Pages[1] = "$AHSE_COMMON2_PAGENAME"
	Pages[2] = "$AHSE_HARVEST_PAGENAME"
	Pages[3] = "$AHSE_LOOT_CONTAINER_PAGENAME"
	Pages[4] = "$AHSE_USERLIST_PAGENAME"
	Pages[5] = "$AHSE_EXCLUDELIST_PAGENAME"

	s_iniSaveLoadArray = New String[3]
	s_iniSaveLoadArray[0] = "$AHSE_PRESET_DO_NOTHING"
	s_iniSaveLoadArray[1] = "$AHSE_PRESET_RESTORE"
	s_iniSaveLoadArray[2] = "$AHSE_PRESET_STORE"

	s_userlistSaveLoadArray = New String[3]
	s_userlistSaveLoadArray[0] = "$AHSE_PRESET_DO_NOTHING"
	s_userlistSaveLoadArray[1] = "$AHSE_USERLIST_RESTORE"
	s_userlistSaveLoadArray[2] = "$AHSE_USERLIST_STORE"

	s_excludelistSaveLoadArray = New String[3]
	s_excludelistSaveLoadArray[0] = "$AHSE_PRESET_DO_NOTHING"
	s_excludelistSaveLoadArray[1] = "$AHSE_EXCLUDELIST_RESTORE"
	s_excludelistSaveLoadArray[2] = "$AHSE_EXCLUDELIST_STORE"

	s_questObjectDefinitionArray = New String[2]
	s_questObjectDefinitionArray[0] = "$AHSE_QUEST_RELATED"
	s_questObjectDefinitionArray[1] = "$AHSE_QUEST_FLAG_ONLY"

	s_crimeCheckNotSneakingArray = New String[3]
	s_crimeCheckNotSneakingArray[0] = "$AHSE_ALL"
	s_crimeCheckNotSneakingArray[1] = "$AHSE_PREVENT_CRIMES"
	s_crimeCheckNotSneakingArray[2] = "$AHSE_OWNERLESS_ONLY"

	s_crimeCheckSneakingArray = New String[3]
	s_crimeCheckSneakingArray[0] = "$AHSE_ALL"
	s_crimeCheckSneakingArray[1] = "$AHSE_PREVENT_CRIMES"
	s_crimeCheckSneakingArray[2] = "$AHSE_OWNERLESS_ONLY"

	s_containerAnimationArray = New String[2]
	s_containerAnimationArray[0] = "$AHSE_DO_NOTHING"
;	s_containerAnimationArray[1] = "$AHSE_EMPTY_ONLY"
	s_containerAnimationArray[1] = "$AHSE_ALL"

	s_behaviorToggleArray = New String[2]
	s_behaviorToggleArray[0] = "$AHSE_DONT_PICK_UP"
	s_behaviorToggleArray[1] = "$AHSE_PICK_UP"

	s_behaviorArray = New String[3]
	s_behaviorArray[0] = "$AHSE_DONT_PICK_UP"
	s_behaviorArray[1] = "$AHSE_PICK_UP_W/O_MSG"
	s_behaviorArray[2] = "$AHSE_PICK_UP_W/MSG"

	id_objectSettingArray = New Int[36]
	objectSettingArray = New float[36]

	id_valueWeightArray = New Int[36]
	valueWeightSettingArray = New float[36]

	id_containerArray = New Int[36]
	containerSettingArray = New float[36]

	id_deadbodyArray = New Int[36]
	deadbodySettingArray = New float[36]

	s_objectTypeNameArray = New String[36]
	s_objectTypeNameArray[0]  = "$AHSE_UNKNOWN"
	s_objectTypeNameArray[1]  = "$AHSE_FLORA"
	s_objectTypeNameArray[2]  = "$AHSE_CRITTER"
	s_objectTypeNameArray[3]  = "$AHSE_INGREDIENT"
	s_objectTypeNameArray[4]  = "$AHSE_SEPTIM"
	s_objectTypeNameArray[5]  = "$AHSE_GEMS"
	s_objectTypeNameArray[6]  = "$AHSE_LOCKPICK"
	s_objectTypeNameArray[7]  = "$AHSE_ANIMAL_HIDE"
	s_objectTypeNameArray[8]  = "$AHSE_ANIMAL_PARTS"
	s_objectTypeNameArray[9]  = "$AHSE_OREINGOT"
	s_objectTypeNameArray[10] = "$AHSE_SOULGEM"
	s_objectTypeNameArray[11] = "$AHSE_KEYS"
	s_objectTypeNameArray[12] = "$AHSE_CLUTTER"
	s_objectTypeNameArray[13] = "$AHSE_CLUTTER_DWEMER"
	s_objectTypeNameArray[14] = "$AHSE_CLUTTER_BROKEN"
	s_objectTypeNameArray[15] = "$AHSE_LIGHT"
	s_objectTypeNameArray[16] = "$AHSE_BOOK"
	s_objectTypeNameArray[17] = "$AHSE_SPELLBOOK"
	s_objectTypeNameArray[18] = "$AHSE_SKILLBOOK"
	s_objectTypeNameArray[19] = "$AHSE_BOOK_READ"
	s_objectTypeNameArray[20] = "$AHSE_SPELLBOOK_READ"
	s_objectTypeNameArray[21] = "$AHSE_SKILLBOOK_READ"
	s_objectTypeNameArray[22] = "$AHSE_SCROLL"
	s_objectTypeNameArray[23] = "$AHSE_AMMO"
	s_objectTypeNameArray[24] = "$AHSE_WEAPON"
	s_objectTypeNameArray[25] = "$AHSE_ENCHANTED_WEAPON"
	s_objectTypeNameArray[26] = "$AHSE_ARMOR"
	s_objectTypeNameArray[27] = "$AHSE_ENCHANTED_ARMOR"
	s_objectTypeNameArray[28] = "$AHSE_JEWELRY"
	s_objectTypeNameArray[29] = "$AHSE_ENCHANTED_JEWELRY"
	s_objectTypeNameArray[30] = "$AHSE_POTION"
	s_objectTypeNameArray[31] = "$AHSE_POISON"
	s_objectTypeNameArray[32] = "$AHSE_FOOD"
	s_objectTypeNameArray[33] = "$AHSE_DRINK"
	s_objectTypeNameArray[34] = "$AHSE_OREVEIN"
	s_objectTypeNameArray[35] = "$AHSE_USERLIST"

	userlist_form = Game.GetFormFromFile(0x0333C, "AutoHarvestSE.esp") as Formlist
	excludelist_form = Game.GetFormFromFile(0x0333D, "AutoHarvestSE.esp") as Formlist
	
	pushLocationToExcludelist = false
	pushCellToExcludelist= false

;	DebugTrace("** OnConfigInit finished **")
endEvent

int function GetVersion()
	return 6
endFunction

Event OnVersionUpdate(int a_version)
;	DebugTrace("OnVersionUpdate start" + a_version)

	if (a_version >=5 && CurrentVersion < 5)
		s_containerAnimationArray = New String[2]
		s_containerAnimationArray[0] = "$AHSE_DO_NOTHING"
;		s_containerAnimationArray[1] = "$AHSE_EMPTY_ONLY"
		s_containerAnimationArray[1] = "$AHSE_ALL"
		containerAnimation = 0

		type_Config = 1
		type_ItemObject = 2
		type_Container = 3
		type_Deadbody = 4
		type_ValueWeight = 5
		type_MaxItemCount = 6
	endif
	if (a_version >= 6 && CurrentVersion < 6)
		; Major revision to reduce script dependence and autodetect lootables
		Debug.Trace(self + ": Updating script to version " + a_version)
		OnConfigInit()
	endIf

;	DebugTrace("OnVersionUpdate finished" + a_version)
endEvent

Event OnGameReload()
	parent.OnGameReload()

	if (gameReloadLock)
		return
	endif
	gameReloadLock = true

	;DebugTrace("* OnGameReload start * enabled: " + g_EnableLaunched.GetValue())
	if ((g_EnableLaunched.GetValue() as int) < 2)
		init()
	endif

	ApplySetting()
	eventScript.ApplySetting()
	
;	DebugTrace("* OnGameReload finished *")
	gameReloadLock = false
endEvent

Event OnConfigOpen()
;	DebugTrace("OnConfigOpen")

	if (consolidateSettings)
		Pages = New String[5]
		Pages[0] = "$AHSE_COMMON1_PAGENAME"
		Pages[1] = "$AHSE_COMMON2_PAGENAME"
		Pages[2] = "$AHSE_CONSOLIDATEDSETTINGS_PAGENAME"
		Pages[3] = "$AHSE_USERLIST_PAGENAME"
		Pages[4] = "$AHSE_EXCLUDELIST_PAGENAME"
	else
		Pages = New String[6]
		Pages[0] = "$AHSE_COMMON1_PAGENAME"
		Pages[1] = "$AHSE_COMMON2_PAGENAME"
		Pages[2] = "$AHSE_HARVEST_PAGENAME"
		Pages[3] = "$AHSE_LOOT_CONTAINER_PAGENAME"
		Pages[4] = "$AHSE_USERLIST_PAGENAME"
		Pages[5] = "$AHSE_EXCLUDELIST_PAGENAME"
	endif
	
	int index = 0
	int max_size = 0
	
	max_size = userlist_form.GetSize()
	if (max_size > 0)
		id_userlist_array = Utility.CreateIntArray(max_size)
		userlist_form_array = userlist_form.toArray()
		userlist_name_array = Utility.CreateStringArray(max_size, "")
		userlist_flag_array = Utility.CreateBoolArray(max_size, false)
		
		index = 0
		while(index < max_size)
			if (userlist_form_array[index])
				userlist_name_array[index] = (userlist_form_array[index]).GetName()
				userlist_flag_array[index] = true
			endif
			index += 1
		endWhile
	endif

	max_size = excludelist_form.GetSize()
	if (max_size > 0)
		id_excludelist_array = Utility.CreateIntArray(max_size)
		excludelist_form_array = Utility.CreateFormArray(max_size)
		excludelist_name_array = Utility.CreateStringArray(max_size)
		excludelist_flag_array = Utility.CreateBoolArray(max_size)
		
		index = 0
		while(index < max_size)

;			DebugTrace("   " + index)
		
			excludelist_form_array[index] = excludelist_form.GetAt(index)
			excludelist_name_array[index] = (excludelist_form_array[index]).GetName()
			excludelist_flag_array[index] = true
			
;			DebugTrace(excludelist_name_array[index])

			
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
				translation = Replace(translation, "{ITEMNAME}", m_forms[index].GetName())
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

	TidyListUp(userlist_form, userlist_form_array, userlist_flag_array, "$AHSE_USERLIST_REMOVED")
	TidyListUp(excludelist_form, excludelist_form_array, excludelist_flag_array, "$AHSE_EXCLUDELIST_REMOVED")
	
	;--- INI File operation --------------------
	if (iniSaveLoad == 1) ; load/restore
		LoadIniFile()
		Load_mcm()
		Debug.Notification("$AHSE_PRESET_RESTORE_MSG")
	elseif (iniSaveLoad == 2) ; save/store
		SaveIniFile()
		Debug.Notification("$AHSE_PRESET_STORE_MSG")
	endif
	iniSaveLoad = 0


	;--- Userlist File operation --------------------
	if (userlistSaveLoad == 1) ; load/restore
		bool result = LoadUserlist()
		if (result)
			Debug.Notification("$AHSE_USERLIST_RESTORE_MSG")
		endif
	elseif (userlistSaveLoad == 2) ; save/store
		bool result = SaveUserlist()
		if (result)
			Debug.Notification("$AHSE_USERLIST_STORE_MSG")
		endif
	endif
	SyncUserlist()
	userlistSaveLoad = 0
	
	
	;--- Excludelist File operation --------------------
	if (excludelistSaveLoad == 1) ; load/restore
		bool result = LoadExcludelist()
		if (result)
			Debug.Notification("$AHSE_EXCLUDELIST_RESTORE_MSG")
		endif
	elseif (excludelistSaveLoad == 2) ; save/store
		bool result = SaveExcludelist()
		if (result)
			Debug.Notification("$AHSE_EXCLUDELIST_STORE_MSG")
		endif
	endif
	SyncExcludelist()
	excludelistSaveLoad = 0
	
	
	if (pushLocationToExcludelist)
		form locForm = Game.GetPlayer().GetCurrentLocation() as form
		if (locForm)
			ManageList(excludelist_form, locForm, location_type_excluded, "$AHSE_EXCLUDELIST_ADDED", "$AHSE_EXCLUDELIST_REMOVED")
		endif
		pushLocationToExcludelist = false
	endif

	if (pushCellToExcludelist)
		form cellForm = Game.GetPlayer().GetParentCell() as form
		if (cellForm)
			ManageList(excludelist_form, cellForm, location_type_excluded, "$AHSE_EXCLUDELIST_ADDED", "$AHSE_EXCLUDELIST_REMOVED")
		endif
		pushCellToExcludelist= false
	endif

	ApplySetting()
	eventScript.ApplySetting()
endEvent

event OnPageReset(string currentPage)
;	DebugTrace("OnPageReset")

	If (currentPage == "")
		LoadCustomContent("towawot/AutoHarvestSE.dds")
		Return
	Else
		UnloadCustomContent()
	EndIf

	if (currentPage == Pages[0])
	

; 	======================== LEFT ========================
		SetCursorFillMode(TOP_TO_BOTTOM)

		AddHeaderOption("$AHSE_COMMON_HEADER")
		AddToggleOptionST("enableAutoHarvest", "$AHSE_ENABLE_AUTOHARVEST", enableAutoHarvest)
		AddToggleOptionST("enableLootContainer", "$AHSE_ENABLE_LOOT_CONTAINER", enableLootContainer)
		AddToggleOptionST("enableLootDeadbody", "$AHSE_ENABLE_LOOT_DEADBODY", enableLootDeadbody)

		AddEmptyOption()

		AddHeaderOption("$AHSE_INTERVAL_HEADER")
		AddSliderOptionST("Radius", "$AHSE_RADIUS", radius, "$AHSE{0}UNIT")
		AddSliderOptionST("Interval", "$AHSE_INTERVAL", interval, "$AHSE{1}SEC")

		AddEmptyOption()

		AddHeaderOption("$AHSE_EXCLUDE_LOCATION_HEADER")

		Form locFrom = Game.GetPlayer().GetCurrentLocation() as Form
		int flagLocation = OPTION_FLAG_NONE
		if (!locFrom)
			flagLocation = OPTION_FLAG_DISABLED
		endif
		AddToggleOptionST("pushLocationToExcludelist", "$AHSE_LOCATION_TO_EXCLUDELIST", pushLocationToExcludelist, flagLocation)

		Form cellFrom = Game.GetPlayer().GetParentCell() as Form
		int flagCell = OPTION_FLAG_NONE
		if (!cellFrom)
			flagCell = OPTION_FLAG_DISABLED
		endif
		
		AddToggleOptionST("pushCellToExcludelist", "$AHSE_CELL_TO_EXCLUDELIST", pushCellToExcludelist, flagCell)

; 	======================== RIGHT ========================
		SetCursorPosition(1)

		AddHeaderOption("$AHSE_KEYCODE_HEADER")
		AddKeyMapOptionST("pauseHotkeyCode", "$AHSE_PAUSE_KEY", pauseHotkeyCode)
		AddKeyMapOptionST("userlistHotkeyCode", "$AHSE_USERLIST_KEY", userlistHotkeyCode)
		AddKeyMapOptionST("excludelistHotkeyCode", "$AHSE_EXCLUDELIST_KEY", excludelistHotkeyCode)

		AddEmptyOption()

		AddHeaderOption("$AHSE_CONSOLIDATED_HEADER")
		AddToggleOptionST("consolidateSettings", "$AHSE_CONSOLIDATED_SETTINGS", consolidateSettings)

		AddEmptyOption()

		AddHeaderOption("$AHSE_PRESET_HEADER")
		AddMenuOptionST("iniSaveLoad", "$AHSE_FILE_OPERATION", s_iniSaveLoadArray[iniSaveLoad])
		AddMenuOptionST("userlistSaveLoad", "$AHSE_USERLIST_FILE_OPERATION", s_userlistSaveLoadArray[userlistSaveLoad])
		AddMenuOptionST("excludelistSaveLoad", "$AHSE_EXCLUDELIST_FILE_OPERATION", s_excludelistSaveLoadArray[excludelistSaveLoad])
		
	elseif (currentPage == Pages[1])

; 	======================== LEFT ========================
		SetCursorFillMode(TOP_TO_BOTTOM)

		AddHeaderOption("$AHSE_BEHAVIOR_HEADER")
		AddToggleOptionST("questObjectLoot", "$AHSE_QUESTOBJECT_LOOT", questObjectLoot)
		AddToggleOptionST("lockedChestLoot", "$AHSE_LOCKEDCHEST_LOOT", lockedChestLoot)
		AddToggleOptionST("bossChestLoot", "$AHSE_BOSSCHEST_LOOT", bossChestLoot)
		AddTextOptionST("questObjectDefinition", "$AHSE_QUESTOBJECT_DEFINITION", s_questObjectDefinitionArray[questObjectDefinition])
		AddTextOptionST("crimeCheckNotSneaking", "$AHSE_CRIME_CHECK_NOT_SNEAKING", s_crimeCheckNotSneakingArray[crimeCheckNotSneaking])
		AddTextOptionST("crimeCheckSneaking", "$AHSE_CRIME_CHECK_SNEAKING", s_crimeCheckSneakingArray[crimeCheckSneaking])
		AddTextOptionST("playerBelongingsLoot", "$AHSE_PLAYER_BELONGINGS_LOOT", s_behaviorToggleArray[playerBelongingsLoot])
		AddTextOptionST("containerAnimation", "$AHSE_CONTAINER_ANIMATION", s_containerAnimationArray[containerAnimation])

		AddHeaderOption("$AHSE_UNENCUMBERED_HEADER")
		AddToggleOptionST("unencumberedInCombat", "$AHSE_UNENCUMBERED_COMBAT", unencumberedInCombat)
		AddToggleOptionST("unencumberedInPlayerHouse", "$AHSE_UNENCUMBERED_PLAYER_HOME", unencumberedInPlayerHouse)
		
; 	======================== RIGHT ========================
		SetCursorPosition(1)

		AddHeaderOption("$AHSE_DISABLE_HEADER")
		AddToggleOptionST("disableInCombat", "$AHSE_DISABLE_INCOMBAT", disableInCombat)
		AddToggleOptionST("disableDrawingWeapon", "$AHSE_DISABLE_DRAWINGWEAPON", disableDrawingWeapon)
		
		AddEmptyOption()
		AddHeaderOption("$AHSE_VW_DEFAULT_HEADER")
	    AddSliderOptionST("ValueWeightDefault", "$AHSE_VW_DEFAULT", valueWeightDefault)
	    AddSliderOptionST("MaxMiningItems", "$AHSE_MAX_MINING_ITEMS", maxMiningItems)

		AddEmptyOption()
		AddHeaderOption("$AHSE_GLOWS_HEADER")
		AddToggleOptionST("enchantItemGlow", "$AHSE_ENCHANTITEM_GLOW", enchantItemGlow)
		AddToggleOptionST("questObjectGlow", "$AHSE_QUESTOBJECT_GLOW", questObjectGlow)
		AddToggleOptionST("lockedChestGlow", "$AHSE_LOCKEDCHEST_GLOW", lockedChestGlow)
		AddToggleOptionST("bossChestGlow", "$AHSE_BOSSCHEST_GLOW", bossChestGlow)
		
	elseif (currentPage == Pages[2]) ; object harvester
		
; 	======================== LEFT ========================
		SetCursorFillMode(TOP_TO_BOTTOM)

		if (consolidateSettings)
			AddHeaderOption("$AHSE_CONSOLIDATED_HEADER")
		else
			AddHeaderOption("$AHSE_ITEM_OBJECT_HEADER")
		endif
		
		int index = 1
		while (index <= (s_objectTypeNameArray.length - 1))
			if (index == objType_Mine)
				id_objectSettingArray[index] = AddTextOption(s_objectTypeNameArray[index], s_behaviorToggleArray[(objectSettingArray[index] as int)])
			else
				id_objectSettingArray[index] = AddTextOption(s_objectTypeNameArray[index], s_behaviorArray[(objectSettingArray[index] as int)])
			endif
			index += 1
		endWhile

; 	======================== RIGHT ========================
		SetCursorPosition(1)

		AddHeaderOption("$AHSE_VALUE/WEIGHT_HEADER")

		AddEmptyOption()
		AddEmptyOption()
		AddEmptyOption()

		index = 4
		while (index <= (s_objectTypeNameArray.length - 1))
			if (index == objType_Mine)
				AddEmptyOption()
			elseif (index == objType_Ammo)
				AddEmptyOption()
			else
				id_valueWeightArray[index] = AddSliderOption(s_objectTypeNameArray[index], valueWeightSettingArray[index], "$AHSE_V/W{1}")
			endif
			index += 1
		endWhile
		
		if (objpage_read_once && consolidateSettings)
			ShowMessage("$AHSE_OBJECTPAGE_ANNOUNCE_MSG", "$AHSE_OK")
			objpage_read_once = false
		endif
		
	elseif (!consolidateSettings && currentPage == Pages[3]) ; container/deadbody

		int flag_option = OPTION_FLAG_NONE
		if (consolidateSettings)
			flag_option = OPTION_FLAG_DISABLED
		endif

; 	======================== LEFT ========================
		SetCursorFillMode(TOP_TO_BOTTOM)

		AddHeaderOption("$AHSE_CONTAINER_HEADER")

;		AddEmptyOption()
;		AddEmptyOption()
;		AddEmptyOption()

		int index = 4
		while (index <= (s_objectTypeNameArray.length - 1))
			if (index == objType_Mine)
				AddEmptyOption()
			else
				id_containerArray[index] = AddTextOption(s_objectTypeNameArray[index], s_behaviorArray[(containerSettingArray[index] as int)], flag_option)
			endif
			index += 1
		endWhile

; 	======================== RIGHT ========================
		SetCursorPosition(1)

		AddHeaderOption("$AHSE_DEADBODY_HEADER")

;		AddEmptyOption()
;		AddEmptyOption()
;		AddEmptyOption()

		index = 4
		while (index <= (s_objectTypeNameArray.length - 1))
			if (index == objType_Mine)
				AddEmptyOption()
			else
				id_deadbodyArray[index] = AddTextOption(s_objectTypeNameArray[index], s_behaviorArray[(deadbodySettingArray[index] as int)], flag_option)
			endif
			index += 1
		endWhile
	elseif ((!consolidateSettings && currentPage == Pages[4]) || (consolidateSettings && currentPage == Pages[3])) ; userlist

		int size = userlist_form.GetSize()
		if (size == 0)
			return
		endif

		int index = 0
		while (index < size)
			if (!userlist_form_array[index])
				id_userlist_array[index] = AddToggleOption("$AHSE_UNKNOWN", userlist_flag_array[index], OPTION_FLAG_DISABLED)
			elseif (!userlist_name_array[index])
				id_userlist_array[index] = AddToggleOption("$AHSE_UNKNOWN", userlist_flag_array[index])
			else
				id_userlist_array[index] = AddToggleOption(userlist_name_array[index], userlist_flag_array[index])
			endif
			index += 1
		endWhile
	elseif ((!consolidateSettings && currentPage == Pages[5]) || (consolidateSettings && currentPage == Pages[4])) ; exclude list
		int size = excludelist_form.GetSize()
		if (size == 0)
			return
		endif

		int index = 0
		while (index < size)
			if (!excludelist_form_array[index])
				id_excludelist_array[index] = AddToggleOption("$AHSE_UNKNOWN", excludelist_flag_array[index], OPTION_FLAG_DISABLED)
			elseif (!excludelist_name_array[index])
				id_excludelist_array[index] = AddToggleOption("$AHSE_UNKNOWN", excludelist_flag_array[index])
			else
				id_excludelist_array[index] = AddToggleOption(excludelist_name_array[index], excludelist_flag_array[index])
			endif
			index += 1
		endWhile
	endif
endEvent

Event OnOptionSelect(int a_option)
	int index = -1

	index = id_objectSettingArray.find(a_option)
	if (index >= 0)
		string keyName = GetObjectKeyString(index)
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
;			PutSetting(type_AutoHarvest, type_ItemObject, keyName, objectSettingArray[index])
		endif
		return
	endif

	index = id_containerArray.find(a_option)
	if (index >= 0)
		string keyName = GetObjectKeyString(index)
		if (keyName != "unknown")
			int size = s_behaviorArray.length
			containerSettingArray[index] = CycleInt(containerSettingArray[index] as int, size)
;			PutSetting(type_AutoHarvest, type_Container, keyName, containerSettingArray[index])
			SetTextOptionValue(a_option, s_behaviorArray[(containerSettingArray[index] as int)])
		endif
		return
	endif

	index = id_deadbodyArray.find(a_option)
	if (index >= 0)
		string keyName = GetObjectKeyString(index)
		if (keyName != "unknown")
			int size = s_behaviorArray.length
			deadbodySettingArray[index] = CycleInt(deadbodySettingArray[index] as int, size)
;			PutSetting(type_AutoHarvest, type_Deadbody, keyName, deadbodySettingArray[index])
			SetTextOptionValue(a_option, s_behaviorArray[(deadbodySettingArray[index] as int)])
		endif
		return
	endif
	
	index = id_userlist_array.find(a_option)
	if (index >= 0)
		userlist_flag_array[index] = !userlist_flag_array[index]
		SetToggleOptionValue(a_option, userlist_flag_array[index])
		return
	endif

	index = id_excludelist_array.find(a_option)
	if (index >= 0)
		excludelist_flag_array[index] = !excludelist_flag_array[index]
		SetToggleOptionValue(a_option, excludelist_flag_array[index])
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
		string keyName = GetObjectKeyString(index)
		if (keyName != "unknown")
			valueWeightSettingArray[index] = a_value
;			PutSetting(type_AutoHarvest, type_ValueWeight, keyName, valueWeightSettingArray[index])
			SetSliderOptionValue(a_option, a_value, "$AHSE_V/W{1}")
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

	index = id_containerArray.find(a_option)
	if (index > -1)
		SetInfoText(s_objectTypeNameArray[index])
		return
	endif

	index = id_deadbodyArray.find(a_option)
	if (index > -1)
		SetInfoText(s_objectTypeNameArray[index])
		return
	endif

	index = id_valueWeightArray.find(a_option)
	if (index > -1)
		SetInfoText(s_objectTypeNameArray[index])
		return
	endif
	
	index = id_userlist_array.find(a_option)
	if (index > -1)
		string translation = GetTranslation("$AHSE_DESC_USERLIST")
		if (!translation)
			return
		endif
		form item = userlist_form_array[index]
		if (!item)
			return
		endif

		string[] targets = New String[3]
		targets[0] = "{OBJ_TYPE}"
		targets[1] = "{FROM_ID}"
		targets[2] = "{ESP_NAME}"

		string[] replacements = New String[3]
		replacements[0] = GetTextObjectType(item)
		replacements[1] = GetTextFormID(item)
		replacements[2] = GetPluginName(item)
		
		translation = ReplaceArray(translation, targets, replacements)
		if (translation)
			SetInfoText(translation)
		endif
		return
	endif
	
	index = id_excludelist_array.find(a_option)
	if (index > -1)
		string translation = GetTranslation("$AHSE_DESC_EXCLUDELIST")
		if (!translation)
			return
		endif

		form item = excludelist_form_array[index]
		if (!item)
			return
		endif

		string[] targets = New String[3]
		targets[0] = "{OBJ_TYPE}"
		targets[1] = "{FROM_ID}"
		targets[2] = "{ESP_NAME}"

		string[] replacements = New String[3]
		replacements[0] = GetTextObjectType(item)
		replacements[1] = GetTextFormID(item)
		replacements[2] = GetPluginName(item)
		
		translation = ReplaceArray(translation, targets, replacements)
		if (translation)
			SetInfoText(translation)
		endif
		return
	endif
endEvent

state pushLocationToExcludelist
	event OnSelectST()
		pushLocationToExcludelist = !(pushLocationToExcludelist as bool)
		SetToggleOptionValueST(pushLocationToExcludelist)
	endEvent

	event OnDefaultST()
		pushLocationToExcludelist = false
		SetToggleOptionValueST(pushLocationToExcludelist)
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_LOCATION_TO_EXCLUDELIST")
	endEvent
endState

state pushCellToExcludelist
	event OnSelectST()
		pushCellToExcludelist = !(pushCellToExcludelist as bool)
		SetToggleOptionValueST(pushCellToExcludelist)
	endEvent

	event OnDefaultST()
		pushCellToExcludelist = false
		SetToggleOptionValueST(pushCellToExcludelist)
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_CELL_TO_EXCLUDELIST")
	endEvent
endState

;Int function GetStateOptionIndex(String a_stateName)
;	return parent.GetStateOptionIndex(a_stateName)
;endFunction

state enableAutoHarvest
;	Event OnBeginState()
;		int result = GetStateOptionIndex(self.GetState())
;		DebugTrace("Entered the running state! " + result)
;	EndEvent

	event OnSelectST()
		enableAutoHarvest = !(enableAutoHarvest as bool)
		SetToggleOptionValueST(enableAutoHarvest)
	endEvent

	event OnDefaultST()
		enableAutoHarvest = true
		SetToggleOptionValueST(enableAutoHarvest)
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_ENABLE_AUTOHARVEST")
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
		SetInfoText("$AHSE_DESC_ENABLE_LOOT_CONTAINER")
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
		SetInfoText("$AHSE_DESC_ENABLE_LOOT_DEADBODY")
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
		SetInfoText("$AHSE_DESC_UNENCUMBERED_COMBAT")
	endEvent
endState

state unencumberedInPlayerHouse
	event OnSelectST()
		unencumberedInPlayerHouse = !(unencumberedInPlayerHouse as bool)
		SetToggleOptionValueST(unencumberedInPlayerHouse)
	endEvent

	event OnDefaultST()
		unencumberedInPlayerHouse = false
		SetToggleOptionValueST(unencumberedInPlayerHouse)
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_UNENCUMBERED_PLAYER_HOME")
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
		SetSliderOptionValueST(radius, "$AHSE{0}UNIT")
	endEvent

	event OnDefaultST()
		radius = defaultRadius
		SetSliderOptionValueST(radius, "$AHSE{0}UNIT")
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_RADIUS")
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
		SetSliderOptionValueST(interval, "$AHSE{1}SEC")
	endEvent

	event OnDefaultST()
		interval = defaultInterval
		SetSliderOptionValueST(interval, "$AHSE{1}SEC")
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_INTERVAL")
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
		SetInfoText("$AHSE_DESC_VW_DEFAULT")
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
		maxMiningItems = value as int
    	;DebugTrace("OnSliderAcceptST: MaxMiningItems set to " + maxMiningItems)
		SetSliderOptionValueST(maxMiningItems)
	endEvent

	event OnDefaultST()
		maxMiningItems = maxMiningItemsDefault
    	;DebugTrace("OnDefaultST: MaxMiningItems set to " + maxMiningItems)
		SetSliderOptionValueST(maxMiningItems)
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_MAX_MINING_ITEMS")
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
		SetInfoText("$AHSE_DESC_PAUSE_KEY")
	endEvent
endState

state userlistHotkeyCode
	event OnKeyMapChangeST(int newKeyCode, string conflictControl, string conflictName)
		userlistHotkeyCode = newKeyCode
		SetKeyMapOptionValueST(userlistHotkeyCode)
	endEvent

	event OnDefaultST()
		userlistHotkeyCode = 34
		SetKeyMapOptionValueST(userlistHotkeyCode)
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_USERLIST_KEY")
	endEvent
endState

state excludelistHotkeyCode
	event OnKeyMapChangeST(int newKeyCode, string conflictControl, string conflictName)
		excludelistHotkeyCode = newKeyCode
		SetKeyMapOptionValueST(excludelistHotkeyCode)
	endEvent

	event OnDefaultST()
		excludelistHotkeyCode = 34
		SetKeyMapOptionValueST(excludelistHotkeyCode)
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_EXCLUDELIST_KEY")
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
			list_warning = "$AHSE_LOAD_WARNING_MSG"
		elseif (index == 2)
			list_warning = "$AHSE_SAVE_WARNING_MSG"
		else
			return
		endif
		bool continue = ShowMessage(list_warning, true, "$AHSE_OK", "$AHSE_CANCEL")
		if (continue)
			iniSaveLoad = index
		SetMenuOptionValueST(s_iniSaveLoadArray[iniSaveLoad])
		endif
	endEvent

	event OnDefaultST()
		iniSaveLoad = 0
		SetMenuOptionValueST(s_iniSaveLoadArray[iniSaveLoad])
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_FILE_OPERATION")
	endEvent
endState

state userlistSaveLoad
	event OnMenuOpenST()
		SetMenuDialogStartIndex(userlistSaveLoad)
		SetMenuDialogDefaultIndex(0)
		SetMenuDialogOptions(s_userlistSaveLoadArray)
	endEvent

	event OnMenuAcceptST(int index)
		string list_warning
		if (index == 1)
			list_warning = "$AHSE_LOAD_WARNING_MSG"
		elseif (index == 2)
			list_warning = "$AHSE_SAVE_WARNING_MSG"
		else
			return
		endif
		bool continue = ShowMessage(list_warning, true, "$AHSE_OK", "$AHSE_CANCEL")
		if (continue)
			userlistSaveLoad = index
			SetMenuOptionValueST(s_userlistSaveLoadArray[userlistSaveLoad])
		endif
	endEvent

	event OnDefaultST()
		userlistSaveLoad = 0
		SetMenuOptionValueST(s_userlistSaveLoadArray[userlistSaveLoad])
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_USERLIST_FILE_OPERATION")
	endEvent
endState

state excludelistSaveLoad
	event OnMenuOpenST()
		SetMenuDialogStartIndex(excludelistSaveLoad)
		SetMenuDialogDefaultIndex(0)
		SetMenuDialogOptions(s_excludelistSaveLoadArray)
	endEvent

	event OnMenuAcceptST(int index)
		if (index == 0)
			return
		endif
		
		string list_warning
		if (index == 1)
			list_warning = "$AHSE_LOAD_WARNING_MSG"
		elseif (index == 2)
			list_warning = "$AHSE_SAVE_WARNING_MSG"
		endif
		bool continue = ShowMessage(list_warning, true, "$AHSE_OK", "$AHSE_CANCEL")
		if (continue)
			excludelistSaveLoad = index
			SetMenuOptionValueST(s_excludelistSaveLoadArray[excludelistSaveLoad])
		endif
	endEvent

	event OnDefaultST()
		excludelistSaveLoad = 0
		SetMenuOptionValueST(s_excludelistSaveLoadArray[excludelistSaveLoad])
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_EXCLUDELIST_FILE_OPERATION")
	endEvent
endState

state consolidateSettings
;	Event OnBeginState()
;		int result = GetStateOptionIndex(self.GetState())
;		DebugTrace("Entered the running state! " + result)
;	EndEvent

	event OnSelectST()
		consolidateSettings = !(consolidateSettings as bool)
		SetToggleOptionValueST(consolidateSettings)
		ShowMessage("$AHSE_REOPEN_MSG", false, "$AHSE_OK")
	endEvent

	event OnDefaultST()
		consolidateSettings = true
		SetToggleOptionValueST(consolidateSettings)
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_CONSOLIDATED_SETTINGS")
	endEvent

;	event OnEndState()
;		DebugTrace("Leaving the running state!")
;	endEvent
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
		SetInfoText("$AHSE_DESC_CRIME_CHECK_NOT_SNEAKING")
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
		SetInfoText("$AHSE_DESC_CRIME_CHECK_SNEAKING")
	endEvent
endState

state playerBelongingsLoot
	event OnSelectST()
		int size = s_behaviorToggleArray.length
		playerBelongingsLoot = CycleInt(playerBelongingsLoot, size)
		SetTextOptionValueST(s_behaviorToggleArray[playerBelongingsLoot])
	endEvent

	event OnDefaultST()
		playerBelongingsLoot = 1
		SetTextOptionValueST(s_behaviorToggleArray[playerBelongingsLoot])
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_PLAYER_BELONGINGS_LOOT")
	endEvent
endState

state questObjectDefinition
	event OnSelectST()
		int size = s_questObjectDefinitionArray.length
		questObjectDefinition = CycleInt(questObjectDefinition, size)
		SetTextOptionValueST(s_questObjectDefinitionArray[questObjectDefinition])
	endEvent

	event OnDefaultST()
		questObjectDefinition = 1
		SetTextOptionValueST(s_questObjectDefinitionArray[questObjectDefinition])
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_QUESTOBJECT_DEFINITION")
	endEvent
endState

state questObjectLoot
	event OnSelectST()
		questObjectLoot = !(questObjectLoot as bool)
		SetToggleOptionValueST(questObjectLoot)
	endEvent

	event OnDefaultST()
		questObjectLoot = false
		SetToggleOptionValueST(questObjectLoot)
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_QUESTOBJECT_LOOT")
	endEvent
endState

state questObjectGlow
	event OnSelectST()
		questObjectGlow = !(questObjectGlow as bool)
		SetToggleOptionValueST(questObjectGlow)
	endEvent

	event OnDefaultST()
		questObjectGlow = true
		SetToggleOptionValueST(questObjectGlow)
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_QUESTOBJECT_GLOW")
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
		SetInfoText("$AHSE_DESC_ENCHANTITEM_GLOW")
	endEvent
endState

state lockedChestLoot
	event OnSelectST()
		lockedChestLoot = !(lockedChestLoot as bool)
		SetToggleOptionValueST(lockedChestLoot)
	endEvent

	event OnDefaultST()
		lockedChestLoot = false
		SetToggleOptionValueST(lockedChestLoot)
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_LOCKEDCHEST_LOOT")
	endEvent
endState

state lockedChestGlow
	event OnSelectST()
		lockedChestGlow = !(lockedChestGlow as bool)
		SetToggleOptionValueST(lockedChestGlow)
	endEvent

	event OnDefaultST()
		lockedChestGlow = true
		SetToggleOptionValueST(lockedChestGlow)
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_LOCKEDCHEST_GLOW")
	endEvent
endState

state bossChestLoot
	event OnSelectST()
		bossChestLoot = !(bossChestLoot as bool)
		SetToggleOptionValueST(bossChestLoot)
	endEvent

	event OnDefaultST()
		bossChestLoot = false
		SetToggleOptionValueST(bossChestLoot)
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_BOSSCHEST_LOOT")
	endEvent
endState

state bossChestGlow
	event OnSelectST()
		bossChestGlow = !(bossChestGlow as bool)
		SetToggleOptionValueST(bossChestGlow)
	endEvent

	event OnDefaultST()
		bossChestGlow = true
		SetToggleOptionValueST(bossChestGlow)
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_BOSSCHEST_GLOW")
	endEvent
endState

state disableInCombat
	event OnSelectST()
		disableInCombat = !(disableInCombat as bool)
		SetToggleOptionValueST(disableInCombat)
	endEvent

	event OnDefaultST()
		disableInCombat = false
		SetToggleOptionValueST(disableInCombat)
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_DISABLE_INCOMBAT")
	endEvent
endState

state disableDrawingWeapon
	event OnSelectST()
		disableDrawingWeapon = !(disableDrawingWeapon as bool)
		SetToggleOptionValueST(disableDrawingWeapon)
	endEvent

	event OnDefaultST()
		disableDrawingWeapon = false
		SetToggleOptionValueST(disableDrawingWeapon)
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_DISABLE_DRAWINGWEAPON")
	endEvent
endState

state containerAnimation
	event OnSelectST()
		int size = s_containerAnimationArray.length
		containerAnimation = CycleInt(containerAnimation, size)
		SetTextOptionValueST(s_containerAnimationArray[containerAnimation])
	endEvent

	event OnDefaultST()
		containerAnimation = 0
		SetTextOptionValueST(s_containerAnimationArray[containerAnimation])
	endEvent

	event OnHighlightST()
		SetInfoText("$AHSE_DESC_CONTAINER_ANIMATION")
	endEvent
endState
