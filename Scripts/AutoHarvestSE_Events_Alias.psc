Scriptname AutoHarvestSE_Events_Alias extends ReferenceAlias  

import AutoHarvestSE
import AutoHarvestSE_mcm
GlobalVariable Property g_LootingEnabled Auto
GlobalVariable Property g_CACOModIndex Auto

Keyword property CastKwd auto

Message property userlist_message auto
Message property house_check_message auto
Message property to_list_message auto
Container Property list_nametag auto
Perk Property greenThumbPerk auto

int type_Common = 1
int type_AutoHarvest = 2
int type_Spell = 3

; object types must be in sync with the native DLL
int objType_Flora
int objType_Critter
int objType_Septim
int objType_Soulgem
int objType_Mine
int objType_Book
int objType_skillBookRead

EffectShader lootedShader		; white
EffectShader bossShader	; green
EffectShader lockedShader		; red
EffectShader questShader		; purple
EffectShader enchantedShader	; blue
EffectShader ownedShader	; blue

int getType_kFlora = 39

float g_interval = 0.3
float min_interval = 0.1

Formlist Property userlist_form auto
Formlist Property excludelist_form auto
int location_type_user = 1
int location_type_excluded = 2
int maxMiningItems
int infiniteWeight = 100000

int glowReasonBossContainer
int glowReasonQuestObject
int glowReasonLockedContainer
int glowReasonEnchantedItem
int glowReasonPlayerProperty
int glowReasonSimpleTarget

Function SyncUserList()
	SyncUserListWithPlugin()
endFunction

; there are two sources of excluded locations- basket file and the script form. Merge these as best we can.
Function SyncExcludeList()
	ClearPluginExcludeList()
	; ensure locations in the ExcludeList Form are present in the plugin's list
	int index = excludelist_form.GetSize()
	int current = 0
	while (current < index)
        Form nextLocation = excludelist_form.GetAt(current)
        if (nextLocation)
        	AddLocationToList(location_type_excluded, nextLocation)
        endif
		current += 1
	endwhile
	MergePluginExcludeList()
endFunction

function ManageList(Formlist m_list, Form m_item, int location_type, string trans_add, string trans_remove) global
	if (!m_list || !m_item)
		return
	endif

	if(m_list.Find(m_item) != -1)
		string translation = GetTranslation(trans_remove)
		if (translation)
			string msg = Replace(translation, "{ITEMNAME}", m_item.GetName())
			if (msg)
				Debug.Notification(msg)
			endif
		endif
		m_list.RemoveAddedForm(m_item)
		DropLocationFromList(location_type, m_item)
	else
		string translation = GetTranslation(trans_add)
		if (translation)
			string msg = Replace(translation, "{ITEMNAME}", m_item.GetName())
			if (msg)
				Debug.Notification(msg)
			endif
		endif
		m_list.AddForm(m_item)
		AddLocationToList(location_type, m_item)
	endif
endFunction

function ManageUserList(Form itemForm)
	ManageList(userlist_form, itemForm, location_type_user, "$AHSE_USERLIST_ADDED", "$AHSE_USERLIST_REMOVED")
endFunction

function MoveFromExcludeToUserList(Form itemForm)
	if (excludelist_form.find(itemForm) != -1)
		int result = ShowMessage(to_list_message, "$AHSE_MOVE_TO_USERLIST", "{ITEMNAME}", itemForm.GetName())
		if (result == 0)
			excludelist_form.removeAddedForm(itemForm)
		else
			return
		endif
	endif
	ManageUserList(itemForm)
endFunction

function MoveFromUserToExcludeList(Form itemForm)
	if (userlist_form.find(itemForm) != -1)
		int result = ShowMessage(to_list_message, "$AHSE_MOVE_TO_EXCLUDELIST", "{ITEMNAME}", itemForm.GetName())
		if (result == 0)
			userlist_form.removeAddedForm(itemForm)
		else
			return
		endif
	endif
	ManageExcludeList(itemForm)
endFunction

function ManageExcludeList(Form itemForm)
	ManageList(excludelist_form, itemForm, location_type_excluded, "$AHSE_EXCLUDELIST_ADDED", "$AHSE_EXCLUDELIST_REMOVED")
endFunction

