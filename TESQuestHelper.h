#pragma once
#include "CommonLibSSE/include/RE/TESQuest.h"

class TESQuestHelper
{
public:
	TESQuestHelper(RE::TESQuest* quest) : m_quest(quest) {}
	RE::VMHandle GetAliasHandle(UInt32 index);

private:
	RE::TESQuest* m_quest;
};

RE::TESQuest* GetTargetQuest(const char* espName, UInt32 questID);
