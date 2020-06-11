Scriptname SHSE_EventsAlias extends ReferenceAlias  

import SHSE_PluginProxy
import SHSE_MCM
GlobalVariable Property g_LootingEnabled Auto
int CACOModIndex
int FossilMiningModIndex

GlobalVariable StrikesBeforeCollection

Message property whitelist_message auto
Message property to_list_message auto
Container Property list_nametag auto
Perk Property greenThumbPerk auto

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
int[] addedItemIDs
int[] addedItemTypes
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

EffectShader lootedShader    ; white
EffectShader bossShader      ; flames
EffectShader lockedShader    ; red
EffectShader questShader     ; purple
EffectShader enchantedShader ; blue
EffectShader ownedShader     ; green

int getType_kFlora = 39

float g_interval = 0.3
float min_interval = 0.1

Formlist Property whitelist_form auto
Formlist Property blacklist_form auto
int location_type_whitelist = 1
int location_type_blacklist = 2
int maxMiningItems
int infiniteWeight = 100000

int glowReasonBossContainer
int glowReasonQuestObject
int glowReasonLockedContainer
int glowReasonEnchantedItem
int glowReasonPlayerProperty
int glowReasonSimpleTarget

Function SetPlayer(Actor playerref)
    player = playerref
EndFunction

Function SyncWhiteList()
    SyncWhiteListWithPlugin()
endFunction

; there are two sources of excluded locations- basket file and the script form. Merge these as best we can.
Function SyncBlackList()
    ClearPluginBlackList()
    ; ensure locations in the BlackList Form are present in the plugin's list
    int index = blacklist_form.GetSize()
    int current = 0
    while (current < index)
        Form nextLocation = blacklist_form.GetAt(current)
        if (nextLocation)
            AddLocationToList(location_type_blacklist, nextLocation)
        endif
        current += 1
    endwhile
    MergePluginBlackList()
endFunction

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
        DropLocationFromList(location_type, m_item)
    else
        string translation = GetTranslation(trans_add)
        if (translation)
            string msg = Replace(translation, "{ITEMNAME}", GetNameForListForm(m_item))
            if (msg)
                Debug.Notification(msg)
            endif
        endif
        m_list.AddForm(m_item)
        AddLocationToList(location_type, m_item)
    endif
endFunction

function ManageWhiteList(Form itemForm)
    ManageList(whitelist_form, itemForm, location_type_whitelist, "$SHSE_WHITELIST_ADDED", "$SHSE_WHITELIST_REMOVED")
endFunction

function MoveFromBlackToWhiteList(Form itemForm)
    if (blacklist_form.find(itemForm) != -1)
        int result = ShowMessage(to_list_message, "$SHSE_MOVE_TO_WHITELIST", "{ITEMNAME}", GetNameForListForm(itemForm))
        if (result == 0)
            blacklist_form.removeAddedForm(itemForm)
        else
            return
        endif
    endif
    ManageWhiteList(itemForm)
endFunction

function MoveFromWhiteToBlackList(Form itemForm)
    if (whitelist_form.find(itemForm) != -1)
        int result = ShowMessage(to_list_message, "$SHSE_MOVE_TO_BLACKLIST", "{ITEMNAME}", GetNameForListForm(itemForm))
        if (result == 0)
            whitelist_form.removeAddedForm(itemForm)
        else
            return
        endif
    endif
    ManageBlackList(itemForm)
endFunction

function ManageBlackList(Form itemForm)
    ManageList(blacklist_form, itemForm, location_type_blacklist, "$SHSE_BLACKLIST_ADDED", "$SHSE_BLACKLIST_REMOVED")
endFunction

