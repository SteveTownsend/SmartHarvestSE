#pragma once

#include <condition_variable>
#include <mutex>

class UIState {
public:
	static UIState& Instance();
	UIState();

	bool OKForSearch();
	void Reset();
	void ReportVMGoodToGo(const bool goodToGo, const int nonce);

private:
	bool VMGoodToGo();

	static std::unique_ptr<UIState> m_instance;
	bool m_effectiveGoodToGo;
	int m_nonce;
	std::condition_variable m_uiReport;
	std::mutex m_uiLock;
	bool m_vmResponded;
	bool m_vmGoodToGo;
};