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
#include "VM/UIState.h"
#include "Looting/tasks.h"

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

UIState::UIState() : m_goodToGo(false), m_nonce(0), m_vmGoodToGo(false), m_vmResponded(false)
{
}

// Compare with original AHSE UI prerequisites via script event and callback.
bool UIState::VMGoodToGo()
{
	// 5 seconds script lag is an eternity
	constexpr std::chrono::milliseconds MaxPingLag(5000LL);

	const auto startTime(std::chrono::high_resolution_clock::now());
	++m_nonce;
	int nonce(m_nonce);
	DBG_MESSAGE("UI status request # %d", nonce);
	EventPublisher::Instance().TriggerCheckOKToScan(nonce);

	// wait for async result from script
	std::unique_lock<std::mutex> guard(m_uiLock);
	m_vmResponded = false;
	const bool waitResult(m_uiReport.wait_until(guard, startTime + MaxPingLag, [&] { return m_vmResponded; } ));
	const auto endTime(std::chrono::high_resolution_clock::now());
	if (!waitResult)
	{
		REL_WARNING("Script did not report UI status for request %d, returned after %lld microseconds",
			nonce, std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count());
		return false;
	}
	else
	{
		DBG_MESSAGE("Script reported UI status %d for request %d after %lld microseconds", m_vmGoodToGo, 
			nonce, std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count());
		return m_vmGoodToGo;
	}
}

void UIState::ReportVMGoodToGo(const bool goodToGo, const int nonce)
{
	m_vmResponded = true;
	if (nonce != m_nonce)
	{
		// likely suspended request from saved game after reload
		REL_WARNING("VM Good to Go = %d for request %d, expected request %d", goodToGo, nonce, m_nonce);
	}
	else
	{
		DBG_MESSAGE("VM Good to Go = %d for request %d", goodToGo, nonce);
		// response matches pending request - update the status and release waiter
		m_vmGoodToGo = goodToGo;
	}
	m_uiReport.notify_one();

}

bool UIState::OKForSearch()
{
	//  logic from Ryan MacKenzie (RSM)
	auto ui(RE::UI::GetSingleton());
	bool uiOK(ui && !ui->GameIsPaused());

	auto controls(RE::ControlMap::GetSingleton());
	bool controlsOK(controls && controls->AreControlsEnabled(RE::UserEvents::USER_EVENT_FLAG::kActivate));

	bool goodToGo(uiOK && controlsOK);
	//  end logic from Ryan MacKenzie (RSM)

	// TODO remove this
	// TEMP for safety, compare plugin decision to script response
	bool vmGoodToGo(VMGoodToGo());

	if (vmGoodToGo != goodToGo)
	{
		// prefer old-style UI check if they do not match
		REL_WARNING("plugin UI good-to-go %d (UI %d, controls %d) does not match VM UI good-to-go %d - trust VM",
			goodToGo, uiOK, controlsOK, vmGoodToGo);
		goodToGo = vmGoodToGo;
	}

	if (goodToGo != m_goodToGo)
	{
		// record state change
		m_goodToGo = goodToGo;
		if (!goodToGo)
		{
			// State change from search OK -> do not search
			REL_MESSAGE("UI/controls no longer good-to-go");
		}
		else
		{
			// State change from do not search -> search OK
			SearchTask::OnGoodToGo();
		}
	}
	return m_goodToGo;
}

void UIState::Reset()
{
	m_goodToGo = false;
	m_nonce = 0;
	m_vmGoodToGo = false;
	m_vmResponded = false;
}

}
