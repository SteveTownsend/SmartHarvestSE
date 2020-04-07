Scriptname AutoHarvestSE_Events_Alias extends ReferenceAlias  

import AutoHarvestSE
GlobalVariable Property g_EnableLaunched Auto

Keyword property CastKwd auto

Message property userlist_message auto
Formlist property userlist_form auto
Formlist property excludelist_form auto
Message property house_check_message auto
Message property to_list_message auto
Container Property list_nametag auto
Perk Property greenThumbPerk auto

int type_Common = 1
int type_AutoHarvest = 2
int type_Spell = 3
int objType_Flora = 1
int objType_Critter = 2
int objType_Septim = 4
int objType_Soulgem = 10
int objType_Mine = 34
int getType_kFlora = 39

float g_interval = 0.5
float min_interval = 0.1

int location_type_user = 1
int location_type_excluded = 2

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
		 (s_pauseKey)
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
	
	SyncUserlist()
	SyncExcludelist()
	
	utility.waitMenumode(g_interval)

    int existingFlags = g_EnableLaunched.GetValue() as int
	bool isEnabled = existingFlags == 1 || existingFlags == 3
	string str = sif(isEnabled, "$AHSE_ENABLE", "$AHSE_DISABLE")
	Debug.Notification(str)
	; reset blocked lists to allow setting updates to referesh their state
	UnblockEverything()
	if (isEnabled)
		AllowSearch()
	Else
		DisallowSearch()
	EndIf
	
	RegisterForMenu("Loading Menu")
	;DebugTrace("eventScript ApplySetting finished")
endFunction

;hotkey changes sense of looting
function Pause()
	string s_enableStr = none
    int existingFlags = g_EnableLaunched.GetValue() as int
	bool isEnabled = existingFlags != 1 && existingFlags != 3
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
		
	string str = sif(isEnabled, "$AHSE_ENABLE", "$AHSE_DISABLE")
	if (isEnabled)
		AllowSearch()
	Else
		DisallowSearch()
	EndIf
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
				if (excludelist_form.find(itemForm) != -1)
					result = ShowMessage(to_list_message, "$AHSE_MOVE_TO_USERLIST", "{ITEMNAME}", itemForm.GetName())
					if (result == 0)
						excludelist_form.removeAddedForm(itemForm)
					else
						return
					endif
				endif
				ManageList(userlist_form, itemForm, location_type_user, "$AHSE_USERLIST_ADDED", "$AHSE_USERLIST_REMOVED")
			elseIf (result == 1)
				if (userlist_form.find(itemForm) != -1)
					result = ShowMessage(to_list_message, "$AHSE_MOVE_TO_EXCLUDELIST", "{ITEMNAME}", itemForm.GetName())
					if (result == 0)
						userlist_form.removeAddedForm(itemForm)
					else
						return
					endif
				endif
				ManageList(excludelist_form, itemForm, location_type_excluded, "$AHSE_EXCLUDELIST_ADDED", "$AHSE_EXCLUDELIST_REMOVED")
			EndIf
			SyncExcludelist()
			SyncUserlist()
		endif
	endif
endEvent


bool Function IsLocationInExcludelist()
	bool result = false
	form locForm = Game.GetPlayer().GetCurrentLocation() as form
	form cellForm = Game.GetPlayer().GetParentCell() as form
	if (locForm && excludelist_form.Find(locForm) != -1)
		result = true
	elseif (cellForm && excludelist_form.Find(locForm) != -1)
		result = true
	endif
	return result
EndFunction

; C++ code ensures that this will only ever be invoked once for a given location
Event OnPlayerHouseCheck(Form currentLocation)
	int result = ShowMessage(house_check_message, "$AHSE_HOUSE_CHECK")
	;DebugTrace("player house showmessage " + result)
	if (result == 0 && !excludelist_form.HasForm(currentLocation))
    	;DebugTrace("player house added to excluded list")
		ManageList(excludelist_form, currentLocation, location_type_excluded, "$AHSE_EXCLUDELIST_ADDED", "$AHSE_EXCLUDELIST_REMOVED")
	endif
	;DebugTrace("player house request unlocked")
	UnlockPossiblePlayerHouse(currentLocation)
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

