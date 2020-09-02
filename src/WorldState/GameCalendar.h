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

class GameCalendar
{
public:

	static GameCalendar& Instance();
	GameCalendar();

	std::string DateTimeString(const float gameTime) const;

private:
	constexpr unsigned int DaysPerYear() const;
	// start date is 4E 201, 17th of Last Seed
	/* start time logged in new game is 9.00 am or so:
		2020-09-01 16:59:23.823     info  18628 GameTime is now 0.375/0.375352
	*/

	static constexpr unsigned int StartYear = 201;
	static constexpr unsigned int StartDay = 17;
	// zero-based offset
	static constexpr unsigned int StartMonth = 7;

	static std::unique_ptr<GameCalendar> m_instance;
	static const std::vector<std::pair<std::string, unsigned int>> m_monthDays;
	static const std::vector<std::string> m_dayNames;
	static const std::vector<std::string> m_dayOfMonth;
};

}