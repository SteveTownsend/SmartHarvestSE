#include "PrecompiledHeaders.h"
#include "UIState.h"
#include "tasks.h"

std::unique_ptr<UIState> UIState::m_instance;

UIState& UIState::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<UIState>();
	}
	return *m_instance;
}

UIState::UIState() : m_effectiveGoodToGo(false), m_nonce(0), m_vmGoodToGo(false), m_vmResponded(false)
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
	// By inspection, UI menu stack has steady state size of 1. Opening application and/or inventory adds 1 each,
	// opening console adds 2. So this appears to be a catch-all for those conditions.
	RE::UI* ui(RE::UI::GetSingleton());
	if (!ui)
	{
		REL_WARNING("UI inaccessible");
		return false;
	}
	size_t count(ui->menuStack.size());
	bool pluginUIBad(count > 1);
	bool pluginControlsBad(!RE::PlayerControls::GetSingleton() || !RE::PlayerControls::GetSingleton()->IsActivateControlsEnabled());
	bool effectiveGoodToGo(!pluginUIBad && !pluginControlsBad);
	bool vmGoodToGo(VMGoodToGo());
	if (vmGoodToGo != effectiveGoodToGo)
	{
		// prefer old-style UI check if they do not match
		REL_WARNING("plugin UI good-to-go %d (menu count %d, controls disabled %d) does not match VM UI good-to-go %d - trust VM",
			effectiveGoodToGo, count, pluginControlsBad, vmGoodToGo);
		effectiveGoodToGo = vmGoodToGo;
	}

	if (effectiveGoodToGo != m_effectiveGoodToGo)
	{
		// record state change
		m_effectiveGoodToGo = effectiveGoodToGo;
		if (!effectiveGoodToGo)
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
	return m_effectiveGoodToGo;
}

void UIState::Reset()
{
	m_effectiveGoodToGo = false;
	m_nonce = 0;
	m_vmGoodToGo = false;
	m_vmResponded = false;
}