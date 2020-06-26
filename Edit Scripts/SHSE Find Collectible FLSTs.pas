{
  Identifies FormLists with concrete collectible objects.
}
unit UserScript;

var
  formLists: TStringList;
  legalTypes: TStringList;
  collectionPreamble: string;
  collectionTemplate: string;
  collectionPostscript: string;

function Initialize: integer;
begin
  legalTypes := TStringList.Create;
  legalTypes.Add('ALCH');
  legalTypes.Add('ARMO');
  legalTypes.Add('BOOK');
  legalTypes.Add('INGR');
  legalTypes.Add('KEYM');
  legalTypes.Add('MISC');
  legalTypes.Add('SLGM');
  legalTypes.Add('WEAP');

  collectionPreamble := '{ "$comment": "Definitions are allowed for up to 128 rulesets per file for Collections", "$schema": "./Schema.json", "collections": [ ';
  collectionTemplate := '{ "name": "{NAME}", "description": "{DESCRIPTION}", "policy": { "action": "take", "notify": true, "repeat": true }, "rootFilter": { "operator": "AND", "condition": { "formList": { "listPlugin": "{PLUGIN}" , "formID": "{FORMID}" } } } },';
  collectionPostscript := ' ] }';

  formLists := TStringList.Create;
  formLists.Add(collectionPreamble);
end;

function DoStringReplace(const Input, Find, Replace : String) : String;
var
  P : Integer;
begin
  Result := Input;

  repeat
    P := Pos(Find, Result);
    if P > 0 then begin
      Delete(Result, P, Length(Find));
      Insert(Replace, Result, P); 
    end;
  until P = 0;
end;

// adds separator before each instance of final consecutive capital letter
function MakeName(const Input : String) : String;
var
  P, inserted, wordLength : Integer;
  lastWasCap : Boolean;
  SEPARATOR : String;
begin
  Result := Input;
  SEPARATOR := ' ';
  lastWasCap := false;
  inserted := 0;
  wordLength := 0;
  for P := 1 to Length(Input) do begin
    if Input[P] = Upcase(Input[P]) then begin
	  if not lastWasCap and wordLength > 0 then begin
	    Insert(SEPARATOR, Result, P + inserted);
		inserted := inserted + 1;
		wordLength := 0;
	  end;
      wordLength := wordLength + 1;
	  lastWasCap := True;
	end else begin
	  // not a capital letter - could be end of a sequence of caps
	  if lastWasCap and wordLength > 1 then begin
	    Insert(SEPARATOR, Result, P + inserted - 1);
		inserted := inserted + 1;
		wordLength := 0;
	  end;
      wordLength := wordLength + 1;
	  lastWasCap := False;
	end;
  end;
end;

function EncodeAsCollection(e: IInterface; edid, rawName: string): string;
begin
	result := DoStringReplace(collectionTemplate, '{NAME}', 'LoTD ' + MakeName(rawName));
	result := DoStringReplace(result, '{DESCRIPTION}', 'LoTD Display: ' + edid);
	result := DoStringReplace(result, '{PLUGIN}', GetFileName(e));
	result := DoStringReplace(result, '{FORMID}', IntToHex(GetLoadOrderFormID(e), 8));
end;

function Process(e: IInterface): integer;
var
  edid, rawName: string;
begin
  if Signature(e) <> 'FLST' then
    Exit;
  // Whitelist LoTD displays
  edid := GetEditValue(ElementBySignature(e, 'EDID'));
  
  if (pos('DBM_Section', edid) = 1) and (pos('Items', edid) = (Length(edid) - Length('Items') + 1))then begin
    AddMessage('LoTD Display ' + edid);
	rawName := DoStringReplace(edid, 'DBM_Section', '');
	rawName := DoStringReplace(rawName, 'Items', '');
	formLists.Add(EncodeAsCollection(e, edid, rawName));
  end;
end;

function Finalize: integer;
var
  fname: string;
begin
  fname := ProgramPath + 'Edit Scripts\CollectionDefinitions.putnamehere.json';
  AddMessage('Saving list to ' + fname);
  formLists.Add(collectionPostscript);
  formLists.SaveToFile(fname);
  formLists.Free;
end;

end.
