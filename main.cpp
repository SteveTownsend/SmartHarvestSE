#include "PrecompiledHeaders.h"

#include "tasks.h"
#include "PlayerCellHelper.h"
#include "version.h"

#include <shlobj.h>
#include <sstream>
#include <KnownFolders.h>

void SKSEMessageHandler(SKSE::MessagingInterface::Message* msg)
{
	static bool scanOK(true);
	switch (msg->type)
	{
	case SKSE::MessagingInterface::kDataLoaded:
		DBG_MESSAGE("Loading Papyrus");
		SKSE::GetPapyrusInterface()->Register(papyrus::RegisterFuncs);
		DBG_MESSAGE("Loaded Papyrus");
		break;

	case SKSE::MessagingInterface::kPreLoadGame:
		DBG_MESSAGE("Game load starting, disable looting");
		scanOK = SearchTask::IsAllowed();
		SearchTask::PrepareForReload();
		break;

	case SKSE::MessagingInterface::kNewGame:
	case SKSE::MessagingInterface::kPostLoadGame:
		DBG_MESSAGE("Game load done, initializing Tasks");
		// if checks fail, abort scanning
		if (!SearchTask::Init())
			return;
		DBG_MESSAGE("Initialized Tasks, restart looting if allowed");
		if (scanOK)
		{
			SearchTask::Allow();
		}
		break;
	}
}

extern "C"
{

bool SKSEPlugin_Query(const SKSE::QueryInterface * a_skse, SKSE::PluginInfo * a_info)
{
	std::wostringstream path;
	path << L"/My Games/Skyrim Special Edition/SKSE/" << std::wstring(L_SHSE_NAME) << L".log";
	std::wstring wLogPath(path.str());
	SKSE::Logger::OpenRelative(FOLDERID_Documents, wLogPath);
	SKSE::Logger::SetPrintLevel(SKSE::Logger::Level::kDebugMessage);
	SKSE::Logger::SetFlushLevel(SKSE::Logger::Level::kDebugMessage);
	SKSE::Logger::UseLogStamp(true);
	SKSE::Logger::UseTimeStamp(true);
	SKSE::Logger::UseThreadID(true);
#if _DEBUG
	SKSE::Logger::HookPapyrusLog(true);
#endif
	REL_MESSAGE("%s v%s", SHSE_NAME, VersionInfo::Instance().GetPluginVersionString().c_str());

	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = SHSE_NAME;
	a_info->version = VersionInfo::Instance().GetVersionMajor();

	if (a_skse->IsEditor()) {
		REL_FATALERROR("Loaded in editor, marking as incompatible!\n");
		return false;
	}
	SKSE::Version runtimeVer(a_skse->RuntimeVersion());
	if (runtimeVer < SKSE::RUNTIME_1_5_73)
	{
		REL_FATALERROR("Unsupported runtime version %08x!\n", runtimeVer);
		return false;
	}

	// print loaded addresses of key functions for debugging
	DBG_MESSAGE("*** Function addresses START");
	utils::LogFunctionAddress(&SearchTask::DoPeriodicSearch, "SearchTask::DoPeriodicSearch");
	utils::LogFunctionAddress(&SearchTask::Run, "SearchTask::Run");
	utils::LogFunctionAddress(&PlayerCellHelper::GetReferences, "PlayerCellHelper::GetReferences");
	DBG_MESSAGE("*** Function addresses END");

	return true;
}

bool SKSEPlugin_Load(const SKSE::LoadInterface * skse)
{
	REL_MESSAGE("%s plugin loaded", SHSE_NAME);
	if (!SKSE::Init(skse)) {
		return false;
	}
	SKSE::GetMessagingInterface()->RegisterListener("SKSE", SKSEMessageHandler);
	SKSE::GetSerializationInterface()->SetUniqueID('SHSE');

	return true;
}

};
