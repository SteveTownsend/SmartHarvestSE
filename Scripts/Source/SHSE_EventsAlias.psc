Scriptname SHSE_EventsAlias extends ReferenceAlias  

import SHSE_PluginProxy
import SHSE_MCM
GlobalVariable Property g_LootingEnabled Auto

int CACOModIndex
int FossilMiningModIndex
int HearthfireExtendedModIndex
int MagicChestModIndex
int VidanisBagOfHoldingModIndex

bool scanActive = True
int pluginNonce
bool pluginDelayed
bool mcmOpen = False

GlobalVariable StrikesBeforeCollection

Message property whitelist_message auto
Message property to_list_message auto
Message property DisposeMsg auto
Container Property NameToDisplay auto

; INIFile::PrimaryType
int type_Common = 1
int type_Harvest = 2

; INIFile::SecondaryType
int itemSourceLoose = 2
int itemSourceNPC = 4

; object types must be in sync with the native DLL
int objType_Flora
int objType_Critter
int objType_Septim
int objType_Soulgem
int objType_Mine
int objType_Book
int objType_skillBookRead

Actor player
bool logEvent

Form[] addedItems
int[] addedItemScope
int[] addedItemType
int maxAddedItems

int currentAddedItem
bool collectionsInUse
int resource_Ore
int resource_Geode
int resource_Volcanic
int resource_VolcanicDigSite
LeveledItem FOS_LItemFossilTierOneGeode
LeveledItem FOS_LItemFossilTierOneVolcanic
LeveledItem FOS_LItemFossilTierOneyum
LeveledItem FOS_LItemFossilTierOneVolcanicDigSite
LeveledItem FOS_LItemFossilTierTwoVolcanic

; supported Effect Shaders
EffectShader redShader          ; red
EffectShader flamesShader       ; flames
EffectShader purpleShader       ; purple
EffectShader copperShader       ; copper/bronze
EffectShader blueShader         ; blue
EffectShader goldShader         ; gold
EffectShader greenShader        ; green
EffectShader silverShader       ; silver
; Effect Shaders by category
EffectShader[] defaultCategoryShaders
EffectShader[] categoryShaders

; FormType from CommonLibSSE - this is core Game data, so invariant
int getType_kTree = 38
int getType_kFlora = 39

; legacy, unreliable
Formlist Property whitelist_form auto
Formlist Property blacklist_form auto
; replacement, easier to handle, stable
Form[] whiteListedForms
int whiteListSize
Form[] blackListedForms
int blackListSize

int location_type_whitelist
int location_type_blacklist
int list_type_transfer
int list_type_in_use_items
int pauseKeyCode
int whiteListKeyCode
int blackListKeyCode
bool keyHandlingActive

Form[] transferList
bool[] transferListInUse
string[] transferNames
int transferListSize

int maxMiningItems
int infiniteWeight

int glowReasonLockedContainer
int glowReasonBossContainer
int glowReasonQuestObject
int glowReasonCollectible
int glowReasonEnchantedItem
int glowReasonHighValue
int glowReasonPlayerProperty
int glowReasonSimpleTarget

Perk spergProspector

Function Prepare(Actor playerref, bool useLog)
    logEvent = useLog
    player = playerref
    ;check for SPERG being active and set up the Prospector Perk to check
    int spergModIndex = Game.GetModByName("SPERG-SSE.esp")
    if spergModIndex != 255
        int perkID = 0x5cc21
        spergProspector = Game.GetFormFromFile(perkID, "SPERG-SSE.esp") as Perk
        if !spergProspector || spergProspector.GetName() != "Prospector"
            AlwaysTrace("SPERG Prospector Perk resolve failed for " + PrintFormID(perkID))
            spergProspector = None
        endIf
    else
        spergProspector = None
    endIf

    RegisterForCrosshairRef()
EndFunction

; one-time migration logic for BlackList and WhiteList
Function CreateArraysFromFormLists()
    if !whiteListedForms
        whiteListSize = whitelist_form.GetSize()
        whiteListedForms = CreateArrayFromFormList(whitelist_form, whiteListSize)
    endIf
    if !blackListedForms
        blackListSize = blacklist_form.GetSize()
        blackListedForms = CreateArrayFromFormList(blacklist_form, blackListSize)
    endIf
EndFunction

Form[] Function CreateArrayFromFormList(FormList oldList, int oldSize)
    AlwaysTrace("Migrate FormList size(" + oldSize + ") to Form[]")
    Form[] newList = Utility.CreateFormArray(128)
    if oldSize > 0
        ; assume max size initially, resize if bad entries are found
        int validSize = 0
        int index = 0
        while index < oldSize
            newList[index] = oldList.GetAt(index)
            index += 1
        endWhile
    endIf
    return newList
EndFunction

Function CreateTransferListArrays()
    transferList = Utility.CreateFormArray(64, None)
    transferListInUse = Utility.CreateBoolArray(64, false)
    transferNames = Utility.CreateStringArray(64, "")
    transferListSize = 0
EndFunction

Function MigrateTransferListArrays(int oldLimit, int newLimit)
    if !transferList || transferList.Length != oldLimit || newLimit < oldLimit
        return
    endIf
    ; in-place Utility.ResizexxxArray does not work
    Form[] newList = Utility.CreateFormArray(newLimit)
    bool[] newListInUse = Utility.CreateBoolArray(newLimit)
    string[] newNames = Utility.CreateStringArray(newLimit)
    int index = 0
    while index < oldLimit
        newList[index] = transferList[index]
        newListInUse[index] = transferListInUse[index]
        newNames[index] = transferNames[index]
        ;DebugTrace("Source index " + index + "=" + transferList[index] + "/" + transferListInUse[index] + "/" + transferNames[index])
        ;DebugTrace("Temp   index " + index + "=" + newList[index] + "/" + newListInUse[index] + "/" + newNames[index])
        index += 1
    endWhile
    while index < newLimit
        newList[index] = None
        newListInUse[index] = False
        newNames[index] = ""
        ;DebugTrace("Temp   index " + index + "=" + newList[index] + "/" + newListInUse[index] + "/" + newNames[index])
        index += 1
    endWhile
    transferList = Utility.CreateFormArray(newLimit)
    transferListInUse = Utility.CreateBoolArray(newLimit)
    transferNames = Utility.CreateStringArray(newLimit)
    index = 0
    while index < newLimit
        ;DebugTrace("Initial index " + index + "=" + newList[index] + "/" + newListInUse[index] + "/" + newNames[index])
        transferList[index] = newList[index]
        transferListInUse[index] = False
        transferNames[index] = newNames[index]
        ;DebugTrace("Interim index " + index + "=" + newList[index] + "/" + newListInUse[index] + "/" + newNames[index])
        ;DebugTrace("Final   index " + index + "=" + transferList[index] + "/" + transferListInUse[index] + "/" + transferNames[index])
        index += 1
    endWhile
    AlwaysTrace("Migrated " + transferListSize + " Transfer List entries, limit " + oldLimit + " to new limit " + newLimit)
EndFunction

Function ResetExcessInventoryTargets(bool updated)
    int index = 0
    while index < 64
        ; reset transfer target - the old setting could be corrupt
        transferListInUse[index] = False
	index += 1
    endWhile
    if updated
        Debug.MessageBox(GetTranslation("$SHSE_MIGRATED_EXCESS_INVENTORY"))
    endIf
