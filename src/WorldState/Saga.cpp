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

#include "WorldState/Saga.h"
#include "WorldState/GameCalendar.h"

namespace shse
{

std::unique_ptr<Saga> Saga::m_instance;

Saga& Saga::Instance()
{
	if (!m_instance)
	{
		m_instance = std::make_unique<Saga>();
	}
	return *m_instance;
}

Saga::Saga()
{
}

void Saga::Reset()
{
	RecursiveLockGuard guard(m_sagaLock);
	m_daysWithEvents.clear();
	m_eventsByDay.clear();
	m_currentDayText.clear();
}

size_t Saga::DaysWithEvents() const
{
	RecursiveLockGuard guard(m_sagaLock);
	m_daysWithEvents.clear();
	m_daysWithEvents.reserve(m_eventsByDay.size());
	unsigned int index(0);
	size_t count(std::count_if(m_eventsByDay.cbegin(), m_eventsByDay.cend(),
		[&](const std::vector<SagaEvent>& events) -> bool {
		if (!events.empty())
		{
			m_daysWithEvents.push_back({ index, {} });
			++index;
			// sort each day's events by time before we render the events as text. This view on the current dataset is used to display text in MCM.
			auto& sortedEvents = m_daysWithEvents.back().second;
			std::for_each(events.cbegin(), events.cend(), [&](const SagaEvent& event) {
				sortedEvents.insert({ std::visit([&] (auto const& e) { return GameCalendar::Instance().DayPartInMinutes(e.GameTime()); }, event), event });
			});
			return true;
		}
		else
		{
			++index;
			return false;
		}
	}));
	DBG_MESSAGE("Days with events {}", count);
	return count;
}

std::string Saga::DateStringByIndex(const unsigned int dayIndex) const
{
	RecursiveLockGuard guard(m_sagaLock);
	if (dayIndex - 1 < m_daysWithEvents.size())
	{
		// seed text lines for this day's events
		m_currentDayText.clear();
		const auto& sortedEvents = m_daysWithEvents[dayIndex - 1].second;
		unsigned int currentMinute(0);
		bool first(true);
		for (const auto timeEvent : sortedEvents)
		{
			std::ostringstream thisLine;
			if (first || currentMinute != timeEvent.first)
			{
				first = false;
				currentMinute = timeEvent.first;
				thisLine << GameCalendar::Instance().TimeString(currentMinute) << ' ';
			}
			thisLine << std::visit([&](auto const& e) { return e.AsString(); }, timeEvent.second);
			m_currentDayText.push_back(thisLine.str());
		}
		DBG_MESSAGE("Current day has {} events", m_currentDayText.size());
		return GameCalendar::Instance().DateString(m_daysWithEvents[dayIndex - 1].first);
	}
	return "";
}

// Relies on context set by last call to DateStringByIndex
size_t Saga::TextLineCount() const
{
	RecursiveLockGuard guard(m_sagaLock);
	DBG_MESSAGE("{} lines available", m_currentDayText.size());
	return m_currentDayText.size();
}

std::string Saga::TextLine(const unsigned int lineNumber) const
{
	RecursiveLockGuard guard(m_sagaLock);
	std::string result;
	if (lineNumber < m_currentDayText.size())
	{
		constexpr size_t MaxLineLength = 75;
		result = m_currentDayText[lineNumber];
		// Don't send MCM too much data. Pad to ensure output left-justified.
		result.resize(MaxLineLength, ' ');
		result.back() = '_';
	}
	DBG_MESSAGE("Saga line number {} = {}", lineNumber, result);
	return result;
}

}