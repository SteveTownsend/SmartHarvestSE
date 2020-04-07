#pragma once

#define MAKE_STR_HELPER(a_str) #a_str
#define MAKE_STR(a_str) MAKE_STR_HELPER(a_str)

#define AHSE_VERSION_MAJOR	2
#define AHSE_VERSION_MINOR	0
#define AHSE_VERSION_PATCH	0
#define AHSE_VERSION_BETA	0
#define AHSE_VERSION_VERSTRING	MAKE_STR(AHSE_VERSION_MAJOR) "." MAKE_STR(AHSE_VERSION_MINOR) "." MAKE_STR(AHSE_VERSION_PATCH) "." MAKE_STR(AHSE_VERSION_BETA)

constexpr char* AHSE_NAME = "AutoHarvestSE";
constexpr wchar_t* L_AHSE_NAME = L"AutoHarvestSE";
constexpr char* MODNAME = "AutoHarvestSE.esp";
