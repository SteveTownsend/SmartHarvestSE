#include "PrecompiledHeaders.h"

#include "papyrus.h"
#include "tasks.h"
#include "version.h"

#include <shlobj.h>
#include <sstream>
#include <KnownFolders.h>

void SKSEMessageHandler(SKSE::MessagingInterface::Message* msg)
{
	switch (msg->type)
	{
	case SKSE::MessagingInterface::kDataLoaded:
#if _DEBUG
		_MESSAGE("Loading Papyrus");
#endif
		SKSE::GetPapyrusInterface()->Register(papyrus::RegisterFuncs);
#if _DEBUG
		_MESSAGE("Loaded Papyrus");
#endif
		break;

	case SKSE::MessagingInterface::kNewGame:
	case SKSE::MessagingInterface::kPostLoadGame:
#if _DEBUG
		_MESSAGE("Initializing Tasks");
#endif
		tasks::Init();
#if _DEBUG
		_MESSAGE("Initialized Tasks");
#endif
		break;
	}
}

extern "C"
{

bool SKSEPlugin_Query(const SKSE::QueryInterface * a_skse, SKSE::PluginInfo * a_info)
{
	std::wostringstream path;
	path << std::wstring(L"\\My Games\\Skyrim Special Edition\\SKSE\\") << std::wstring(L_AHSE_NAME) << std::wstring(L".log");
	SKSE::Logger::OpenRelative(FOLDERID_Documents, path.str());
	SKSE::Logger::SetPrintLevel(SKSE::Logger::Level::kDebugMessage);
	SKSE::Logger::SetFlushLevel(SKSE::Logger::Level::kDebugMessage);
	SKSE::Logger::UseLogStamp(true);
	SKSE::Logger::UseThreadId(true);

#if _DEBUG
	_MESSAGE("%s v%s", AHSE_NAME, AHSE_VERSION_VERSTRING);
#endif

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = AHSE_NAME;
	a_info->version = AHSE_VERSION_MAJOR;

	if (a_skse->IsEditor()) {
		_FATALERROR("Loaded in editor, marking as incompatible!\n");
		return false;
	}
	SKSE::Version runtimeVer(a_skse->RuntimeVersion());
	if (runtimeVer < SKSE::RUNTIME_1_5_73)
	{
		_FATALERROR("Unsupported runtime version %08X!\n", runtimeVer);
		return false;
	}

	return true;
}

bool SKSEPlugin_Load(const SKSE::LoadInterface * skse)
{
#if _DEBUG
	_MESSAGE("%s plugin loaded", AHSE_NAME);
#endif

	if (!SKSE::Init(skse)) {
		return false;
	}
	SKSE::GetMessagingInterface()->RegisterListener("SKSE", SKSEMessageHandler);

	return true;
}

};