; must line up with enumerations from C++
Function SyncNativeDataTypes()
    objType_Flora = GetObjectTypeByName("flora")
    objType_Critter = GetObjectTypeByName("critter")
    objType_Septim = GetObjectTypeByName("septims")
    objType_Soulgem = GetObjectTypeByName("soulgem")
    objType_Mine = GetObjectTypeByName("oreVein")
    objType_Book = GetObjectTypeByName("book")
    objType_skillBookRead = GetObjectTypeByName("skillbookread")

    resource_Ore = GetResourceTypeByName("Ore")
    resource_Geode = GetResourceTypeByName("Geode")
    resource_Volcanic = GetResourceTypeByName("Volcanic")
    resource_VolcanicDigSite = GetResourceTypeByName("VolcanicDigSite")

    glowReasonBossContainer = 1
    glowReasonQuestObject = 2
    glowReasonEnchantedItem = 3
    glowReasonLockedContainer = 4
    glowReasonPlayerProperty = 5
    glowReasonSimpleTarget = 6

    ; must line up with shaders defined in ESP/ESM file
    lootedShader = Game.GetFormFromFile(0xa9e1, "SmartHarvestSE.esp") as EffectShader   ; white
    enchantedShader = Game.GetFormFromFile(0xa9dc, "SmartHarvestSE.esp") as EffectShader    ; blue
    lockedShader = Game.GetFormFromFile(0xa9df, "SmartHarvestSE.esp") as EffectShader   ; red
    bossShader = Game.GetFormFromFile(0xa9e2, "SmartHarvestSE.esp") as EffectShader     ; gold
    questShader = Game.GetFormFromFile(0xa9de, "SmartHarvestSE.esp") as EffectShader        ; purple
    ownedShader = Game.GetFormFromFile(0xa9dd, "SmartHarvestSE.esp") as EffectShader        ; green
endFunction

Function ResetAddedItems()
    ;prepare for collection management, if configured
    collectionsInUse = CollectionsInUse()
    if !collectionsInUse
        return
    endIf
    ;DebugTrace("eventScript.ResetAddedItems")
    addedItemIDs = New Int[128]
    addedItemTypes = New Int[128]
    maxAddedItems = 128
    currentAddedItem = 0
EndFunction

Function ApplySetting()

    ;DebugTrace("eventScript ApplySetting start")

    UnregisterForAllKeys()
    UnregisterForMenu("Loading Menu")

    int s_pauseKey = GetConfig_Pausekey()
    if (s_pauseKey != 0)
         RegisterForKey(s_pauseKey)
    endif
    
    int s_whiteListKey = GetConfig_WhiteListKey()
    if (s_whiteListKey != 0)
        if (s_whiteListKey != s_pauseKey)
            RegisterForKey(s_whiteListKey)
        endif
    endif

    int s_blackListKey = GetConfig_BlackListKey()
    if (s_blackListKey != 0)
        if (s_blackListKey != s_whiteListKey && s_blackListKey != s_pauseKey)
            RegisterForKey(s_blackListKey)
        endif
    endif

    ;update CACO index in load order, to handle custom ore mining
    CACOModIndex = Game.GetModByName("Complete Alchemy & Cooking Overhaul.esp")
    if (CACOModIndex != 255)
        ;DebugTrace("CACO mod index: " + CACOModIndex)
        StrikesBeforeCollection = Game.GetFormFromFile(0xCC0503,"Update.esm") as GlobalVariable
    endif

    ;update Fossil Mining index in load order, to handle fossil handout after mining
    FossilMiningModIndex = Game.GetModByName("Fossilsyum.esp")
    if (FossilMiningModIndex != 255)
        ;DebugTrace("Fossil Mining mod index: " + FossilMiningModIndex)
        FOS_LItemFossilTierOneGeode = Game.GetFormFromFile(0x3ee7d, "Fossilsyum.esp") as LeveledItem
        FOS_LItemFossilTierOneVolcanic = Game.GetFormFromFile(0x3ee7a, "Fossilsyum.esp") as LeveledItem
        FOS_LItemFossilTierOneyum = Game.GetFormFromFile(0x3c77, "Fossilsyum.esp") as LeveledItem
        FOS_LItemFossilTierOneVolcanicDigSite = Game.GetFormFromFile(0x3f41f, "Fossilsyum.esp") as LeveledItem
        FOS_LItemFossilTierTwoVolcanic = Game.GetFormFromFile(0x3ee7b, "Fossilsyum.esp") as LeveledItem
    endif

    ;DebugTrace("eventScript ApplySetting start")

    SyncWhiteList()
    SyncBlackList()
    ResetAddedItems()

    utility.waitMenumode(g_interval)
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

