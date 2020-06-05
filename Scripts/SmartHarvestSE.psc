scriptname SmartHarvestSE

Function DebugTrace(string str) global native
string Function GetPluginVersion() global native
string Function GetPluginName(Form thisForm) global native
string Function GetTextFormID(Form thisForm) global native
string Function GetTextObjectType(Form thisForm) global native

bool Function UnlockHarvest(ObjectReference refr, bool isSilent) global native
Function UnblockEverything() global native
Form Function GetNthLootableObject(ObjectReference refr, int index) global native
Function ClearLootableObjects(ObjectReference refr) global native

string Function GetObjectTypeNameByType(int num) global native
int Function GetObjectTypeByName(string name) global native
int Function GetResourceTypeByName(string name) global native
float Function GetSetting(int m_section_first, int m_section_second, string m_key) global native
float Function GetSettingToObjectArrayEntry(int m_section_first, int m_section_second, int index) global native
Function PutSetting(int m_section_first, int m_section_second, string m_key, float m_value) global native
Function PutSettingObjectArray(int m_section_first, int m_section_second, float[] m_values) global native

bool Function Reconfigure() global native
Function LoadIniFile() global native
Function SaveIniFile() global native

Function SetIngredientForCritter(Form critter, Form ingredient) global native

Function AllowSearch() global native
Function DisallowSearch() global native
bool Function IsSearchAllowed() global native

Function SyncWhiteListWithPlugin() global native
bool Function SaveWhiteList() global native
bool Function LoadWhiteList() global native
Function ClearPluginBlackList() global native
Function MergePluginBlackList() global native
bool Function SaveBlackList() global native
bool Function LoadBlackList() global native
String Function PrintFormID(int formID) global native

Function AddLocationToList(int locationType, Form location) global native
Function DropLocationFromList(int locationType, Form location) global native

String Function GetTranslation(String key) global native
String Function Replace(String str, String target, String replacement) global native
String Function ReplaceArray(String str, String[] targets, String[] replacements) global native

int location_type_whitelist = 1
int location_type_blacklist = 2

Formlist whitelist_form
Formlist blacklist_form

int Function GetConfig_Pausekey() global
	int type1_Common = 1
	int type2_Config = 1
	return GetSetting(type1_Common, type2_Config, "pauseHotkeycode") as int
endFunction

int Function GetConfig_WhiteListKey() global
	int type1_Common = 1
	int type2_Config = 1
	return GetSetting(type1_Common, type2_Config, "whiteListHotkeycode") as int
endFunction

int Function GetConfig_BlackListKey() global
	int type1_Common = 1
	int type2_Config = 1
	return GetSetting(type1_Common, type2_Config, "blackListHotkeycode") as int
endFunction

string Function GetNameForListForm(Form locationOrCell) global
	string name = locationOrCell.GetName()
	if (StringUtil.GetLength(name) == 0)
		name = "Cell " + PrintFormID(locationOrCell.GetFormID())
	endif
	return name
EndFunction

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

string Function sif (bool cc, string aa, string bb) global
	string result
	if (cc)
		result = aa
	else
		result = bb
	endif
	return result
endFunction
