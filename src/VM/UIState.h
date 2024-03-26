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
#pragma once

#include <condition_variable>
#include <mutex>

namespace shse
{
class UIState {
public:

	static UIState& Instance();
	UIState();

	void WaitUntilVMGoodToGo();
	void ReportVMGoodToGo(const bool delayed, const int nonce);
	void Reset();
	void SetMCMState(const bool isOpen);
	ScanStatus OKToScan() const;

private:
	// Worker thread loop delays once UI ready
	static constexpr double OnUIClosedThreadDelaySeconds = 1.0;

	static std::unique_ptr<UIState> m_instance;
	int m_nonce;
	std::condition_variable m_uiReport;
	std::mutex m_uiLock;
	bool m_vmResponded;
	bool m_uiDelayed;
	bool m_waiting;
	bool m_mcmOpen;
};

}