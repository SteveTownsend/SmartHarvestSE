#pragma once

class LootableREFR;

namespace RE
{
	class ExtraDataList;
}

void DumpExtraData(const RE::ExtraDataList * extraData);
void DumpReference(const LootableREFR& refr, const char * typeName, const INIFile::SecondaryType scope);
void DumpContainer(const LootableREFR& ref);
