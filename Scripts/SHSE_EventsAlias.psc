Scriptname SHSE_EventsAlias extends ReferenceAlias  

import SHSE_PluginProxy
import SHSE_MCM
GlobalVariable Property g_LootingEnabled Auto
int CACOModIndex
int FossilMiningModIndex
int HearthfireExtendedModIndex
bool scanActive
int pluginNonce
bool pluginDelayed

GlobalVariable StrikesBeforeCollection

Message property whitelist_message auto
Message property to_list_message auto
Container Property list_nametag auto

int type_Common = 1
int type_Harvest = 2

; object types must be in sync with the native DLL
int objType_Flora
int objType_Critter
int objType_Septim
int objType_Soulgem
int objType_Mine
int objType_Book
int objType_skillBookRead

Actor player
Form[] addedItems
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

Formlist Property whitelist_form auto
Formlist Property blacklist_form auto
int location_type_whitelist
int location_type_blacklist
int pauseKeyCode
int whiteListKeyCode
int blackListKeyCode

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

Function SetPlayer(Actor playerref)
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

; merge FormList with plugin data
Function SyncList(int listNum, FormList forms)
    ; plugin resets to fixed baseline
    ResetList(listNum)
    ; ensure BlackList/WhiteList members are present in the plugin's list
    int index = forms.GetSize()
    int current = 0
    while (current < index)
        Form nextEntry = forms.GetAt(current)
        if (nextEntry)
            AddEntryToList(listNum, nextEntry)
        endif
        current += 1
    endwhile
endFunction

;push updated lists to plugin
Function SyncLists(bool reload, bool updateLists)
    if updateLists
        SyncList(location_type_whitelist, whitelist_form)
        SyncList(location_type_blacklist, blacklist_form)
    endIf
    SyncDone(reload)
endFunction

; manages FormList in VM - SyncLists pushes state to plugin once all local operations are complete
function ManageList(Formlist m_list, Form m_item, int location_type, string trans_add, string trans_remove) global
    if (!m_list || !m_item)
        return
    endif

    if(m_list.Find(m_item) != -1)
        string translation = GetTranslation(trans_remove)
        if (translation)
            string msg = Replace(translation, "{ITEMNAME}", GetNameForListForm(m_item))
            if (msg)
                Debug.Notification(msg)
            endif
        endif
        m_list.RemoveAddedForm(m_item)
    else
        string translation = GetTranslation(trans_add)
        if (translation)
            string msg = Replace(translation, "{ITEMNAME}", GetNameForListForm(m_item))
            if (msg)
                Debug.Notification(msg)
            endif
        endif
        m_list.AddForm(m_item)
    endif
endFunction

function MoveFromBlackToWhiteList(Form target, bool confirm)
    if (blacklist_form.find(target) != -1)
        int result = 0
        if confirm
            result = ShowMessage(to_list_message, "$SHSE_MOVE_TO_WHITELIST", "{ITEMNAME}", GetNameForListForm(target))
        endif
        if (result == 0)
            blacklist_form.removeAddedForm(target)
        else
            return
        endif
    endif
    ManageWhiteList(target)
endFunction

function RemoveFromBlackList(Form target)
    if (blacklist_form.find(target) != -1)
        blacklist_form.removeAddedForm(target)
    endif
endFunction

function ManageWhiteList(Form target)
    ManageList(whitelist_form, target, location_type_whitelist, "$SHSE_WHITELIST_ADDED", "$SHSE_WHITELIST_REMOVED")
endFunction

function AddToBlackList(Form target)
    ManageBlackList(target)
endFunction

function MoveFromWhiteToBlackList(Form target, bool confirm)
    if (whitelist_form.find(target) != -1)
        int result = 0
        if confirm
            result = ShowMessage(to_list_message, "$SHSE_MOVE_TO_BLACKLIST", "{ITEMNAME}", GetNameForListForm(target))
        endif
        if (result == 0)
            whitelist_form.removeAddedForm(target)
        else
            return
        endif
    endif
    ManageBlackList(target)
endFunction

function ManageBlackList(Form target)
    ManageList(blacklist_form, target, location_type_blacklist, "$SHSE_BLACKLIST_ADDED", "$SHSE_BLACKLIST_REMOVED")
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

    infiniteWeight = 100000
endFunction

Function ResetCollections()
    ;prepare for collection management, if configured
    collectionsInUse = CollectionsInUse()
    if !collectionsInUse
        return
    endIf
    ;DebugTrace("eventScript.ResetCollections")
    addedItems = New Form[128]
    maxAddedItems = 128
    currentAddedItem = 0
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