EndFunction

int Function GetWhiteListSize()
    return whiteListSize
EndFunction

int Function GetBlackListSize()
    return blackListSize
EndFunction

int Function GetTransferListSize()
    return transferListSize
EndFunction

Form[] Function GetWhiteList()
    return whiteListedForms
EndFunction

Form[] Function GetBlackList()
    return blackListedForms
EndFunction

Form[] Function GetTransferList()
    return transferList
EndFunction

string[] Function GetTransferNames()
    return transferNames
EndFunction

; merge FormList with plugin data
Function SyncList(int listNum, Form[] forms, int formCount)
    ; plugin resets to fixed baseline
    ResetList(listNum)
    ; ensure BlackList/WhiteList members are present in the plugin's list
    int index = 0
    while index < formCount
        Form nextEntry = forms[index]
        ; do not push empty entries to C++ for blacklist or whitelist.
        if nextEntry && (StringUtil.GetLength(GetNameForListForm(nextEntry)) > 0)
            AddEntryToList(listNum, nextEntry)
        endif
        index += 1
    endwhile
endFunction

; merge FormList with plugin data
Function SyncTransferList(Form[] forms, string[] names, int formCount)
    ; plugin resets to empty baseline
    ResetList(list_type_transfer)
    ; ensure BlackList/WhiteList members are present in the plugin's list
    int index = 0
    while index < formCount
        ; Transfer List is sparse. Include empty entries to keep plugin in sync.
        AddEntryToTransferList(forms[index], names[index])
        index += 1
    endwhile
endFunction

int Function UpdateListedForms(int totalEntries, Form[] myList, form[] updateList, bool[] flags, string trans)
    ; replace existing entries with valid Forms from MCM
    int index = 0
    int valid = 0
    while index < totalEntries
        if flags[index]
            myList[valid] = updateList[index]
            valid += 1
        else
            string translation = GetTranslation(trans)
            if (translation)
                translation = Replace(translation, "{ITEMNAME}", GetNameForListForm(updateList[index]))
                if (translation)
                    Debug.Notification(translation)
                endif
            endif
        endIf
        index += 1
    endWhile
    ;clear any removed entries
    index = valid
    while index < totalEntries
    	myList[index] = None
    	index += 1
    endWhile
    if valid != totalEntries
        AlwaysTrace("Updated Form[] size from (" + totalEntries + ") to (" + valid + ")")
    endIf
    return valid
endFunction

Function UpdateTransferListForms(int activeEntries, form[] updateList, bool[] updateInUse, int[] indices, string[] updateNames, bool[] flags, string trans)
    ; replace existing entries with valid Forms from MCM
    int index = 0
    int xrefIndex = 0   ; index of the passed container in the sparse list
    while index < activeEntries
        ; iterate blank entries
        while xrefIndex < indices[index]
            transferListInUse[xrefIndex] = False
            transferList[xrefIndex] = None
            transferNames[xrefIndex] = ""
            ;DebugTrace("Skip blank transfer list xref-index " + xrefIndex + " index " + index)
            xrefIndex += 1
        endWhile

        if flags[index]
            transferListInUse[xRefIndex] = updateInUse[index]
            transferList[xRefIndex] = updateList[index]
            transferNames[xRefIndex] = updateNames[index]
            ;DebugTrace("In-use transfer list xref-index " + xrefIndex + " index " + index + " " + transferNames[xRefIndex])
        else
            transferListInUse[xrefIndex] = False
            transferList[xrefIndex] = None
            transferNames[xrefIndex] = ""
            transferListSize -= 1
            ;DebugTrace("Unused transfer list xref-index " + xrefIndex + " index " + index)
	    
            string translation = GetTranslation(trans)
            if (translation)
                translation = Replace(translation, "{ITEMNAME}", updateNames[index])
                if (translation)
                    Debug.Notification(translation)
                endif
            endif
        endIf
        xrefIndex += 1
        index += 1
    endWhile
endFunction

Function UpdateWhiteList(int totalEntries, Form[] updateList, bool[] flags, string trans)
    whiteListSize = UpdateListedForms(totalEntries, whiteListedForms, updateList, flags, trans)
EndFunction

Function UpdateBlackList(int totalEntries, Form[] updateList, bool[] flags, string trans)
    blackListSize = UpdateListedForms(totalEntries, blackListedForms, updateList, flags, trans)
EndFunction

Function UpdateTransferList(int activeEntries, bool[] updateInUse, Form[] updateList, int[] indices, string[] names, bool[] flags, string trans)
    UpdateTransferListForms(activeEntries, updateList, updateInUse, indices, names, flags, trans)
    SyncTransferList(transferList, transferNames, 64)
EndFunction

;push updated lists to plugin
Function SyncLists(bool reload, bool updateLists)
    if updateLists
        ; force plugin refresh of player's current worn and equipped items
        ResetList(list_type_in_use_items)
        SyncTransferList(transferList, transferNames, 64)
        SyncList(location_type_whitelist, whiteListedForms, whiteListSize)
        SyncList(location_type_blacklist, blackListedForms, blackListSize)
    endIf
    if reload
        ; reset UI State checking nonce in case saved game left us with a bum value
        pluginNonce = 0
        SetMCMState(false)
        mcmOpen = False
    endIf
    SyncDone(reload)
endFunction

int Function RemoveFormAtIndex(Form[] forms, int entries, int index)
    if index < entries
        AlwaysTrace("Removing " + forms[index] + ", entry " + (index+1) + " of " + entries)
        ; shuffle down entries above this one
        while index < entries - 1
            forms[index] = forms[index+1]
            index += 1
        endWhile
        ; clear prior final entry
        forms[entries - 1] = None
        return entries - 1
    endIf
    AlwaysTrace(index + " not valid for Form[]")
    return entries
endFunction

int Function ClearTransferListEntry(int listMax, int index)
    if index < listMax
        AlwaysTrace("Removing " + transferList[index] + ", entry " + index + " of " + listMax)
        transferList[index] = None
        transferNames[index] = ""
        transferListInUse[index] = False
        transferListSize -= 1
    else
        AlwaysTrace(index + " not a valid index for TransferList")
    endIf
endFunction

; manages FormList in VM - SyncLists pushes state to plugin once all local operations are complete
function ToggleStatusInBlackList(Form item)
    if !item
        return
    endif

    if !RemoveFromBlackList(item)
        AddToBlackList(item)
    endif
endFunction

function ToggleStatusInWhiteList(Form item)
    if !item
        return
    endif
    if !RemoveFromWhiteList(item)
        AddToWhiteList(item)
    endif
endFunction

; manages FormList in VM - SyncLists pushes state to plugin once all local operations are complete
function ToggleStatusInTransferList(string locationName, Form item)
    if !item
        return
    endif

    if RemoveFromTransferList(locationName, item) == 0
        AddToTransferList(locationName, item)
    endif
endFunction

