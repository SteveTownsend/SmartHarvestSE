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

	bool Init();
	void PrepareForReloadOrNewGame();
	void ResetTransientState(const bool gameReload);
	void OnVMSync();
	void OnSettingsPushed(void);

	// give the debug message time to catch up during calibration
	static constexpr double CalibrationThreadDelaySeconds = 5.0;

private:
	bool OneTimeLoad(void);
	bool Load();
	void Start();
	bool IsSynced() const;
	static void ScanThread(void);
	inline bool Loaded() const { return m_loadProgress == LoadProgress::Complete; }

	// Worker thread loop smallest possible delay
	static constexpr double MinThreadDelaySeconds = 0.1;

	static std::unique_ptr<PluginFacade> m_instance;
	mutable RecursiveLock m_pluginLock;
	enum class LoadProgress : uint8_t {
		NotStarted,
		Started,
		Complete
	};
	LoadProgress m_loadProgress;
	bool m_threadStarted;
	bool m_pluginSynced;
};

}
