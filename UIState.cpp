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

UIState::UIState() : m_pluginUIGoodToGo(false), m_nonce(0), m_uiGoodToGo(false), m_uiResponded(false)
{
}

// Compare with original AHSE UI prerequisites via script event and callback.
bool UIState::UIGoodToGo()
{
	// 5 seconds script lag is an eternity
	constexpr std::chrono::milliseconds MaxPingLag(5000LL);

	const auto startTime(std::chrono::high_resolution_clock::now());
	EventPublisher::Instance().TriggerCheckOKToScan(++m_nonce);

	// wait for async result from script
	std::unique_lock<std::mutex> guard(m_uiLock);
	m_uiResponded = false;
	const bool waitResult(m_uiReport.wait_until(guard, startTime + MaxPingLag, [&] { return m_uiResponded; } ));
	const auto endTime(std::chrono::high_resolution_clock::now());
	if (!waitResult)
	{
		REL_WARNING("Script did not report UI status, returned after %lld microseconds",
			std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count());
		return false;
	}
	else
	{
		DBG_MESSAGE("Script reported UI status %d after %lld microseconds", m_uiGoodToGo, 
			std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count());
		return m_uiGoodToGo;
	}
}

void UIState::ReportGoodToGo(const bool goodToGo, const int nonce)
{
	if (nonce != m_nonce)
	{
		REL_WARNING("VM Good to Go = %d for request %d, expected request %d", goodToGo, nonce, m_nonce);
		return;
	}
	DBG_MESSAGE("VM Good to Go = %d for request %d", goodToGo, nonce);
	// response matches pending request - update the status and release waiter
	m_uiGoodToGo = goodToGo;
	m_uiResponded = true;
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
	bool pluginGoodToGo(!pluginUIBad && !pluginControlsBad);
	bool vmGoodToGo(UIGoodToGo());
	if (vmGoodToGo != pluginGoodToGo)
	{
		// prefer old-style UI check if they do not match
		REL_WARNING("plugin UI good-to-go %d (menu count %d, controls disabled %d) does not match VM UI good-to-go %d - trust VM",
			pluginGoodToGo, count, pluginControlsBad, vmGoodToGo);
		pluginGoodToGo = vmGoodToGo;
	}

	if (pluginGoodToGo != m_pluginUIGoodToGo)
	{
		// record state change
		m_pluginUIGoodToGo = pluginGoodToGo;
		if (!pluginGoodToGo)
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
	return !m_pluginUIGoodToGo;
}

void UIState::Reset()
{
	m_pluginUIGoodToGo = false;
}