;hotkey changes sense of looting
function Pause()
    string s_enableStr = none
    int priorState = g_LootingEnabled.GetValue() as int
    if (priorState != 0)
        g_LootingEnabled.SetValue(0)
        DisallowSearch()
    else
        g_LootingEnabled.SetValue(1)
        AllowSearch()
    endif
        
    ;DebugTrace("Pause, looting-enabled toggled to = " + g_LootingEnabled.GetValue())
    string str = sif(priorState == 0, "$SHSE_ENABLE", "$SHSE_DISABLE")
    str = Replace(GetTranslation(str), "{VERSION}", GetPluginVersion())
    Debug.Notification(str)
endFunction

Event OnKeyUp(Int keyCode, Float holdTime)
    if (UI.IsTextInputEnabled())
        return
    endif

    if (!Utility.IsInMenumode())
        if (keyCode == GetConfig_Pausekey())
            Pause()
        endif
    else
        string s_menuName = none
        if (UI.IsMenuOpen("InventoryMenu"))
            s_menuName = "InventoryMenu"
        elseif (UI.IsMenuOpen("ContainerMenu"))
            s_menuName = "ContainerMenu"
        endif
        
        if (s_menuName == "ContainerMenu" || s_menuName == "InventoryMenu")

            Form itemForm = GetSelectedItemForm(s_menuName)
            if (!itemForm)
                string msg = "$SHSE_whitelist_form_ERROR"
                Debug.Notification(msg)
                ;DebugTrace(msg)
                return
            endif

            int result = -1
            if (GetConfig_WhiteListKey() == GetConfig_BlackListKey())
                result = ShowMessage(whitelist_message, "$SHSE_REGISTER_LIST", "{ITEMNAME}", GetNameForListForm(itemForm))
            elseif (keyCode == GetConfig_WhiteListKey())
                result = 0
            elseif (keyCode == GetConfig_BlackListKey())
                result = 1
            else
                return
            endif
            
            if (result == 0)
                MoveFromBlackToWhiteList(itemForm)
            elseIf (result == 1)
                MoveFromWhiteToBlackList(itemForm)
            EndIf
            SyncBlackList()
            SyncWhiteList()
        endif
    endif
endEvent

int Function ShowMessage(Message m_msg, string m_trans, string m_target_text = "", string m_replace_text = "")
    if (!m_msg)
        return -1
    endif
    string str = GetTranslation(m_trans)
    if (m_target_text != "" && m_replace_text != "")
        str = Replace(str, m_target_text, m_replace_text)
    endif
    list_nametag.setName(str)
    int result = m_msg.Show()
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

Function RecordItem(Form akBaseItem, int objectType)
    ;register item received in the 'collection pending' list
    ;DebugTrace("RecordItem " + akBaseItem)
    if !collectionsInUse
        return
    endIf
    if currentAddedItem == maxAddedItems
        ; list is full, flush to the plugin
    FlushAddedItems(addedItemIDs, addedItemTypes, currentAddedItem)
    currentAddedItem = 0
    endif
    addedItemIDs[currentAddedItem] = akBaseItem.GetFormID()
    addedItemTypes[currentAddedItem] = objectType
    currentAddedItem += 1
EndFunction

bool Function ActivateEx(ObjectReference akTarget, ObjectReference akActivator, bool isSilentMessage = false)
    bool bShowHUD = Utility.GetINIBool("bShowHUDMessages:Interface")
    if (bShowHUD && isSilentMessage)
        Utility.SetINIBool("bShowHUDMessages:Interface", false)
    endif
    bool result = akTarget.Activate(akActivator)
    if (bShowHUD && isSilentMessage)
        Utility.SetINIBool("bShowHUDMessages:Interface", true)
    endif
    return result