Event OnAutoHarvest(ObjectReference akTarget, int itemType, int count, bool silent)
	;DebugTrace("OnAutoHarvest:Run: " + akTarget.GetDisplayName() + "RefID(" +  akTarget.GetFormID() + ")  BaseID(" + akTarget.GetBaseObject().GetFormID() + ")" ) 
	
	Actor akActivator = Game.GetPlayer()
	form baseForm = akTarget.GetBaseObject()

	if (IsBookObject(itemType))
		if (!IsLocationInExcludelist())
			akActivator.AddItem(akTarget, count, true)
		endif
	elseif (itemType == objType_Soulgem && akTarget.GetLinkedRef(None))
		; no-op but must still unlock
	elseif (itemType == objType_Mine)
		;DebugTrace("Harvesting Ore")
		if (!IsLocationInExcludelist())
    		;DebugTrace("Ore location valid")
			MineOreScript oreScript = akTarget as MineOreScript
			if (oreScript)
				; brute force ore gathering to bypass tedious MineOreScript/Furniture handshaking
				int remaining = oreScript.ResourceCountCurrent
				if (remaining == -1)
        		    ;DebugTrace("Vein not yet initialized, start mining")
        		else
        		    ;DebugTrace("Vein has ore available: " + remaining)
        		endif

				int harvested = 0
				; 'remaining' is set to -1 before the vein is initialized
				while (remaining != 0 && harvested < count)
    				;DebugTrace("Trigger harvesting")
					oreScript.giveOre()
					harvested = harvested + 1
					remaining = oreScript.ResourceCountCurrent
					;DebugTrace("Ore harvested, amount remaining: " + remaining)
				endwhile
				;DebugTrace("Done harvesting")
			endif
		endif	
		; don't try to re-harvest excluded, depleted or malformed vein again until we revisit the cell
		BlockReference(akTarget)

	elseif (!akTarget.IsActivationBlocked())
		if (!IsLocationInExcludelist())
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
			else
				if (ActivateEx(akTarget, akActivator, false) && !silent)
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
			endif
		endif

		;DebugTrace("OnAutoHarvest:Activated:" + akTarget.GetDisplayName() + "RefID(" +  akTarget.GetFormID() + ")  BaseID(" + akTarget.GetBaseObject().GetFormID() + ")" )
	endif
	
	UnlockAutoHarvest(akTarget)
endEvent

Event OnGetCritterIngredient(ObjectReference akTarget)
	;DebugTrace("OnGetCritterIngredient " + akTarget.GetDisplayName() + "RefID(" +  akTarget.GetFormID() + ")  BaseID(" + akTarget.GetBaseObject().GetFormID() + ")" )
	form baseForm = akTarget.GetBaseObject()
    Critter thisCritter = akTarget as Critter
	if (thisCritter)
       	;DebugTrace("setting ingredient " + thisCritter.lootable.GetName() + " for critter " + baseForm.GetName())
       	SetIngredientForCritter(baseForm, thisCritter.lootable)
    endif
endEvent

Event OnObjectGlow(ObjectReference akTargetRef, int duration)
	;DebugTrace("------------ Event OnObjectGlow " + akTargetRef.GetDisplayName())

	if (!IsLocationInExcludelist())
		EffectShader effShader = Game.GetFormFromFile(0x04000, "AutoHarvestSE.esp") as EffectShader
		if (effShader)
			; play forever - C++ code will tidy up when out of range
			effShader.Play(akTargetRef, duration)
		endif
	endif
endEvent

Event OnObjectGlowStop(ObjectReference akTargetRef)
	;DebugTrace("------------ Event OnObjectGlowStop " + akTargetRef.GetDisplayName())

	if (!IsLocationInExcludelist())
		EffectShader effShader = Game.GetFormFromFile(0x04000, "AutoHarvestSE.esp") as EffectShader
		if (effShader)
			effShader.Stop(akTargetRef)
		endif
	endif
endEvent

Event OnMenuOpen(String MenuName)
	if (MenuName == "Loading Menu")
		UnblockEverything()
	endif
endEvent

;waste
string sAddItemtoInventory
