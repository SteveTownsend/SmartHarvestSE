#pragma once

class TESObjectREFRHelper;

namespace RE
{
	class ExtraDataList;
}

void DumpExtraData(const RE::ExtraDataList * extraData);
void DumpReference(const TESObjectREFRHelper& refr, const char * typeName, const INIFile::SecondaryType scope);
void DumpContainer(const TESObjectREFRHelper& ref, const INIFile::SecondaryType scope);
