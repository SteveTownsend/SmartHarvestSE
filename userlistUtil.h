#pragma once

#include "skse64/GameObjects.h"
#include <vector>

//#define USERLIST_FILE	"data\\SKSE\\Plugins\\userlist.tsv"
#define USERLIST_FILE	"AutoHarvestSE\\userlist.tsv"

int UserlistSave(void);
int UserlistLoad(void);

TESForm* GetLineForm(const std::string &str, char delim);
std::vector<std::string> Split(const std::string &str, char delim);
UInt32 atoul(const char* chr);
std::string ToStringID(UInt32 id);