Function HandleCrosshairItemHotKey(ObjectReference targetedRefr, bool isWhiteKey, Float holdTime)
    ; check for long press
    if holdTime > 3.0
        if isWhiteKey
            if targetedRefr.GetBaseObject() as Container
                ProcessContainerCollectibles(targetedRefr)
            else
                Debug.Notification("$SHSE_HOTKEY_NOT_A_CONTAINER")
            endif
        else ; BlackList Key
            ; object lootability introspection
            CheckLootable(targetedRefr)
        endIf
    else
        ; regular press. Does nothing unless this is a non-generated Dead Body or Container
        bool valid = False
        if !HasDynamicData(targetedRefr)
            Actor refrActor = targetedRefr as Actor
            Container refrContainer = targetedRefr.GetBaseObject() as Container
            if (refrActor && refrActor.IsDead()) || refrContainer
                ; blacklist or un-blacklist the REFR, not the Base, to avoid blocking other REFRs with same Base
                if isWhiteKey
                    RemoveFromBlackList(targetedRefr)
                else ; BlackList Key
                    AddToBlackList(targetedRefr)
                EndIf
                SyncLists(false, true)    ; not a reload
                valid = True
            endIf
        endIf
        if !valid
            Debug.Notification("$SHSE_HOTKEY_NOT_A_CONTAINER_OR_NPC")
        endIf
    endIf
EndFunction

Event OnKeyUp(Int keyCode, Float holdTime)
    if (UI.IsTextInputEnabled())
        return
    endif

    if (!Utility.IsInMenumode())
        if keyCode == pauseKeyCode
            if holdTime > 3.0
                ; trigger shader test on really long press
                ToggleCalibration(holdTime > 10.0)
            else
                Pause()
            endif
        elseif keyCode == whiteListKeyCode || keyCode == blackListKeyCode
            ; handle hotkey actions for crosshair in reference
            ObjectReference targetedRefr = Game.GetCurrentCrosshairRef()
            if targetedRefr
                HandleCrosshairItemHotKey(targetedRefr, keyCode == whiteListKeyCode, holdTime)
                return
            endIf

            ; Location/cell blacklist whitelist toggle in worldspace
            Form place = GetPlayerPlace()
            if (!place)
                string msg = "$SHSE_whitelist_form_ERROR"
                Debug.Notification(msg)
                return
            endif
            int result = -1
            if keyCode == whiteListKeyCode
                result = 0
            else ; blacklist key
                result = 1
            endif

            if (result == 0)
                MoveFromBlackToWhiteList(place, false)
            elseIf (result == 1)
                MoveFromWhiteToBlackList(place, false)
            EndIf
            SyncLists(false, true)    ; not a reload
        endif
    ; menu open - only actionable on our blacklist/whitelist keys
    elseif keyCode == whiteListKeyCode || keyCode == blackListKeyCode
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
                return
            endif
            if IsQuestTarget(itemForm)
                string msg
                if keyCode == whiteListKeyCode
                    msg = "$SHSE_WHITELIST_QUEST_TARGET"
                else
                    msg = "$SHSE_BLACKLIST_QUEST_TARGET"
                endIf
                Debug.Notification(msg)
                return
            endif

            int result = -1
            if keyCode == whiteListKeyCode
                result = 0
            else ; blacklist key
                result = 1
            endif
            
            if (result == 0)
                MoveFromBlackToWhiteList(itemForm, false)
            elseIf (result == 1)
                MoveFromWhiteToBlackList(itemForm, false)
            EndIf
            SyncLists(false, true)    ; not a reload
        endif
    endif
endEvent

int Function ShowMessage(Message msg, string trans, string target_text = "", string replace_text = "")
    if (!msg)
        return -1
    endif
    string str = GetTranslation(trans)
    if (target_text != "" && replace_text != "")
        str = Replace(str, target_text, replace_text)
    endif
    list_nametag.setName(str)
    int result = msg.Show()
    list_nametag.setName("dummy_name")
    return result
endFunction

function updateMaxMiningItems(int maxItems)
    ;DebugTrace("maxMiningItems -> " + maxItems)
    maxMiningItems = maxItems
endFunction

bool Function IsBookObject(int type)
    return type >= objType_Book && type <= objType_skillBookRead
endFunction