function HandlePauseKeyPress(Form target)
    NameToDisplay.SetName(target.GetName())
    int ibutton = DisposeMsg.show()     ; allows user to dispose of all/excess selected items
    string result = ""
    if ibutton == 0 ; Sell All
        result = SellItem(target, False)
    elseif ibutton == 1 ; sell excess
        result = SellItem(target, True)
    elseif ibutton == 2 ; transfer all
        result = TransferItem(target, False)
    elseif ibutton == 3 ; transfer excess
        result = TransferItem(target, True)
    elseif ibutton == 4 ; delete all
        result = DeleteItem(target, False)
    elseif ibutton == 5 ; delete excess
        result = DeleteItem(target, True)
    elseif ibutton == 6 ; delete excess
        result = CheckItemAsExcess(target)
    endIf
    ; Cancel (7) or unknown option is a no-op
    if result != ""
        if ibutton < 6
            ; Poke InventoryMenu to update count        
            Player.RemoveItem(target, 0, True, None)
        endIf
        ; display diagnostic, error or action taken
        Debug.MessageBox(result)
    endIf
endFunction

function HandleWhiteListKeyPress(Form target)
    ; first remove from whitelist if present
    RemoveFromBlackList(target)
    ToggleStatusInWhiteList(target)
endFunction

bool function RemoveFromWhiteList(Form target)
    int match = whiteListedForms.find(target)
    if match != -1
        string translation = GetTranslation("$SHSE_WHITELIST_REMOVED")
        if (translation)
            string msg = Replace(translation, "{ITEMNAME}", GetNameForListForm(target))
            if (msg)
                Debug.Notification(msg)
            endif
        endif
        whiteListSize = RemoveFormAtIndex(whiteListedForms, whiteListSize, match)
        AlwaysTrace(whiteListSize + " entries on WhiteList")
        return True
    endIf
    AlwaysTrace(target + " not found in WhiteList")
    return False
endFunction

function AddToWhiteList(Form target)
    ; do not add if empty or no name
    if !target || StringUtil.GetLength(GetNameForListForm(target)) == 0
        return
    endIf
    if whiteListSize == 128
        string translation = GetTranslation("$SHSE_WHITELIST_FULL")
        if (translation)
            string msg = Replace(translation, "{ITEMNAME}", GetNameForListForm(target))
            if (msg)
                Debug.Notification(msg)
            endif
        endif
        return
    endIf
    if whiteListedForms.find(target) == -1
        string translation = GetTranslation("$SHSE_WHITELIST_ADDED")
        if (translation)
            string msg = Replace(translation, "{ITEMNAME}", GetNameForListForm(target))
            if (msg)
                Debug.Notification(msg)
            endif
        endif
        whiteListedForms[whiteListSize] = target
        whiteListSize += 1
        AlwaysTrace(target + " added to WhiteList, size now " + whiteListSize)
    else
        AlwaysTrace(target + " already on WhiteList")
    endif
endFunction

function HandleBlackListKeyPress(Form target)
    ; first remove from whitelist if present
    RemoveFromWhiteList(target)
    ToggleStatusInBlackList(target)
endFunction

bool function RemoveFromBlackList(Form target)
    int match = blackListedForms.find(target)
    if match != -1
        string translation = GetTranslation("$SHSE_BLACKLIST_REMOVED")
        if (translation)
            string msg = Replace(translation, "{ITEMNAME}", GetNameForListForm(target))
            if (msg)
                Debug.Notification(msg)
            endif
        endif
        blackListSize = RemoveFormAtIndex(blackListedForms, blackListSize, match)
        AlwaysTrace(blackListSize + " entries on BlackList")
        return True
    endIf
    AlwaysTrace(target + " not found in BlackList")
    return False
endFunction

function AddToBlackList(Form target)
    ; do not add if empty or no name
    if !target || StringUtil.GetLength(GetNameForListForm(target)) == 0
        return
    endIf
    if blackListSize == 128
        string translation = GetTranslation("$SHSE_BLACKLIST_FULL")
        if (translation)
            string msg = Replace(translation, "{ITEMNAME}", GetNameForListForm(target))
            if (msg)
                Debug.Notification(msg)
            endif
        endif
        return
    endIf
    if blackListedForms.find(target) == -1
        string translation = GetTranslation("$SHSE_BLACKLIST_ADDED")
        if (translation)
            string msg = Replace(translation, "{ITEMNAME}", GetNameForListForm(target))
            if (msg)
                Debug.Notification(msg)
            endif
        endif
        blackListedForms[blackListSize] = target
        blackListSize += 1
        AlwaysTrace(target + " added to BlackList, size now " + blackListSize)
    else
        AlwaysTrace(target + " already on BlackList")
    endif
endFunction

int function RemoveFromTransferList(string locationName, Form target)
    int match = transferList.find(target)
    if match != -1
        if transferListInUse[match]
            string translation = GetTranslation("$SHSE_TRANSFERLIST_CANNOT_REMOVE_IN_USE")
            if (translation)
                string msg = Replace(translation, "{ITEMNAME}", locationName + "/" + GetNameForListForm(target))
                if (msg)
                    Debug.Notification(msg)
                endif
            endif
            return -1
        endif
        string translation = GetTranslation("$SHSE_TRANSFERLIST_REMOVED")
        if (translation)
            string msg = Replace(translation, "{ITEMNAME}", locationName + "/" + GetNameForListForm(target))
            if (msg)
                Debug.Notification(msg)
            endif
        endif
        ClearTransferListEntry(64, match)
        AlwaysTrace(transferListSize + " entries on TransferList")
        return 1
    endIf
    AlwaysTrace(target + " not found in TransferList")
    return 0
endFunction

function AddToTransferList(string locationName, Form target)
    ; do not add if empty or no name
    string containerName = GetNameForListForm(target)
    if !target || StringUtil.GetLength(containerName) == 0
        return
    endIf
    string name = locationName + "/" + containerName
    if transferListSize == 64
        string translation = GetTranslation("$SHSE_TRANSFERLIST_FULL")
        if (translation)
            string msg = Replace(translation, "{ITEMNAME}", name)
            if (msg)
                Debug.Notification(msg)
            endif
        endif
        return
    endIf
    if transferList.find(target) == -1
        string translation = GetTranslation("$SHSE_TRANSFERLIST_ADDED")
        if (translation)
            string msg = Replace(translation, "{ITEMNAME}", name)
            if (msg)
                Debug.Notification(msg)
            endif
        endif
        ; find a free entry
        int index = 0
        while index < 64
            if transferList[index] == None
                transferList[index] = target
                transferListInUse[index] = False
                transferNames[index] = name
                transferListSize += 1
                AlwaysTrace(target + " added to TransferList at index " + index + ", size now " + transferListSize)
                return
            endif
            index += 1
        endWhile
    else
        AlwaysTrace(target + " already on TransferList")
    endif
endFunction

