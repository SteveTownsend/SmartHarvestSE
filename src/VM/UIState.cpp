/*************************************************************************
SmartHarvest SE
Copyright (c) Steve Townsend 2020

>>> SOURCE LICENSE >>>
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation (www.fsf.org); either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License is available at
http://www.fsf.org/licensing/licenses
>>> END OF LICENSE >>>
*************************************************************************/
#include "PrecompiledHeaders.h"


#include "PluginFacade.h"
#include "VM/UIState.h"
#include "Looting/ScanGovernor.h"
#include "Utilities/utils.h"

namespace shse
{

std::unique_ptr<UIState> UIState::m_instance;

UIState& UIState::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<UIState>();
	}
	return *m_instance;
}

UIState::UIState() : m_nonce(0), m_vmResponded(false), m_uiDelayed(false)
{
}

// Check VM is not handling UI - blocks if so, to avoid problems in-game and at game shutdown
void UIState::WaitUntilVMGoodToGo()
{
	const auto startTime(std::chrono::high_resolution_clock::now());
	++m_nonce;
	int nonce(m_nonce);
	DBG_MESSAGE("UI status request # {}", nonce);
	EventPublisher::Instance().TriggerCheckOKToScan(nonce);

	// wait for async result from script
	std::unique_lock<std::mutex> guard(m_uiLock);
	m_vmResponded = false;
	m_uiDelayed = false;
	m_uiReport.wait(guard, [&] { return m_vmResponded; });
	const auto endTime(std::chrono::high_resolution_clock::now());
	DBG_MESSAGE("Script reported UI ready for request {} after {} microseconds",
		nonce, std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count());
	if (m_uiDelayed)
	{
		REL_MESSAGE("UI/controls were active, delay scan");
		WindowsUtils::TakeNap(OnUIClosedThreadDelaySeconds);
	}
	REL_MESSAGE("Scan progressing");
}

void UIState::ReportVMGoodToGo(const bool delayed, const int nonce)
{
	m_vmResponded = true;
	if (nonce != m_nonce)
	{
		// likely suspended request from saved game after reload
		REL_WARNING("VM Good to Go for request {}, expected request {}", nonce, m_nonce);
	}
	else
	{
		DBG_MESSAGE("VM Good to Go for request {}", nonce);
		// response matches pending request - update the status and release waiter
	}
	if (delayed)
	{
		// OK to continue after blocking check
		m_uiDelayed = true;
	}
	m_uiReport.notify_one();
}

void UIState::Reset()
{
	m_nonce = 0;
	m_vmResponded = false;
	m_uiDelayed = false;
}

}