Function RecordItem(Form akBaseItem)
    ;register item received in the 'collection pending' list
    ;DebugTrace("RecordItem " + akBaseItem)
    if !collectionsInUse
        return
    endIf
    if currentAddedItem == maxAddedItems
        ; list is full, flush to the plugin
        FlushAddedItems(Utility.GetCurrentGameTime(), addedItems, currentAddedItem)
        currentAddedItem = 0
    endif
    addedItems[currentAddedItem] = akBaseItem
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
    ;DebugTrace("OnMining: " + akMineable.GetDisplayName() + "RefID(" +  akMineable.GetFormID() + ")  BaseID(" + akMineable.GetBaseObject().GetFormID() + ")" ) 
    ;DebugTrace("resource type: " + resourceType + ", notify for manual loot: " + manualLootNotify)
    int miningStrikes = 0
    int targetResourceTotal = 0
    int strikesToCollect = 0
    MineOreScript oreScript = akMineable as MineOreScript
    int FOSStrikesBeforeFossil
    bool handled = false
    if (oreScript)
        ;DebugTrace("Detected ore vein")
        ; brute force ore gathering to bypass tedious MineOreScript/Furniture handshaking
        targetResourceTotal = oreScript.ResourceCountTotal
        strikesToCollect = oreScript.StrikesBeforeCollection
        int available = oreScript.ResourceCountCurrent
        int mined = 0
        bool useSperg = spergProspector && player.HasPerk(spergProspector)
        if useSperg
            PrepareSPERGMining()
        endif
        if (available == -1)
            ;DebugTrace("Vein not yet initialized, start mining")
        else
            ;DebugTrace("Vein has ore available: " + available)
        endif

        ; 'available' is set to -1 before the vein is initialized - after we call giveOre the amount received is
        ; in ResourceCount and the remaining amount in ResourceCountCurrent 
        while OKToScan() && available != 0 && mined < maxMiningItems
            ;DebugTrace("Trigger harvesting")
            oreScript.giveOre()
            mined += oreScript.ResourceCount
            ;DebugTrace("Ore amount so far: " + mined + ", this time: " + oreScript.ResourceCount + ", max: " + maxMiningItems)
            available = oreScript.ResourceCountCurrent
            miningStrikes += 1
        endwhile
        if !OKToScan()
            AlwaysTrace("UI open : oreScript mining interrupted, " + mined + " obtained")
        endIf
        ;DebugTrace("Ore harvested amount: " + mined + ", remaining: " + oreScript.ResourceCountCurrent)
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
            ;DebugTrace("Detected CACO ore vein")
            ; brute force ore gathering to bypass tedious MineOreScript/Furniture handshaking
            int available = cacoMinable.ResourceCountCurrent
            targetResourceTotal = cacoMinable.ResourceCountTotal
            strikesToCollect = cacoMinable.StrikesBeforeCollection
            int mined = 0
            ; do not harvest firehose unless set in config
            if (available == -1)
                ;DebugTrace("CACO ore vein not yet initialized, start mining")
            else
                ;DebugTrace("CACO ore vein has ore available: " + available)
            endif

            ; 'available' is set to -1 before the vein is initialized - after we call giveOre the amount received is
            ; in ResourceCount and the remaining amount in ResourceCountCurrent 
            while OKToScan() && available != 0 && mined < maxMiningItems
                ;DebugTrace("Trigger CACO ore harvesting")
                cacoMinable.giveOre()
                mined += cacoMinable.ResourceCount
                ;DebugTrace("CACO ore vein amount so far: " + mined + ", this time: " + cacoMinable.ResourceCount + ", max: " + maxMiningItems)
                available = cacoMinable.ResourceCountCurrent
                miningStrikes += 1
            endwhile
            if !OKToScan()
                AlwaysTrace("UI open : CACO_MineOreScript mining interrupted, " + mined + " obtained")
            endIf
            ;DebugTrace("CACO ore vein harvested amount: " + mined + ", remaining: " + oreScript.ResourceCountCurrent)
            handled = true
        endif
    endif
    if !handled && (FossilMiningModIndex != 255)
        ;DebugTrace("Check for Fossil Mining Dig Site")
        FOS_DigsiteScript FOSMinable = akMineable as FOS_DigsiteScript
        if (FOSMinable)
            ;DebugTrace("Process Fossil Mining Dig Site")
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
        ;DebugTrace("Fossil Mining: strikes = " + miningStrikes + ", required for drop = " + FOSStrikesBeforeFossil)
        if (dropFactor <= miningStrikes)
            ;DebugTrace("Fossil Mining: provide loot!")
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