Function SetDefaultShaders()
    ; must line up with native GlowReason enum
    glowReasonLockedContainer = 0
    glowReasonBossContainer = 1
    glowReasonQuestObject = 2
    glowReasonCollectible = 3
    glowReasonHighValue = 4
    glowReasonEnchantedItem = 5
    glowReasonPlayerProperty = 6
    glowReasonSimpleTarget = 7

    ; must line up with shaders defined in ESP/ESM file
    redShader = Game.GetFormFromFile(0x80e, "SmartHarvestSE.esp") as EffectShader        ; red
    flamesShader = Game.GetFormFromFile(0x810, "SmartHarvestSE.esp") as EffectShader          ; flames
    purpleShader = Game.GetFormFromFile(0x80d, "SmartHarvestSE.esp") as EffectShader         ; purple
    copperShader = Game.GetFormFromFile(0x814, "SmartHarvestSE.esp") as EffectShader   ; copper
    goldShader = Game.GetFormFromFile(0x815, "SmartHarvestSE.esp") as EffectShader          ; gold
    blueShader = Game.GetFormFromFile(0x80b, "SmartHarvestSE.esp") as EffectShader     ; blue
    greenShader = Game.GetFormFromFile(0x80c, "SmartHarvestSE.esp") as EffectShader         ; green
    silverShader = Game.GetFormFromFile(0x813, "SmartHarvestSE.esp") as EffectShader        ; silver

    ; category default and current glow colour
    defaultCategoryShaders = new EffectShader[8]
    defaultCategoryShaders[glowReasonLockedContainer] = redShader
    defaultCategoryShaders[glowReasonBossContainer] = flamesShader
    defaultCategoryShaders[glowReasonQuestObject] = purpleShader
    defaultCategoryShaders[glowReasonCollectible] = copperShader
    defaultCategoryShaders[glowReasonHighValue] = goldShader
    defaultCategoryShaders[glowReasonEnchantedItem] = blueShader
    defaultCategoryShaders[glowReasonPlayerProperty] = greenShader
    defaultCategoryShaders[glowReasonSimpleTarget] = silverShader

    categoryShaders = new EffectShader[8]
    int index = 0
    while index < categoryShaders.length
        categoryShaders[index] = defaultCategoryShaders[index]
        index = index + 1
    endWhile
EndFunction

Function SyncShaders(Int[] colours)
    int index = 0
    while index < colours.length
        categoryShaders[index] = defaultCategoryShaders[colours[index]]
        index = index + 1
    endWhile
EndFunction

Function SyncVeinResourceTypes()
    resource_Ore = GetResourceTypeByName("Ore")
    resource_Geode = GetResourceTypeByName("Geode")
    resource_Volcanic = GetResourceTypeByName("Volcanic")
    resource_VolcanicDigSite = GetResourceTypeByName("VolcanicDigSite")
EndFunction

; must line up with enumerations from C++
Function SyncUpdatedNativeDataTypes()
    objType_Flora = GetObjectTypeByName("flora")
    objType_Critter = GetObjectTypeByName("critter")
    objType_Septim = GetObjectTypeByName("septims")
    objType_Soulgem = GetObjectTypeByName("soulgem")
    objType_Mine = GetObjectTypeByName("oreVein")
    objType_Book = GetObjectTypeByName("book")
    objType_skillBookRead = GetObjectTypeByName("skillbookread")

    SyncVeinResourceTypes()

    location_type_whitelist = 1
    location_type_blacklist = 2
    list_type_transfer = 3
    list_type_in_use_items = 4

    infiniteWeight = 100000
endFunction

Function ResetCollections()
    ;prepare for collection management, if configured
    collectionsInUse = CollectionsInUse()
    if !collectionsInUse
        return
    endIf
    ;DebugTrace("eventScript.ResetCollections")
    addedItems = Utility.CreateFormArray(128)
    maxAddedItems = 128
    currentAddedItem = 0
    addedItemScope = Utility.CreateIntArray(128)
    addedItemType = Utility.CreateIntArray(128)
EndFunction

Function ApplySetting()
    ;DebugTrace("eventScript ApplySetting start")
    UnregisterForAllKeys()
    UnregisterForMenu("Loading Menu")

    if pauseKeyCode != 0
         RegisterForKey(pauseKeyCode)
    endif
    if whiteListKeyCode != 0
        RegisterForKey(whiteListKeyCode)
    endif
    if blackListKeyCode != 0
        RegisterForKey(blackListKeyCode)
    endif

    utility.waitMenumode(0.1)
    RegisterForMenu("Loading Menu")
    ;DebugTrace("eventScript ApplySetting finished")
endFunction

string Function sif (bool cc, string aa, string bb) global
    string result
    if (cc)
        result = aa
    else
        result = bb
    endif
    return result
endFunction

Function PushScanActive()
    SyncScanActive(scanActive)
    if scanActive
        Debug.Notification(Replace(GetTranslation("$SHSE_UNPAUSED"), "{VERSION}", GetPluginVersion()))
    else
        Debug.Notification(Replace(GetTranslation("$SHSE_PAUSED"), "{VERSION}", GetPluginVersion()))
    endif
EndFunction

Function SyncWhiteListKey(int keyCode)
    whiteListKeyCode = keyCode
EndFunction

Function SyncBlackListKey(int keyCode)
    blackListKeyCode = keyCode
EndFunction

Function SyncPauseKey(int keyCode)
    pauseKeyCode = keyCode
EndFunction

; hotkey changes sense of looting. Use of MCM/new character/game reload resets this to whatever's
; implied by current settings
function Pause()
    string s_enableStr = none
    scanActive = !scanActive
    PushScanActive()
endFunction

; BlackList/WhiteList hotkeys
Function HandleCrosshairItemHotKey(ObjectReference targetedRefr, bool isWhiteKey, Float holdTime)
    ; check for long press
    if holdTime > 1.5
        if isWhiteKey
            if targetedRefr.GetBaseObject() as Container
                ProcessContainerCollectibles(targetedRefr)
            elseif targetedRefr.GetBaseObject() as Book
                ; special-case to force-harvest Quest Targets
                TryForceHarvest(targetedRefr)
            else
                Debug.Notification("$SHSE_HOTKEY_NOT_A_CONTAINER_OR_FORCE_HARVEST")
            endif
        else ; BlackList Key
            ; object lootability introspection
            CheckLootable(targetedRefr)
        endIf
    else
        ; regular press. Does nothing unless this is a non-generated Dead Body or Container, or a Lootable Object
        bool valid = False
        Actor refrActor = targetedRefr as Actor
        Container refrContainer = targetedRefr.GetBaseObject() as Container
        if refrActor || refrContainer
            valid = !IsDynamic(targetedRefr) && ((refrActor && refrActor.IsDead()) || refrContainer)
            if valid
                ; blacklist or un-blacklist the REFR, not the Base, to avoid blocking other REFRs with same Base
                if isWhiteKey
                    RemoveFromBlackList(targetedRefr)
                else ; BlackList Key
                    AddToBlackList(targetedRefr)
                EndIf
            endif
        elseif IsLootableObject(targetedRefr)
            ; open world blacklist or whitelist for item
            valid = True
            if isWhiteKey
                HandleWhiteListKeyPress(targetedRefr.GetBaseObject())
            else ; blacklist key
                HandleBlackListKeyPress(targetedRefr.GetBaseObject())
            endif
        endif
        if valid
            SyncLists(false, true)    ; not a reload
        else
            Debug.Notification("$SHSE_HOTKEY_NOT_VALID_FOR_LISTS")
        endIf
    endIf
EndFunction

