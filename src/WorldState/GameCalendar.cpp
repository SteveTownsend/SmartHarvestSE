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

#include "WorldState/GameCalendar.h"

namespace shse
{

std::unique_ptr<GameCalendar> GameCalendar::m_instance;

GameCalendar& GameCalendar::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<GameCalendar>();
	}
	return *m_instance;
}

GameCalendar::GameCalendar()
{
}

// Values from https://en.uesp.net/wiki/Lore:Calendar#Skyrim_Calendar
const std::vector<std::pair<std::string, unsigned int>> GameCalendar::m_monthDays = {
	{"Morning Star", 31},
	{"Sun's Dawn",	 28},
	{"First Seed",	 31},
	{"Rain's Hand",	 30},
	{"Second Seed",	 31},
	{"Midyear",		 30},
	{"Sun's Height", 31},
	{"Last Seed",	 31},
	{"Hearthfire",	 30},
	{"Frostfall",	 31},
	{"Sun's Dusk",	 30},
	{"Evening Star", 31}
};
const std::vector<std::string> GameCalendar::m_dayNames = {
	"Sundas",
	"Morndas",
	"Tirdas",
	"Middas",
	"Turdas",
	"Fredas",
	"Loredas"
};

constexpr unsigned int GameCalendar::DaysPerYear()
{
	unsigned int days(0);
	return std::accumulate(m_monthDays.cbegin(), m_monthDays.cend(), days,
		[&] (const unsigned int& total, const auto& value) { return total + value.second; });
}

std::string GameCalendar::DateTimeString(const float gameTime) const
{
	return std::to_string(gameTime);
}

}