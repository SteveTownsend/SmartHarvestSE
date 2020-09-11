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

namespace shse
{

class PluginFacade {
public:
	static PluginFacade& Instance();
	PluginFacade();

	bool Init(const bool onGameReload);
	void PrepareForReload();
	void ResetTransientState(const bool gameReload);
	void AfterReload();
	void OnSettingsPushed(void);

	// give the debug message time to catch up during calibration
	static constexpr double CalibrationThreadDelaySeconds = 5.0;

private:
	bool Load();
	void Start();
	bool IsSynced() const;
	static void ScanThread(void);

	// Worker thread loop smallest possible delay
	static constexpr double MinThreadDelaySeconds = 0.1;

	static std::unique_ptr<PluginFacade> m_instance;
	mutable RecursiveLock m_pluginLock;
	bool m_pluginOK;
	bool m_threadStarted;
	bool m_pluginSynced;
};

}