; Pause hotkey on crosshair item indicates user is trying to set or unset a Loot Transfer Target
Function HandleCrosshairPauseHotKey(ObjectReference targetedRefr)
    ; Does nothing unless this is a Container to which we can safely send loot
    ; check for REFR that encapsulates a valid linked Container
    bool linkedChest = False
    bool knownGood = False
    ObjectReference refrToStore = targetedRefr
    if targetedRefr as ActivateLinkedChestDummyScript != None
        linkedChest = True
    elseif (MagicChestModIndex != 255) && (targetedRefr as _Skyrim_SE_Nexus_Script != None)
        knownGood = True
        linkedChest = True
    elseIf (VidanisBagOfHoldingModIndex != 255) && (targetedRefr as aaaMRbohArmScript != None)
        knownGood = True
        refrToStore = (targetedRefr as aaaMRbohArmScript).aaaMRbohContREF
    endIf
    ;DebugTrace("REFR " + refrToStore + ", linked container ? " + linkedChest + ", known good ? " + knownGood)
    string locationName = ValidTransferTargetLocation(refrToStore, linkedChest, knownGood)
    if locationName != ""
        ; add or remove the REFR, not the Base, to avoid blocking other REFRs with same Base
        ToggleStatusInTransferList(locationName, refrToStore)
        SyncTransferList(transferList, transferNames, 64)
    else
        Debug.Notification("$SHSE_HOTKEY_NOT_VALID_FOR_TRANSFERLIST")
    endIf
EndFunction

Event OnKeyUp(Int keyCode, Float holdTime)
    if (UI.IsTextInputEnabled())
        return
    endif
    ; only handle one at a time, if player spams the keyboard results will be confusing
    if keyHandlingActive
        return
    endif
    keyHandlingActive = true
    if (!Utility.IsInMenumode())
        ; handle hotkey actions for crosshair in reference
        ObjectReference targetedRefr = Game.GetCurrentCrosshairRef()
        if keyCode == pauseKeyCode
            if holdTime > 1.5
                ; add Container to TargetList requires long press
                if targetedRefr
                    HandleCrosshairPauseHotKey(targetedRefr)
                    keyHandlingActive = false
                    return
                endIf
                ; trigger shader test on really long press
                ToggleCalibration(holdTime > 5.0)
            else
                Pause()
            endif
        elseif keyCode == whiteListKeyCode || keyCode == blackListKeyCode
            if targetedRefr
                HandleCrosshairItemHotKey(targetedRefr, keyCode == whiteListKeyCode, holdTime)
                keyHandlingActive = false
                return
            endIf

            ; Location/cell blacklist whitelist toggle in worldspace
            Form place = GetPlayerPlace()
            if (!place)
                if keyCode == whiteListKeyCode
                    Debug.Notification(GetTranslation("$SHSE_WHITELIST_FORM_ERROR"))
                else
                    Debug.Notification(GetTranslation("$SHSE_BLACKLIST_FORM_ERROR"))
                endIf
                keyHandlingActive = false
                return
            endif
            if keyCode == whiteListKeyCode
                HandleWhiteListKeyPress(place)
            else ; blacklist key
                HandleBlackListKeyPress(place)
            endif
            SyncLists(false, true)    ; not a reload
        endif
    ; menu open - only actionable on our blacklist/whitelist keys
    elseif keyCode == whiteListKeyCode || keyCode == blackListKeyCode || keyCode == pauseKeyCode
        string s_menuName = none
        if (UI.IsMenuOpen("InventoryMenu"))
            s_menuName = "InventoryMenu"
        elseif (UI.IsMenuOpen("ContainerMenu"))
            s_menuName = "ContainerMenu"
        endif
        
        if (s_menuName == "ContainerMenu" || s_menuName == "InventoryMenu")

            Form itemForm = GetSelectedItemForm(s_menuName)
            if !itemForm
                string msg
                if keyCode == whiteListKeyCode
                    msg = "$SHSE_WHITELIST_FORM_ERROR"
                else
                    msg = "$SHSE_BLACKLIST_FORM_ERROR"
                endIf
                Debug.Notification(msg)
                keyHandlingActive = false
                return
            endif

            if keyCode == pauseKeyCode
                HandlePauseKeyPress(itemForm)
            else
                if IsQuestTarget(itemForm)
                    string msg
                    if keyCode == whiteListKeyCode
                        msg = "$SHSE_WHITELIST_QUEST_TARGET"
                    else
                        msg = "$SHSE_BLACKLIST_QUEST_TARGET"
                    endIf
                    Debug.Notification(msg)
                    keyHandlingActive = false
                    return
                endif
                if keyCode == whiteListKeyCode
                    HandleWhiteListKeyPress(itemForm)
                else ; blacklist key
                    HandleBlackListKeyPress(itemForm)
                endif
                SyncLists(false, true)    ; not a reload
            endIf
        endif
    endif
    keyHandlingActive = false
endEvent

function updateMaxMiningItems(int maxItems)
    ;DebugTrace("maxMiningItems -> " + maxItems)
    maxMiningItems = maxItems
endFunction

bool Function IsBookObject(int type)
    return type >= objType_Book && type <= objType_skillBookRead
endFunction

Function RecordItem(Form akBaseItem, int scope, int objectType)
    ;register item received in the 'collection pending' list
    ;DebugTrace("RecordItem " + akBaseItem)
    if !collectionsInUse
        return
    endIf
    if currentAddedItem == maxAddedItems
        ; list is full, flush to the plugin
        FlushAddedItems(Utility.GetCurrentGameTime(), addedItems, addedItemScope, addedItemType, currentAddedItem)
        currentAddedItem = 0
    endif
    addedItems[currentAddedItem] = akBaseItem
    addedItemScope[currentAddedItem] = scope
    addedItemType[currentAddedItem] = objectType
    currentAddedItem += 1
EndFunction

bool Function ActivateEx(ObjectReference akTarget, ObjectReference akActivator, bool suppressMessage, int activateCount)
    bool bShowHUD = Utility.GetINIBool("bShowHUDMessages:Interface")
    if (bShowHUD && suppressMessage)
        Utility.SetINIBool("bShowHUDMessages:Interface", false)
    endif
    int activated = 0
    bool result = True
    while result && activated < activateCount
        result = akTarget.Activate(akActivator)
        activated += 1
    endWhile
    if (bShowHUD && suppressMessage)
        Utility.SetINIBool("bShowHUDMessages:Interface", true)
    endif
    return result
endFunction

