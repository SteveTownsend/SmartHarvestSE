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

UIState::UIState() : m_menuOpen(false)
{
}

bool UIState::OKForSearch()
{
	// By inspection, UI menu stack has steady state size of 1. Opening application and/or inventory adds 1 each,
	// opening console adds 2. So this appears to be a catch-all for those conditions.
	RE::UI* ui(RE::UI::GetSingleton());
	if (!ui)
	{
		DBG_WARNING("UI inaccessible");
		return false;
	}
	size_t count(ui->menuStack.size());
	bool menuOpen(count > 1);
	if (menuOpen != m_menuOpen)
	{
		// record state change
		m_menuOpen = menuOpen;
		if (menuOpen)
		{
			// Menu just opened
			DBG_MESSAGE("console and/or menu(s) opened, delta to menu-stack size = %d", count);
		}
		else
		{
			SearchTask::OnMenuClose();
		}
	}
	return !m_menuOpen;
}

void UIState::Reset()
{
	m_menuOpen = false;
}