; must line up with enumerations from C++
Function SyncNativeDataTypes()
	objType_Flora = GetObjectTypebyName("flora")
	objType_Critter = GetObjectTypebyName("critter")
	objType_Septim = GetObjectTypebyName("septims")
	objType_Soulgem = GetObjectTypebyName("soulgem")
	objType_Mine = GetObjectTypebyName("oreVein")
	objType_Book = GetObjectTypebyName("books")
	objType_skillBookRead = GetObjectTypebyName("skillbookread")

	glowReasonBossContainer = 1
	glowReasonQuestObject = 2
	glowReasonEnchantedItem = 3
	glowReasonLockedContainer = 4
	glowReasonPlayerProperty = 5
	glowReasonSimpleTarget = 6

	; must line up with shaders defined in ESP/ESM file
	lootedShader = Game.GetFormFromFile(0xa9e1, "AutoHarvestSE.esp") as EffectShader	; white
	enchantedShader = Game.GetFormFromFile(0xa9dc, "AutoHarvestSE.esp") as EffectShader	; blue
	lockedShader = Game.GetFormFromFile(0xa9df, "AutoHarvestSE.esp") as EffectShader	; red
	bossShader = Game.GetFormFromFile(0xa9e2, "AutoHarvestSE.esp") as EffectShader		; gold
	questShader = Game.GetFormFromFile(0xa9de, "AutoHarvestSE.esp") as EffectShader		; purple
	ownedShader = Game.GetFormFromFile(0xa9dd, "AutoHarvestSE.esp") as EffectShader		; green
endFunction

Event OnInit()
endEvent

Event OnPlayerLoadGame()
endEvent

Function ApplySetting()

	;DebugTrace("eventScript ApplySetting start")

	UnregisterForAllKeys()
	UnregisterForMenu("Loading Menu")

	int s_pauseKey = GetConfig_Pausekey()
	if (s_pauseKey != 0)
		 RegisterForKey(s_pauseKey)
	endif
	
	int s_userlistKey = GetConfig_Userlistkey()
	if (s_userlistKey != 0)
		if (s_userlistKey != s_pauseKey)
			RegisterForKey(s_userlistKey)
		endif
	endif

	int s_excludelistKey = GetConfig_Excludelistkey()
	if (s_excludelistKey != 0)
		if (s_excludelistKey != s_userlistKey && s_excludelistKey != s_pauseKey)
			RegisterForKey(s_excludelistKey)
		endif
	endif

	g_interval = GetConfig_Interval(type_AutoHarvest)
	if (g_interval > 0)
		if (g_interval < min_interval)
			g_interval = min_interval
		EndIf
  	endIf

  	;update CACO index in load order, to handle custom ore mining
    g_CACOModIndex.SetValue(Game.GetModByName("Complete Alchemy & Cooking Overhaul.esp"))
	;DebugTrace("eventScript ApplySetting start")

	SyncUserList()
	SyncExcludeList()
	
	utility.waitMenumode(g_interval)
	RegisterForMenu("Loading Menu")
	;DebugTrace("eventScript ApplySetting finished")
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
	string str = sif(priorState == 0, "$AHSE_ENABLE", "$AHSE_DISABLE")
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
				string msg = "$AHSE_USERLIST_FORM_ERROR"
				Debug.Notification(msg)
				;DebugTrace(msg)
				return
			endif

			int result = -1
			if (GetConfig_Userlistkey() == GetConfig_Excludelistkey())
				result = ShowMessage(userlist_message, "$AHSE_REGISTER_LIST", "{ITEMNAME}", itemForm.GetName())
			elseif (keyCode == GetConfig_Userlistkey())
				result = 0
			elseif (keyCode == GetConfig_Excludelistkey())
				result = 1
			else
				return
			endif
			
			if (result == 0)
				MoveFromExcludeToUserList(itemForm)
			elseIf (result == 1)
				MoveFromUserToExcludeList(itemForm)
			EndIf
			SyncExcludeList()
			SyncUserList()
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

