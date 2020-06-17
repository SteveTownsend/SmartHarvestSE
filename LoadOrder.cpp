#include "PrecompiledHeaders.h"

std::unique_ptr<LoadOrder> LoadOrder::m_instance;

LoadOrder& LoadOrder::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<LoadOrder>();
	}
	return *m_instance;
}

bool LoadOrder::Analyze(void)
{
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	if (!dhnd)
		return false;
	static const std::string oldName(PRIORNAME);
	// Process each ESP and ESPFE/ESL in the master list of RE::TESFile / loaded mod instances
	for (const auto modFile : dhnd->files)
	{
		// Make sure the earlier version of the mod is not installed
		if (oldName.compare(0, oldName.length(), &modFile->fileName[0]) == 0)
		{
			REL_ERROR("Prior mod plugin version (%s) is incompatible with current plugin (%s)", oldName.c_str(), &MODNAME[0]);
			return false;
		}

		// validation logic from CommonLibSSE 
		if (modFile->compileIndex == 0xFF)
		{
			REL_MESSAGE("%s skipped, has load index 0xFF", modFile->fileName);
			continue;
		}

		RE::FormID formIDMask(modFile->compileIndex << (3 * 8));
		formIDMask += modFile->smallFileCompileIndex << ((1 * 8) + 4);

		m_formIDMaskByName.insert(std::make_pair(modFile->fileName, formIDMask));
		REL_MESSAGE("%s has FormID mask 0x%08x", modFile->fileName, formIDMask);
	}
	return true;
}


// returns zero if mod not loaded
RE::FormID LoadOrder::GetFormIDMask(const std::string& modName) const
{
	const auto& matched(m_formIDMaskByName.find(modName));
	if (matched != m_formIDMaskByName.cend())
	{
		return matched->second;
	}
	return InvalidForm;
}

// returns true iff mod loaded
bool LoadOrder::IncludesMod(const std::string& modName) const
{
	return m_formIDMaskByName.find(modName) != m_formIDMaskByName.cend();
}

