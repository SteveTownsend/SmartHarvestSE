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

UIState::UIState() : m_nonce(0), m_vmResponded(false), m_uiDelayed(false), m_waiting(false), m_mcmOpen(false)
{
}

bool UIState::OKToActivate() const
{
	if (!RE::ControlMap::GetSingleton()->IsActivateControlsEnabled())
	{
		DBG_VMESSAGE("IsActivateControlsEnabled false");
		return false;
	}
	return true;
}

ScanStatus UIState::OKToScan() const
{
	if (m_mcmOpen)
	{
		DBG_VMESSAGE("MCM is Open");
		return ScanStatus::MCMOpen;
	}
	if (RE::UI::GetSingleton()->GameIsPaused())
	{
		DBG_VMESSAGE("GameIsPaused true");
		return ScanStatus::GamePaused;
	}
	return ScanStatus::GoodToGo;
}

// Check VM is not handling UI - blocks if so, to avoid problems in-game and at game shutdown
// Returns True iff there was a UI-induced delay
void UIState::WaitUntilVMGoodToGo()
{
#ifdef _PROFILING
	WindowsUtils::ScopedTimer elapsed("VM Status check");
#endif
	// perform UI state checking here, only fire handshake with scripts if UI is open - do not check activate availability here
	ScanStatus status(OKToScan());
	if (status == ScanStatus::GoodToGo)
		return;

	const auto startTime(std::chrono::high_resolution_clock::now());
	++m_nonce;
	int nonce(m_nonce);
	REL_MESSAGE("UI status request # {} for scan status {}", nonce, status);
	EventPublisher::Instance().TriggerCheckOKToScan(nonce);

	std::unique_lock<std::mutex> guard(m_uiLock);
	m_vmResponded = false;
	m_uiDelayed = false;
	m_waiting = true;
	m_uiReport.wait(guard, [&] { return m_vmResponded; });
	m_waiting = false;

	const auto endTime(std::chrono::high_resolution_clock::now());
	REL_MESSAGE("Script reported UI ready for request {} after {} microseconds",
		nonce, std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count());
	if (m_nonce == 0)
	{
		REL_MESSAGE("Game load during UI poll, delay scan until synced");
		while (!PluginFacade::Instance().IsSynced())
		{
			WindowsUtils::TakeNap(OnUIClosedThreadDelaySeconds);
		}
		REL_MESSAGE("Scan progressing after game load");
	}
	else if (m_uiDelayed)
	{
		REL_MESSAGE("UI/controls were active, delay scan");
		WindowsUtils::TakeNap(OnUIClosedThreadDelaySeconds);
		REL_MESSAGE("Scan progressing");
	}
}

void UIState::ReportVMGoodToGo(const bool delayed, const int nonce)
{
	m_vmResponded = true;
	if (nonce != m_nonce)
	{
		// likely suspended request from saved game after reload (nonce -1)
		REL_WARNING("VM Good to Go for request {}, expected request {}", nonce, m_nonce);
	}
	else
	{
		REL_MESSAGE("VM Good to Go for request {}", nonce);
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
	int nonce = m_nonce;
	m_nonce = 0;
	m_uiDelayed = false;
	m_vmResponded = m_waiting;
	// Scan thread was waiting for response when game load kicked off. Release it with a failure code once .
	if (m_waiting)
	{
		REL_MESSAGE("VM check released for request {}", nonce);
		m_uiReport.notify_one();
	}
}


void UIState::SetMCMState(const bool isOpen)
{
	REL_MESSAGE("MCM open = {}", isOpen);
	m_mcmOpen = isOpen;
}

}
