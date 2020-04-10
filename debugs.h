#pragma once

class TESObjectREFRHelper;

namespace RE
{
	class ExtraDataList;
}

void DumpExtraData(const RE::ExtraDataList * extraData);
void DumpReference(const TESObjectREFRHelper& refr, const char * typeName);
void DumpContainer(const TESObjectREFRHelper& ref);
