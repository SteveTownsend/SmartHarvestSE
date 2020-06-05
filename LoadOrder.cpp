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

void LoadOrder::Analyze(void)
{
	RE::TESDataHandler* dhnd = RE::TESDataHandler::GetSingleton();
	if (!dhnd)
		return;
	// Process each ESP and ESPFE/ESL in the master list of RE::TESFile / loaded mod instances
	for (const auto modFile : dhnd->files)
	{
		// validation logic from CommonLibSSE 
		if (modFile->compileIndex == 0xFF)
		{
#if _DEBUG
			_MESSAGE(" %s skipped, has load index 0xFF", modFile->fileName);
#endif
			continue;
		}

		RE::FormID formIDMask(modFile->compileIndex << (3 * 8));
		formIDMask += modFile->smallFileCompileIndex << ((1 * 8) + 4);

		m_formIDMaskByName.insert(std::make_pair(modFile->fileName, formIDMask));
#if _DEBUG
		_MESSAGE(" %s has FormID mask 0x%08x", modFile->fileName, formIDMask);
#endif
	}
}