Event OnHarvest(ObjectReference akTarget, int itemType, int count, bool silent, bool collectible, float ingredientCount)
    bool notify = false
    form baseForm = akTarget.GetBaseObject()

    ;DebugTrace("OnHarvest:Run: target " + akTarget + ", base " + baseForm) 
    ;DebugTrace(", item type: " + itemType + ", do not notify: " + silent + ")

    if (IsBookObject(itemType))
        player.AddItem(akTarget, count, true)
        if collectible
            RecordItem(baseForm)
        endIf

        notify = !silent
    elseif (itemType == objType_Soulgem && akTarget.GetLinkedRef(None))
        ; no-op but must still unlock

    elseif (!akTarget.IsActivationBlocked())
        if (itemType == objType_Septim && baseForm.GetType() == getType_kFlora)
            ActivateEx(akTarget, player, silent, 1)

        elseif baseForm.GetType() == getType_kFlora || baseForm.GetType() == getType_kTree
            ; "Flora" or "Tree" Producer REFRs cannot be identified by item type
            ;DebugTrace("Player has ingredient count " + ingredientCount)
            bool suppressMessage = silent || ingredientCount as int > 1
            ;DebugTrace("Flora/Tree original base form " + baseForm.GetName())
            if ActivateEx(akTarget, player, suppressMessage, 1)
                ;we must send the message if required default would have been incorrect
                notify = !silent && ingredientCount as int > 1
                count = count * ingredientCount as int
            endif
        ; Critter ACTI REFRs cannot be identified by item type
        else
            int critterActivations = SupportedCritterActivateCount(akTarget)
            if critterActivations > 0
                ;DebugTrace("Critter " + baseForm.GetName())
                ActivateEx(akTarget, player, silent, critterActivations)
            elseif ActivateEx(akTarget, player, true, 1)
                notify = !silent
                if count >= 2
                    ; work round for ObjectReference.Activate() known issue
                    ; https://www.creationkit.com/fallout4/index.php?title=Activate_-_ObjectReference
                    int toGet = count - 1
                    player.AddItem(baseForm, toGet, true)
                    ;DebugTrace("Add extra count " + toGet + " of " + baseForm)
                endIf
            endIf
        endif
        if collectible
            RecordItem(baseForm)
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
            replacements[0] = baseForm.GetName()
            replacements[1] = count as string
            
            activateMsg = ReplaceArray(translation, targets, replacements)
        else
            string translation = GetTranslation("$SHSE_ACTIVATE_MSG")
            activateMsg = Replace(translation, "{ITEMNAME}", baseForm.GetName())
        endif
        if (activateMsg)
            Debug.Notification(activateMsg)
        endif
    endif
    
    UnlockHarvest(akTarget, silent)
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
        RecordItem(akForm)
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
        FlushAddedItems(Utility.GetCurrentGameTime(), addedItems, currentAddedItem)
        currentAddedItem = 0
    else
        PushGameTime(Utility.GetCurrentGameTime())
    endIf
EndEvent

bool Function OKToScan()
    if (Utility.IsInMenuMode())
        ;DebugTrace("UI has menu open")
        return False
    elseif (!Game.IsActivateControlsEnabled())
        ;DebugTrace("UI has controls disabled")
        return False
    endIf
    return True
EndFunction

; Periodic poll to check whether native code can be released. If game closes down this will never happen.
; Enter a poll loop if UI State forbids native code from scanning.
Function CheckReportUIState()
    bool goodToGo = OKToScan()
    if goodToGo
        ;DebugTrace("Report UI Good-to-go = " + goodToGo + " for request " + nonce)
        ReportOKToScan(pluginDelayed, pluginNonce)
        pluginNonce = 0
        pluginDelayed = false
    else
        pluginDelayed = true
        RegisterForSingleUpdate(2.0)
    endIf
EndFunction

; this should not kick off competing OnUpdate cycles
Function StartCheckReportUIState(int nonce)
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
    int currentActor = 0
    bool detected = False
    ;DebugTrace("Check player detection, actorCount=" + actorCount)
    String msg
    while currentActor < actorCount && !detected
        if !OKToScan()
            msg = "UI Open : Actor Detection interrupted"
            AlwaysTrace(msg)
            detected = True     ; do not steal items while UI is active
        else
            Actor npc = GetDetectingActor(currentActor, dryRun)
            if player.IsDetectedBy(npc)
                msg = "Player detected by " + npc.getActorBase().GetName()
                detected = True
            else
                ;DebugTrace("Player not detected by " + npc.getActorBase().GetName())
            endIf
            currentActor = currentActor + 1
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

    ; only need to check Collections requisite data structure on reload, not MCM close
    ResetCollections()
    PushGameTime(Utility.GetCurrentGameTime())
    SyncLists(True, True)
    ; kick off scan thread release checking once game is ready. Use sentinel value for this case.
    StartCheckReportUIState(-1)
EndEvent
