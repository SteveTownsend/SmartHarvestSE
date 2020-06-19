scriptname SHSE_PluginProxy

Function DebugTrace(string str) global native
string Function GetPluginVersion() global native
string Function GetPluginName(Form thisForm) global native
string Function GetTextObjectType(Form thisForm) global native

bool Function UnlockHarvest(ObjectReference refr, bool isSilent) global native
Function BlockFirehose(ObjectReference refr)  global native

string Function GetObjectTypeNameByType(int num) global native
int Function GetObjectTypeByName(string name) global native
int Function GetResourceTypeByName(string name) global native
float Function GetSetting(int m_section_first, int m_section_second, string m_key) global native
float Function GetSettingObjectArrayEntry(int m_section_first, int m_section_second, int index) global native
Function PutSetting(int m_section_first, int m_section_second, string m_key, float m_value) global native
Function PutSettingObjectArrayEntry(int section_first, int section_second, int index, float value) global native

bool Function Reconfigure() global native
Function LoadIniFile() global native
Function SaveIniFile() global native

Function SetIngredientForCritter(Form critter, Form ingredient) global native

Function AllowSearch() global native
Function DisallowSearch() global native
bool Function IsSearchAllowed() global native
Function ReportOKToScan(bool goodToGo, int nonce) global native

Function ResetList(bool reload, int listNum) global native
Function AddEntryToList(int entryType, Form entry) global native
Function SyncDone() global native
String Function PrintFormID(int formID) global native

String Function GetTranslation(String key) global native
String Function Replace(String str, String target, String replacement) global native
String Function ReplaceArray(String str, String[] targets, String[] replacements) global native

;Collection Management
bool Function CollectionsInUse() global native
Function FlushAddedItems(int[] formIDs, int[] objectTypes, int itemCount) global native
Function ToggleCalibration() global native
Form Function GetPlayerPlace() global native

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