Event OnMining(ObjectReference akMineable, int resourceType, bool manualLootNotify, bool isFirehose)
    if logEvent
        DebugTrace("OnMining: " + akMineable.GetDisplayName() + "RefID(" +  akMineable.GetFormID() + ")  BaseID(" + akMineable.GetBaseObject().GetFormID() + ")" ) 
        DebugTrace("resource type: " + resourceType + ", notify for manual loot: " + manualLootNotify)
    endIf
    int miningStrikes = 0
    int targetResourceTotal = 0
    int strikesToCollect = 0
    MineOreScript oreScript = akMineable as MineOreScript
    int FOSStrikesBeforeFossil
    bool handled = false
    if (oreScript)
        if logEvent
            DebugTrace("Detected ore vein")
        endIf            
        ; brute force ore gathering to bypass tedious MineOreScript/Furniture handshaking
        targetResourceTotal = oreScript.ResourceCountTotal
        strikesToCollect = oreScript.StrikesBeforeCollection
        int available = oreScript.ResourceCountCurrent
        int mined = 0
        bool useSperg = spergProspector && player.HasPerk(spergProspector)
        if useSperg
            PrepareSPERGMining()
        endif
        if logEvent
            if (available == -1)
                DebugTrace("Vein not yet initialized, start mining")
            else
                DebugTrace("Vein has ore available: " + available)
            endIf            
        endif

        ; 'available' is set to -1 before the vein is initialized - after we call giveOre the amount received is
        ; in ResourceCount and the remaining amount in ResourceCountCurrent 
        while OKToScan() && available != 0 && mined < maxMiningItems
            if logEvent
                DebugTrace("Trigger harvesting")
            endIf            
            oreScript.giveOre()
            mined += oreScript.ResourceCount
            if logEvent
                DebugTrace("Ore amount so far: " + mined + ", this time: " + oreScript.ResourceCount + ", max: " + maxMiningItems)
            endIf            
            available = oreScript.ResourceCountCurrent
            miningStrikes += 1
        endwhile
        if !OKToScan()
            AlwaysTrace("UI open : oreScript mining interrupted, " + mined + " obtained")
        endIf
        if logEvent
            DebugTrace("Ore harvested amount: " + mined + ", remaining: " + oreScript.ResourceCountCurrent)
        endIf
        if useSperg
            PostprocessSPERGMining()
        endif
        FOSStrikesBeforeFossil = 6
        handled = true
    endif
    ; CACO provides its own mining script, unfortunately not derived from baseline though largely identical
    if !handled && (CACOModIndex != 255)
        CACO_MineOreScript cacoMinable = akMineable as CACO_MineOreScript
        if (cacoMinable)
            if logEvent
                DebugTrace("Detected CACO ore vein")
            endIf
            ; brute force ore gathering to bypass tedious MineOreScript/Furniture handshaking
            int available = cacoMinable.ResourceCountCurrent
            targetResourceTotal = cacoMinable.ResourceCountTotal
            strikesToCollect = cacoMinable.StrikesBeforeCollection
            int mined = 0
            ; do not harvest firehose unless set in config
            if logEvent
                if (available == -1)
                    DebugTrace("CACO ore vein not yet initialized, start mining")
                else
                    DebugTrace("CACO ore vein has ore available: " + available)
                endIf
            endif

            ; 'available' is set to -1 before the vein is initialized - after we call giveOre the amount received is
            ; in ResourceCount and the remaining amount in ResourceCountCurrent 
            while OKToScan() && available != 0 && mined < maxMiningItems
                if logEvent
                    DebugTrace("Trigger CACO ore harvesting")
                endIf            
                cacoMinable.giveOre()
                mined += cacoMinable.ResourceCount
                if logEvent
                    DebugTrace("CACO ore vein amount so far: " + mined + ", this time: " + cacoMinable.ResourceCount + ", max: " + maxMiningItems)
                endIf            
                available = cacoMinable.ResourceCountCurrent
                miningStrikes += 1
            endwhile
            if !OKToScan()
                AlwaysTrace("UI open : CACO_MineOreScript mining interrupted, " + mined + " obtained")
            endIf
            if logEvent
                DebugTrace("CACO ore vein harvested amount: " + mined + ", remaining: " + oreScript.ResourceCountCurrent)
            endIf            
            handled = true
        endif
    endif
    if !handled && (FossilMiningModIndex != 255)
        if logEvent
            DebugTrace("Check for Fossil Mining Dig Site")
        endIf            
        FOS_DigsiteScript FOSMinable = akMineable as FOS_DigsiteScript
        if (FOSMinable)
            if logEvent
                DebugTrace("Process Fossil Mining Dig Site")
            endIf            
            ; brute force fossil gathering to bypass tedious Script/Furniture handshaking
            ; REFR will be blocked ater this call, until we leave the cell
            ; FOS script enables the FURN when we first enter the cell, provided mining is legal
            ; If we re-enter the cell we will check again but not be able to mine
            if !FOSMinable.GetLinkedRef().IsDisabled()
                player.AddItem(FOS_LItemFossilTierOneVolcanicDigSite, 1)
                player.AddItem(FOS_LItemFossilTierTwoVolcanic, 1)
                Debug.Notification("Dig site is exhausted")
                FOSMinable.GetLinkedRef().Disable() 
            else
                Debug.Notification("Dig site is exhausted, check back at a later time.")    
            endif
            handled = true
        endif
    endif

    if isFirehose
        ; no-op

    elseif (miningStrikes > 0 && FossilMiningModIndex != 255 && resourceType != resource_VolcanicDigSite)
        ; Fossil Mining Drop Logic from oreVein per Fos_AttackMineAlias.psc, bypassing the FURN.
        ; Excludes Hearthfire house materials (by construction) to mimic FOS_IgnoreList filtering.
        ; Excludes Fossil Mining Dig Sites, processed in full above
        ;randomize drop of fossil based on number of strikes and vein characteristics
        FOSStrikesBeforeFossil = strikesToCollect * targetResourceTotal
        int dropFactor = Utility.RandomInt(1, FOSStrikesBeforeFossil)
        if logEvent
            DebugTrace("Fossil Mining: strikes = " + miningStrikes + ", required for drop = " + FOSStrikesBeforeFossil)
        endIf            
        if (dropFactor <= miningStrikes)
            if logEvent
                DebugTrace("Fossil Mining: provide loot!")
            endIf            
            if (resourceType == resource_Geode)
                player.AddItem(FOS_LItemFossilTierOneGeode, 1)
            Elseif (resourceType == resource_Volcanic)
                player.AddItem(FOS_LItemFossilTierOneVolcanic, 1)
            Elseif (resourceType == resource_Ore)
                player.AddItem(FOS_LItemFossilTierOneyum, 1)
            Endif
        Endif
    Endif

    if !handled && manualLootNotify
        ; unrecognized 'Mine' verb target - print message for 'nearby manual lootable' if configured to do so
        NotifyManualLootItem(akMineable)
    endif

EndEvent

int Function SupportedCritterActivateCount(ObjectReference target)
    if target as Critter || target as FXFakeCritterScript || target as WispCoreScript
        return 1
    endIf
    if HearthfireExtendedModIndex != 255 && target as KmodApiaryScript
        return 5
    endIf
    ; 'not a critter' sentinel
    return 0
EndFunction