endFunction

Event OnMining(ObjectReference akMineable, int resourceType, bool manualLootNotify)
    ;DebugTrace("OnMining: " + akMineable.GetDisplayName() + "RefID(" +  akMineable.GetFormID() + ")  BaseID(" + akMineable.GetBaseObject().GetFormID() + ")" ) 
    ;DebugTrace("resource type: " + resourceType + ", notify for manual loot: " + manualLootNotify)
    int miningStrikes = 0
    int targetResourceTotal = 0
    int strikesToCollect = 0
    MineOreScript oreScript = akMineable as MineOreScript
    string oreName = ""
    int FOSStrikesBeforeFossil
    bool handled = false
    if (oreScript)
        ;DebugTrace("Detected ore vein")
        ; brute force ore gathering to bypass tedious MineOreScript/Furniture handshaking
        targetResourceTotal = oreScript.ResourceCountTotal
        strikesToCollect = oreScript.StrikesBeforeCollection
        int available = oreScript.ResourceCountCurrent
        oreName = oreScript.ore.GetName()
        int mined = 0
        if (available == -1)
            ;DebugTrace("Vein not yet initialized, start mining")
        else
            ;DebugTrace("Vein has ore available: " + available)
        endif

        ; 'available' is set to -1 before the vein is initialized - after we call giveOre the amount received is
        ; in ResourceCount and the remaining amount in ResourceCountCurrent 
        while (available != 0 && mined < maxMiningItems)
                ;DebugTrace("Trigger harvesting")
            oreScript.giveOre()
            mined += oreScript.ResourceCount
            ;DebugTrace("Ore amount so far: " + mined + ", this time: " + oreScript.ResourceCount + ", max: " + maxMiningItems)
            available = oreScript.ResourceCountCurrent
            miningStrikes += 1
        endwhile
        ;DebugTrace("Ore harvested amount: " + mined + ", remaining: " + oreScript.ResourceCountCurrent)
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
            if cacoMinable.Ore
                oreName = cacoMinable.Ore.GetName()
            elseif cacoMinable.lItemGems10
                oreName = "gems"
            endif
            int mined = 0
            if (available == -1)
                ;DebugTrace("CACO ore vein not yet initialized, start mining")
            else
                ;DebugTrace("CACO ore vein has ore available: " + available)
            endif

            ; 'available' is set to -1 before the vein is initialized - after we call giveOre the amount received is
            ; in ResourceCount and the remaining amount in ResourceCountCurrent 
            while (available != 0 && mined < maxMiningItems)
                ;DebugTrace("Trigger CACO ore harvesting")
                cacoMinable.giveOre()
                mined += cacoMinable.ResourceCount
                ;DebugTrace("CACO ore vein amount so far: " + mined + ", this time: " + cacoMinable.ResourceCount + ", max: " + maxMiningItems)
                available = cacoMinable.ResourceCountCurrent
                miningStrikes += 1
            endwhile
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

    ; Fossil Mining Drop Logic from oreVein per Fos_AttackMineAlias.psc, bypassing the FURN.
    ; Excludes Hearthfire house materials to mimic FOS_IgnoreList filtering.
    ; Excludes Fossil Mining DIg Sites, processed in full above
    if (miningStrikes > 0 && FossilMiningModIndex != 255 && resourceType != resource_VolcanicDigSite && oreName != "Quarried Stone" && oreName != "Clay")
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
        ; unrecognized 'Mine' target? glow as a 'nearby manual lootable' if configured to do so
        DoObjectGlow(akMineable, 5, glowReasonSimpleTarget)
    endif

EndEvent

