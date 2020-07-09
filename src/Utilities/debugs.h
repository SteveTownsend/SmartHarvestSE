#pragma once

namespace RE
{
	class ExtraDataList;
}

namespace shse
{

class LootableREFR;

void DumpExtraData(const RE::ExtraDataList * extraData);
void DumpReference(const LootableREFR& refr, const char * typeName, const INIFile::SecondaryType scope);
void DumpContainer(const LootableREFR& ref);

}
