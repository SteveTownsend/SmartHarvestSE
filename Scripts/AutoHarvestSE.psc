scriptname AutoHarvestSE

Function DebugTrace(string str) global native
string Function GetPluginName(Form thisForm) global native
string Function GetTextFormID(Form thisForm) global native
string Function GetTextObjectType(Form thisForm) global native

int Function GetCloseReferences(int type) global native
bool Function UnlockAutoHarvest(ObjectReference refr) global native
bool Function UnlockPossiblePlayerHouse(Form currentLocation) global native
bool Function BlockReference(ObjectReference refr) global native
Function UnblockEverything() global native
Form Function GetNthLootableObject(ObjectReference refr, int index) global native
Function ClearLootableObjects(ObjectReference refr) global native

;Function PlayPickupSound(form baseForm) global native

string Function GetObjectKeyString(int num) global native
float Function GetSetting(int m_section_first, int m_section_second, string m_key) global native
Function GetSettingToObjectArray(int m_section_first, int m_section_second, float[] m_values) global native
Function PutSetting(int m_section_first, int m_section_second, string m_key, float m_value) global native
Function PutSettingObjectArray(int m_section_first, int m_section_second, float[] m_values) global native

bool Function Reconfigure() global native
Function LoadIniFile() global native
Function SaveIniFile() global native

Function SetIngredientForCritter(Form critter, Form ingredient) global native

Function AllowSearch() global native
Function DisallowSearch() global native
bool Function IsSearchAllowed() global native

Function SyncUserlist() global native
bool Function SaveUserlist() global native
bool Function LoadUserlist() global native
Function SyncExcludelist() global native
bool Function SaveExcludelist() global native
bool Function LoadExcludelist() global native

Function AddLocationToList(int locationType, Form location) global native
Function DropLocationFromList(int locationType, Form location) global native

String Function GetTranslation(String key) global native
String Function Replace(String str, String target, String replacement) global native
String Function ReplaceArray(String str, String[] targets, String[] replacements) global native

bool Function IsBookObject(int type) global
	return (type >= 17 && type <= 22)
endFunction

int Function GetConfig_Pausekey() global
	int type1_Common = 1
	int type2_Config = 1
	return GetSetting(type1_Common, type2_Config, "pauseHotkeycode") as int
endFunction

int Function GetConfig_Userlistkey() global
	int type1_Common = 1
	int type2_Config = 1
	return GetSetting(type1_Common, type2_Config, "userlistHotkeycode") as int
endFunction

int Function GetConfig_Excludelistkey() global
	int type1_Common = 1
	int type2_Config = 1
	return GetSetting(type1_Common, type2_Config, "excludelistHotkeycode") as int
endFunction

float Function GetConfig_Interval(int section) global
	int type1_Common = 1
	int type2_Config = 1
	return GetSetting(section, type2_Config, "IntervalSeconds")
endFunction

bool Function ActivateEx(ObjectReference akTarget, ObjectReference akActivator, bool isSilentMessage = false) global
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

form Function GetSelectedItemForm(string menuName) global
	int formID = UI.GetInt(menuName, "_root.Menu_mc.inventoryLists.itemList.selectedEntry.formId")
	if (formID == 0x0)
		return none
	endif
	
	Form itemForm = Game.GetFormEx(formID)
	if (!itemForm)
		return none
	endif
	return itemForm
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

string Function sif (bool cc, string aa, string bb) global
	string result
	if (cc)
		result = aa
	else
		result = bb
	endif
	return result
endFunction