Event OnHarvest(ObjectReference akTarget, Form itemForm, string baseName, int itemType, int count, bool silent, bool collectible, float ingredientCount, bool isWhitelisted)
    bool notify = false
    ; capture values now, dynamic REFRs can become invalid before we need them
    int refrID = akTarget.GetFormID()
    int baseID = akTarget.GetBaseObject().GetFormID()
    Form baseForm = akTarget.GetBaseObject()

    ;DebugTrace("OnHarvest:Run: target " + akTarget + ", base " + itemForm) 
    ;DebugTrace(", item type: " + itemType + ", do not notify: " + silent + ")

    if (IsBookObject(itemType))
        player.AddItem(akTarget, count, true)
        if collectible
            RecordItem(itemForm, itemSourceLoose, itemType)
        endIf

        notify = !silent
    elseif (itemType == objType_Soulgem && akTarget.GetLinkedRef(None))
        ; Harvest trapped SoulGem only after deactivation - no-op otherwise
        TrapSoulGemController myTrap = akTarget as TrapSoulGemController
        if myTrap
            string baseState = akTarget.GetLinkedRef(None).getState()
            if logEvent
                DebugTrace("Trapped soulgem " + akTarget + ", state " + myTrap.getState() + ", linked to " + akTarget.GetLinkedRef(None) + ", state " + baseState) 
            endIf
            if myTrap.getState() == "disarmed" && (baseState == "disarmed" || baseState == "idle") && ActivateEx(akTarget, player, true, 1)
                notify = !silent
            endIf
        endIf
    elseif (!akTarget.IsActivationBlocked())
        if (itemType == objType_Septim && baseForm.GetType() == getType_kFlora)
            ActivateEx(akTarget, player, silent, 1)

        elseif baseForm.GetType() == getType_kFlora || baseForm.GetType() == getType_kTree
            ; "Flora" or "Tree" Producer REFRs cannot be identified by item type
            ;DebugTrace("Player has ingredient count " + ingredientCount)
            bool suppressMessage = silent || ingredientCount as int > 1
            ;DebugTrace("Flora/Tree original base form " + itemForm.GetName())
            if ActivateEx(akTarget, player, suppressMessage, 1)
                ;we must send the message if required default would have been incorrect
                notify = !silent && ingredientCount as int > 1
                count = count * ingredientCount as int
            endif
        ; Critter ACTI REFRs cannot be identified by item type
        else
            int critterActivations = SupportedCritterActivateCount(akTarget)
            if critterActivations > 0
                ;DebugTrace("Critter " + itemForm.GetName())
                ActivateEx(akTarget, player, silent, critterActivations)
            elseif ActivateEx(akTarget, player, true, 1)
                notify = !silent
                if count >= 2
                    ; work round for ObjectReference.Activate() known issue
                    ; https://www.creationkit.com/fallout4/index.php?title=Activate_-_ObjectReference
                    int toGet = count - 1
                    player.AddItem(itemForm, toGet, true)
                    ;DebugTrace("Add extra count " + toGet + " of " + itemForm)
                endIf
            endIf
        endif
        if collectible
            RecordItem(itemForm, itemSourceLoose, itemType)
        endIf
        ;DebugTrace("OnHarvest:Activated:" + akTarget.GetDisplayName() + "RefID(" +  akTarget.GetFormID() + ")  BaseID(" + akTarget.GetBaseObject().GetFormID() + ")" )
    endif
    if (notify)
        string activateMsg = none
        if (count >= 2)
            string translation = GetTranslation("$SHSE_ACTIVATE(COUNT)_MSG")
            
            string[] targets = New String[2]
            targets[0] = "{ITEMNAME}"
            targets[1] = "{COUNT}"

            string[] replacements = New String[2]
            replacements[0] = baseName
            replacements[1] = count as string
            
            activateMsg = ReplaceArray(translation, targets, replacements)
        else
            string translation = GetTranslation("$SHSE_ACTIVATE_MSG")
            activateMsg = Replace(translation, "{ITEMNAME}", baseName)
        endif
        if (activateMsg)
            Debug.Notification(activateMsg)
        endif
    endif
    if isWhitelisted
        string whitelistMsg = Replace(GetTranslation("$SHSE_WHITELIST_ITEM_LOOTED"), "{ITEMNAME}", baseName)
        if whitelistMsg
            Debug.Notification(whitelistMsg)
        endif
    endIf
    
    UnlockHarvest(refrID, baseID, baseName, silent)
endEvent

; NPC looting appears to have thread safety issues requiring script to perform
Event OnLootFromNPC(ObjectReference akContainerRef, Form akForm, int count, int itemType, bool collectible)
    ;DebugTrace("OnLootFromNPC: " + akContainerRef.GetDisplayName() + " " + akForm.GetName() + "(" + count + ")")
    if (!akContainerRef)
        return
    elseif (!akForm)
        return
    endif

    akContainerRef.RemoveItem(akForm, count, true, player)
    if collectible
        RecordItem(akForm, itemSourceNPC, itemType)
    endIf
endEvent

Event OnGetProducerLootable(ObjectReference akTarget)
    ;DebugTrace("OnGetProducerLootable " + akTarget.GetDisplayName() + "RefID(" +  akTarget.GetFormID() + ")  BaseID(" + akTarget.GetBaseObject().GetFormID() + ")" )
    if SupportedCritterActivateCount(akTarget) == 0
        return
    endIf
    Form baseForm = akTarget.GetBaseObject()
    Critter thisCritter = akTarget as Critter
    if thisCritter
        if thisCritter.nonIngredientLootable
            ; Salmon and other fish - FormList, 0-1 elements seen so far - make a log if > 1
            int lootableCount = thisCritter.nonIngredientLootable.GetSize()
            if lootableCount > 1
                AlwaysTrace(akTarget + " with Base " + akTarget.GetBaseObject() + " has " + lootableCount + "nonIngredientLootable entries")
            elseif lootableCount == 1
                SetLootableForProducer(baseForm, thisCritter.nonIngredientLootable.GetAt(0))
            else
                ; blacklist empty vessel
                SetLootableForProducer(baseForm, None)
            endif
        else
            ; everything else - simple ingredient
            SetLootableForProducer(baseForm, thisCritter.lootable)
        endIf
        return
    endIf
    FXfakeCritterScript fakeCritter = akTarget as FXfakeCritterScript
    if fakeCritter
        ; Activation may produce 0-2 items, return the most valuable
        if fakeCritter.myIngredient
            SetLootableForProducer(baseForm, fakeCritter.myIngredient)
        elseif fakeCritter.myFood
            SetLootableForProducer(baseForm, fakeCritter.myFood)
        else
            AlwaysTrace(akTarget + " with Base " + akTarget.GetBaseObject() + " has neither myFood nor myIngredient")
            SetLootableForProducer(baseForm, None)
        endif
        return
    endIf
    WispCoreScript wispCore = akTarget as WispCoreScript
    if wispCore
        SetLootableForProducer(baseForm, wispCore.glowDust)
        return
    endIf
    ; handle modified apiary if present
    if HearthfireExtendedModIndex != 255
        KmodApiaryScript apiary = akTarget as KmodApiaryScript
        if apiary
            ; there are three items: we choose the critter for loot rule checking, but all items should be looted
            SetLootableForProducer(baseForm, apiary.CritterBeeIngredient)
            return
        endIf
    endIf
endEvent

Function DoObjectGlow(ObjectReference akTargetRef, int duration, int reason)
    EffectShader effShader
    if reason >= 0 && reason <= glowReasonSimpleTarget
        effShader = categoryShaders[reason]
    else
        effShader = categoryShaders[glowReasonSimpleTarget]
    endif
    if effShader && OKToScan()
        ; play for requested duration - C++ code will tidy up when out of range
        ;DebugTrace("OnObjectGlow for " + akTargetRef.GetDisplayName() + " for " + duration + " seconds")
        effShader.Play(akTargetRef, duration)
    endif
endFunction

Event OnObjectGlow(ObjectReference akTargetRef, int duration, int reason)
    DoObjectGlow(akTargetRef, duration, reason)
endEvent

; event should only fire if we are managing carry weight
Event OnCarryWeightDelta(int weightDelta)
    player.ModActorValue("CarryWeight", weightDelta as float)
    ;DebugTrace("Player carry weight " + player.GetActorValue("CarryWeight") + " after applying delta " + weightDelta)
EndEvent

Function RemoveCarryWeightDelta()
    int carryWeight = player.GetActorValue("CarryWeight") as int
    ;DebugTrace("Player carry weight initially " + carryWeight)

    int weightDelta = 0
    while (carryWeight > infiniteWeight)
        weightDelta -= infiniteWeight
        carryWeight -= infiniteWeight
    endwhile
    while (carryWeight < 0)
        weightDelta += infiniteWeight
        carryWeight += infiniteWeight
    endwhile

    if (weightDelta != 0)
        player.ModActorValue("CarryWeight", weightDelta as float)
    endif
    ;DebugTrace("Player carry weight adjusted to " + player.GetActorValue("CarryWeight"))
endFunction

; event should only fire if we are managing carry weight
Event OnResetCarryWeight()
    ;DebugTrace("Player carry weight reset request")
    RemoveCarryWeightDelta()
EndEvent

Event OnFlushAddedItems()
    ;DebugTrace("Request to flush added items")
    ; always respond to poll so plugin DLL keeps in sync with game-time
    if currentAddedItem > 0
        FlushAddedItems(Utility.GetCurrentGameTime(), addedItems, addedItemScope, addedItemType, currentAddedItem)
        currentAddedItem = 0
    else
        PushGameTime(Utility.GetCurrentGameTime())
    endIf
EndEvent

Function OnMCMOpen()
    mcmOpen = True
    SetMCMState(True)
EndFunction

Function OnMCMClose()
    SetMCMState(False)
    mcmOpen = False
EndFunction

bool Function OKToScan()
    if mcmOpen
        AlwaysTrace("MCM for SHSE is open")
        return False
    elseif (Utility.IsInMenuMode())
        AlwaysTrace("UI has menu open")
        return False
    elseif (!Game.IsActivateControlsEnabled())
        AlwaysTrace("UI has controls disabled")
        return False
    endIf
    return True
EndFunction

; Periodic poll to check whether native code can be released. If game closes down this will never happen.
; Enter a poll loop if UI State forbids native code from scanning.
Function CheckReportUIState()
    bool goodToGo = OKToScan()
    AlwaysTrace("UI Good-to-go = " + goodToGo + " for request " + pluginNonce + " plugin-delayed = " + pluginDelayed)
    if goodToGo
        ; if UI was detected open, refresh player's equipped/worn items in case they changed
        if pluginDelayed
            ; force plugin refresh of player's current worn and equipped items
            ResetList(list_type_in_use_items)
        endIf
        ReportOKToScan(pluginDelayed, pluginNonce)
        pluginNonce = 0
        pluginDelayed = false
    else
        pluginDelayed = true
        RegisterForSingleUpdate(0.25)
    endIf
EndFunction

; this should not kick off competing OnUpdate cycles
Function StartCheckReportUIState(int nonce)
    AlwaysTrace("Kick off UI State check for request " + nonce + ", plugin nonce = " + pluginNonce)
    if pluginNonce == 0
        pluginNonce = nonce
        pluginDelayed = false
        CheckReportUIState()
    endIf
EndFunction

; OnUpdate is used only while UI is active, until UI State becomes inactive and native code can resume scanning
Event OnUpdate()
    CheckReportUIState()
EndEvent

; Check UI State is OK for scan thread - block the plugin if not, rechecking on a timed poll
Event OnCheckOKToScan(int nonce)
    StartCheckReportUIState(nonce)
EndEvent

; check if Actor detects player - used for real stealing, or stealibility check in dry run
Event OnStealIfUndetected(int actorCount, bool dryRun)
    ;DebugTrace("Check player detection, actorCount=" + actorCount)
    ; Get all the requested actors to check first, as this is a slow process
    Form[] actors = Utility.CreateFormArray(actorCount)
    int currentActor = 0
    while currentActor < actorCount
        actors[currentActor] = GetDetectingActor(currentActor, dryRun)
        currentActor += 1
    endWhile

    String msg
    bool detected = False
    currentActor = 0
    while currentActor < actorCount && !detected
        if !OKToScan()
            msg = "UI Open : Actor Detection interrupted"
            AlwaysTrace(msg)
            detected = True     ; do not steal items while UI is active
        else
            Actor npc = actors[currentActor] as Actor
            if npc && player.IsDetectedBy(npc)
                msg = "Player detected by " + npc.getActorBase().GetName()
                detected = True
            else
                ;DebugTrace("Player not detected by " + npc.getActorBase().GetName())
            endIf
            currentActor += 1
        endIf
    endWhile

    if dryRun
    	if !detected
            msg = "Player is not detected"
        endIf
        Debug.Notification(msg)
    else
        ReportPlayerDetectionState(detected)
    endIf
EndEvent

; Reset state related to new game/load game
Event OnGameReady()
    ;DebugTrace("SHSE_EventsAlias.OnGameReady")
    ;update CACO index in load order, to handle custom ore mining
    CACOModIndex = Game.GetModByName("Complete Alchemy & Cooking Overhaul.esp")
    if CACOModIndex != 255
        AlwaysTrace("CACO mod index: " + CACOModIndex)
        StrikesBeforeCollection = Game.GetFormFromFile(0xCC0503,"Update.esm") as GlobalVariable
    endif

    ;update Fossil Mining index in load order, to handle fossil handout after mining
    FossilMiningModIndex = Game.GetModByName("Fossilsyum.esp")
    if FossilMiningModIndex != 255
        AlwaysTrace("Fossil Mining mod index: " + FossilMiningModIndex)
        FOS_LItemFossilTierOneGeode = Game.GetFormFromFile(0x3ee7d, "Fossilsyum.esp") as LeveledItem
        FOS_LItemFossilTierOneVolcanic = Game.GetFormFromFile(0x3ee7a, "Fossilsyum.esp") as LeveledItem
        FOS_LItemFossilTierOneyum = Game.GetFormFromFile(0x3c77, "Fossilsyum.esp") as LeveledItem
        FOS_LItemFossilTierOneVolcanicDigSite = Game.GetFormFromFile(0x3f41f, "Fossilsyum.esp") as LeveledItem
        FOS_LItemFossilTierTwoVolcanic = Game.GetFormFromFile(0x3ee7b, "Fossilsyum.esp") as LeveledItem
    endif

    ;update Hearthfire Extended index in load order, to handle Apiary ACTI
    HearthfireExtendedModIndex = Game.GetModByName("hearthfireextended.esp")
    if HearthfireExtendedModIndex != 255
        AlwaysTrace("Hearthfire Extended mod index: " + HearthfireExtendedModIndex)
    endif

    ; yes, this really is the ESP name. And there are three different names.
    MagicChestModIndex = Game.GetModByName("Skyrim_SE_Nexus .esp")
    if MagicChestModIndex == 255
        MagicChestModIndex = Game.GetModByName("Skyrim_SE_Nexus_Chests .esp")
    endif
    if MagicChestModIndex == 255
        MagicChestModIndex = Game.GetModByName("SSENexus OCS Patch.esp")
    endif
    if MagicChestModIndex != 255
        AlwaysTrace("Magic Chest Spell with Multi Linked Containers mod index: " + MagicChestModIndex)
    endif

    ;update Vidani's Bag Of Holding index in load order, to handle Bag as a Transfer List candidate
    VidanisBagOfHoldingModIndex = Game.GetModByName("Vidani's Bag of Holding.esp")
    if VidanisBagOfHoldingModIndex != 255
        AlwaysTrace("Vidani's Bag Of Holding mod index: " + VidanisBagOfHoldingModIndex)
    endif

    ; only need to check Collections requisite data structure on reload, not MCM close
    ResetCollections()
    PushGameTime(Utility.GetCurrentGameTime())
EndEvent
