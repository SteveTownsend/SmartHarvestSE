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
#include "WorldState/AdventureTargets.h"
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

Saga::Saga() : m_currentPageLength(0), m_hasContent(false)
{
}

void Saga::Reset()
{
	RecursiveLockGuard guard(m_sagaLock);
	m_daysWithEvents.clear();
	m_eventsByDay.clear();
	m_currentDayPages.clear();
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

void Saga::AddPaginatedText(std::ostringstream& page, const std::string& text, const bool skipIfNewPage) const
{
	if (text.empty())
		return;
	// 1023 is the limit but we want display to not go off the edge of the screen
	constexpr size_t MaxPageLength = 620;
	if (m_currentPageLength + text.length() >= MaxPageLength)
	{
		if (skipIfNewPage)
			return;
		FlushPaginatedText(page);
	}
	page << text;
	m_currentPageLength += text.length();
	if (!skipIfNewPage)
	{
		m_hasContent = true;
	}
}

void Saga::FlushPaginatedText(std::ostringstream& page) const
{
	if (m_currentPageLength > 0 && m_hasContent)
	{
		page << std::ends;		// terminate text we wrote for use with str()
		std::string pageStr(page.str());
		pageStr.resize(m_currentPageLength);
		m_currentDayPages.push_back(pageStr);
		DBG_MESSAGE("Flushed page {}\n{}", m_currentDayPages.size(), page.str());

		// see https://stackoverflow.com/questions/624260/how-to-reuse-an-ostringstream including comments
		page.seekp(0);			// seek put ptr to start - no reallocation unless next page has more text
		m_currentPageLength = 0;
		m_hasContent = false;
	}
}

std::string Saga::DateStringByIndex(const unsigned int dayIndex) const
{
	RecursiveLockGuard guard(m_sagaLock);
	if (dayIndex - 1 < m_daysWithEvents.size())
	{
		// seed freeform text pages for this day's events. Text is stateful for some events.
		AdventureEvent::ResetSagaState();
		VisitedPlace::ResetSagaState();
		m_currentDayPages.clear();
		const auto& sortedEvents = m_daysWithEvents[dayIndex - 1].second;
		unsigned int currentMinute(0);
		bool first(true);
		std::ostringstream thisPage;
		m_currentPageLength = 0;
		for (const auto timeEvent : sortedEvents)
		{
			// check if event has anything to output
			std::string eventStr(std::visit([&](auto const& e) { return e.AsString(); }, timeEvent.second));
			if (!eventStr.empty())
			{
				if (first || currentMinute != timeEvent.first)
				{
					// add newline for new paragraph if this is a time change - not needed if this forces new page
					if (!first)
					{
						AddPaginatedText(thisPage, "\n", true);
					}
					first = false;
					currentMinute = timeEvent.first;
					// timestamp and first entry need to be on the same page
					std::string timestampedEvent(GameCalendar::Instance().TimeString(currentMinute));
					timestampedEvent.append(" ");
					timestampedEvent.append(eventStr);
					AddPaginatedText(thisPage, timestampedEvent, false);
				}
				else
				{
					AddPaginatedText(thisPage, " ", true);
					AddPaginatedText(thisPage, eventStr, false);
				}
			}
		}
		FlushPaginatedText(thisPage);

		DBG_MESSAGE("Current day has {} pages for {} events", m_currentDayPages.size(),sortedEvents.size());
		return GameCalendar::Instance().DateString(m_daysWithEvents[dayIndex - 1].first);
	}
	return "";
}

// Relies on context set by last call to DateStringByIndex
size_t Saga::CurrentDayPageCount() const
{
	RecursiveLockGuard guard(m_sagaLock);
	DBG_MESSAGE("{} pages available", m_currentDayPages.size());
	return m_currentDayPages.size();
}

std::string Saga::PageByNumber(const unsigned int pageNumber) const
{
	RecursiveLockGuard guard(m_sagaLock);
	std::string result;
	// use 1-based MCM offset, 0-based vector offset
	if (pageNumber <= m_currentDayPages.size())
	{
		result = m_currentDayPages[pageNumber-1];
	}
	DBG_MESSAGE("Saga page {} =\n{}", pageNumber, result);
	return result;
}

}