Event OnAutoHarvest(ObjectReference akTarget, int itemType, int count, bool silent, bool ignoreBlock, bool manualLootNotify)
	;DebugTrace("OnAutoHarvest:Run: " + akTarget.GetDisplayName() + "RefID(" +  akTarget.GetFormID() + ")  BaseID(" + akTarget.GetBaseObject().GetFormID() + ")" ) 
	;DebugTrace("item type: " + itemType + ", type-mine: " + objType_mine + ", do not notify: " + silent + ", ignore activation blocking: " + ignoreBlock) 
	;DebugTrace("notify for manual loot: " + manualLootNotify)
	Actor akActivator = Game.GetPlayer()

	if (IsBookObject(itemType))
		akActivator.AddItem(akTarget, count, true)
	elseif (itemType == objType_Soulgem && akTarget.GetLinkedRef(None))
		; no-op but must still unlock

	elseif (itemType == objType_Mine)
		;DebugTrace("Harvesting Ore")
		bool miningDone = false
		MineOreScript oreScript = akTarget as MineOreScript
		if (oreScript)
			; brute force ore gathering to bypass tedious MineOreScript/Furniture handshaking
			int available = oreScript.ResourceCountCurrent
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
			endwhile
			;DebugTrace("Ore harvested amount: " + mined + ", remaining: " + oreScript.ResourceCountCurrent)
			miningDone = true
		; CACO provides its own mining script, unfortunately not derived from baseline though largely identical
		elseif ((g_CACOModIndex.GetValue() as int) != 255)
			CACO_MineOreScript cacoMinable = akTarget as CACO_MineOreScript
			if (cacoMinable)
				DebugTrace("Detected CACO ore vein")
				; brute force ore gathering to bypass tedious MineOreScript/Furniture handshaking
				int available = cacoMinable.ResourceCountCurrent
				int mined = 0
				if (available == -1)
	       		    DebugTrace("CACO ore vein not yet initialized, start mining")
	       		else
	       		    DebugTrace("CACO ore vein has ore available: " + available)
	       		endif

				; 'available' is set to -1 before the vein is initialized - after we call giveOre the amount received is
				; in ResourceCount and the remaining amount in ResourceCountCurrent 
				while (available != 0 && mined < maxMiningItems)
	   				DebugTrace("Trigger CACO ore harvesting")
					cacoMinable.giveOre()
					mined += cacoMinable.ResourceCount
				    DebugTrace("CACO ore vein amount so far: " + mined + ", this time: " + cacoMinable.ResourceCount + ", max: " + maxMiningItems)
					available = cacoMinable.ResourceCountCurrent
				endwhile
				DebugTrace("CACO ore vein harvested amount: " + mined + ", remaining: " + oreScript.ResourceCountCurrent)
				miningDone = true
			endif
		endif

		if (!miningDone && manualLootNotify)
			; could be CACO-scripted 'Mine' target - glow as a 'nearby manual lootable' if configured to do so
			DoObjectGlow(akTarget, 5, glowReasonSimpleTarget)
		endif

    ; Blocked activators may be looted according to a config setting
	elseif (!akTarget.IsActivationBlocked() || ignoreBlock)
    	form baseForm = akTarget.GetBaseObject()
		if (akTarget.IsActivationBlocked())
			;DebugTrace("Activator blocked " + akTarget.GetDisplayName())
		endif
		if (itemType == objType_Flora)
			if (ActivateEx(akTarget, akActivator, silent) && (greenThumbPerk && akActivator.HasPerk(greenThumbPerk)) as bool)
				float greenThumbValue = greenThumbPerk.GetNthEntryValue(0, 0)
				int countPP = ((count * greenThumbValue) - count) as int
				if (countPP >= 1)
					akActivator.AddItem(baseForm, countPP, true)
				endif
			endif
		elseif (itemType == objType_Critter)
			ActivateEx(akTarget, akActivator, silent)
		elseif (itemType == objType_Septim && baseForm.GetType() == getType_kFlora)
			ActivateEx(akTarget, akActivator, silent)
		elseif (ActivateEx(akTarget, akActivator, true) && !silent)
			string activateMsg = none
			if (count >= 2)
				string translation = GetTranslation("$AHSE_ACTIVATE(COUNT)_MSG")
				
				string[] targets = New String[2]
				targets[0] = "{ITEMNAME}"
				targets[1] = "{COUNT}"

				string[] replacements = New String[2]
				replacements[0] = baseForm.GetName()
				replacements[1] = count as string
				
				activateMsg = ReplaceArray(translation, targets, replacements)
			else
				string translation = GetTranslation("$AHSE_ACTIVATE_MSG")
				activateMsg = Replace(translation, "{ITEMNAME}", baseForm.GetName())
			endif
			if (activateMsg)
				Debug.Notification(activateMsg)
			endif
		endif

		;DebugTrace("OnAutoHarvest:Activated:" + akTarget.GetDisplayName() + "RefID(" +  akTarget.GetFormID() + ")  BaseID(" + akTarget.GetBaseObject().GetFormID() + ")" )
	endif
	
	UnlockAutoHarvest(akTarget)
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
	Actor player = Game.GetPlayer()
	player.ModActorValue("CarryWeight", weightDelta as float)
	;DebugTrace("Player carry weight " + player.GetActorValue("CarryWeight") + " after applying delta " + weightDelta)
EndEvent

Function RemoveCarryWeightDelta()
	Actor player = Game.GetPlayer()
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

Event OnMenuOpen(String MenuName)
	if (MenuName == "Loading Menu")
		UnblockEverything()
	endif
endEvent
