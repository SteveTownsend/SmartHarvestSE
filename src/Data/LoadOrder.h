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

class LoadOrder {
public:
	static LoadOrder& Instance();
	bool Analyze(void);
	RE::FormID GetFormIDMask(const std::string& modName) const;
	bool IncludesMod(const std::string& modName) const;
	bool ModOwnsForm(const std::string& modName, const RE::FormID formID) const;
	void AsJSON(nlohmann::json& j) const;

private:
	static constexpr RE::FormID LightFormIDSentinel = 0xfe000000;
	static constexpr RE::FormID LightFormIDMask = 0xfefff000;
	static constexpr RE::FormID RegularFormIDMask = 0xff000000;
	// no lock as all public functions are const once loaded
	static std::unique_ptr<LoadOrder> m_instance;
	std::unordered_map<std::string, RE::FormID> m_formIDMaskByName;
};

void to_json(nlohmann::json& j, const LoadOrder& p);

}
