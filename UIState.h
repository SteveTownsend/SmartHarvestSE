#pragma once

class UIState {
public:
	static UIState& Instance();
	UIState();

	bool OKForSearch();
	void Reset();

private:
	static std::unique_ptr<UIState> m_instance;
	bool m_menuOpen;
};