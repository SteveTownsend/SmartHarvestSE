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

#include <variant>
#include "WorldState/ActorTracker.h"
#include "WorldState/AdventureTargets.h"
#include "Collections/Collection.h"
#include "WorldState/PartyMembers.h"
#include "WorldState/VisitedPlaces.h"

namespace shse
{

typedef std::variant<AdventureEvent, ItemCollected, PartyUpdate, PartyVictim, VisitedPlace> SagaEvent;

class Saga
{
public:
	static Saga& Instance();
	Saga();

	void Reset();
	size_t DaysWithEvents() const;
	std::string DateStringByIndex(const unsigned int dayIndex) const;
	size_t CurrentDayPageCount() const;
	std::string PageByNumber(const unsigned int pageNumber) const;

private:
	void AddPaginatedText(std::ostringstream& page, const std::string& text, const bool skipIfNewPage) const;
	void FlushPaginatedText(std::ostringstream& page) const;

	static std::unique_ptr<Saga> m_instance;
	mutable RecursiveLock m_sagaLock;
	// persistent record of events on each day
	std::vector<std::vector<SagaEvent>> m_eventsByDay;
	// transient view used to map MCM Input day number to that day's events - ordered by time of day in minutes
	mutable std::vector<std::pair<unsigned int, std::multimap<unsigned int, SagaEvent>>> m_daysWithEvents;
	mutable std::vector<std::string> m_currentDayPages;
	mutable size_t m_currentPageLength;
	mutable bool m_hasContent;

public:
	template <typename EVENTTYPE>
	void AddEvent(const EVENTTYPE& event)
	{
		RecursiveLockGuard guard(m_sagaLock);
		size_t elapsedDays(static_cast<size_t>(std::floor(event.GameTime())));
		if (elapsedDays+1 > m_eventsByDay.size())
		{
			m_eventsByDay.resize(elapsedDays+1);
		}
		m_eventsByDay[elapsedDays].push_back(SagaEvent(event));
	}	
};

}