Event OnHarvest(ObjectReference akTarget, int itemType, int count, bool silent, bool manualLootNotify)
    ;DebugTrace("OnHarvest:Run: " + akTarget.GetDisplayName() + "RefID(" +  akTarget.GetFormID() + ")  BaseID(" + akTarget.GetBaseObject().GetFormID() + ")" ) 
    ;DebugTrace("item type: " + itemType + ", do not notify: " + silent + "notify for manual loot: " + manualLootNotify)
    bool notify = false
    form baseForm = akTarget.GetBaseObject()

    if (IsBookObject(itemType))
        player.AddItem(akTarget, count, true)
        RecordItem(baseForm, itemType)

        notify = !silent
    elseif (itemType == objType_Soulgem && akTarget.GetLinkedRef(None))
        ; no-op but must still unlock

    elseif (!akTarget.IsActivationBlocked())
        if (itemType == objType_Flora)
            if (ActivateEx(akTarget, player, silent) && (greenThumbPerk && player.HasPerk(greenThumbPerk)) as bool)
                float greenThumbValue = greenThumbPerk.GetNthEntryValue(0, 0)
                int countPP = ((count * greenThumbValue) - count) as int
                if (countPP >= 1)
                    player.AddItem(baseForm, countPP, true)
                endif
            endif
        elseif (itemType == objType_Critter)
            ActivateEx(akTarget, player, silent)
        elseif (itemType == objType_Septim && baseForm.GetType() == getType_kFlora)
            ActivateEx(akTarget, player, silent)
        elseif (ActivateEx(akTarget, player, true) && !silent)
            notify = true
        endif
        RecordItem(baseForm, itemType)
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
Event OnLootFromNPC(ObjectReference akContainerRef, Form akForm, int count, int itemType)
    ;DebugTrace("OnLootFromNPC: " + akContainerRef.GetDisplayName() + " " + akForm.GetName() + "(" + count + ")")
    if (!akContainerRef)
        return
    elseif (!akForm)
        return
    endif

    akContainerRef.RemoveItem(akForm, count, true, player)
    RecordItem(akForm, itemType)
endEvent

Event OnGetCritterIngredient(ObjectReference akTarget)
    ;DebugTrace("OnGetCritterIngredient " + akTarget.GetDisplayName() + "RefID(" +  akTarget.GetFormID() + ")  BaseID(" + akTarget.GetBaseObject().GetFormID() + ")" )
    Critter thisCritter = akTarget as Critter
    if (thisCritter)
        ;DebugTrace("setting ingredient " + thisCritter.lootable.GetName() + " for critter " + baseForm.GetName())
        form baseForm = akTarget.GetBaseObject()
        SetIngredientForCritter(baseForm, thisCritter.lootable)
    endif
endEvent

Function DoObjectGlow(ObjectReference akTargetRef, int duration, int reason)
    EffectShader effShader
    if (reason == glowReasonBossContainer)
        effShader = bossShader
    elseif (reason == glowReasonQuestObject)
        effShader = questShader
    elseif (reason == glowReasonEnchantedItem)
        effShader = enchantedShader
    elseif (reason == glowReasonLockedContainer)
        effShader = lockedShader
    elseif (reason == glowReasonPlayerProperty)
        effShader = ownedShader
    else
        effShader = lootedShader
    endif

    if (effShader)
        ; play for requested duration - C++ code will tidy up when out of range
        ;DebugTrace("OnObjectGlow for " + akTargetRef.GetDisplayName() + " for " + duration + " seconds")
        effShader.Play(akTargetRef, duration)
    endif
endFunction

Event OnObjectGlow(ObjectReference akTargetRef, int duration, int reason)
    DoObjectGlow(akTargetRef, duration, reason)
endEvent

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

Event OnResetCarryWeight()
    ;DebugTrace("Player carry weight reset request")
    RemoveCarryWeightDelta()
EndEvent

Event OnFlushAddedItems()
    ;DebugTrace("Request to flush added items")
    ; no-op if empty list
    if currentAddedItem > 0
        FlushAddedItems(addedItemIDs, addedItemTypes, currentAddedItem)
        currentAddedItem = 0
    endif
EndEvent

Event OnMenuOpen(String MenuName)
    if (MenuName == "Loading Menu")
        UnblockEverything()
    endif
endEvent
