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

RecursiveLock::RecursiveLock()
{
	InitializeCriticalSectionAndSpinCount(&m_cs, 100);
}
RecursiveLock::~RecursiveLock()
{
	DeleteCriticalSection(&m_cs);
}
void RecursiveLock::Lock()
{
	EnterCriticalSection(&m_cs);
}
void RecursiveLock::Unlock()
{
	LeaveCriticalSection(&m_cs);
}

RecursiveLockGuard::RecursiveLockGuard(RecursiveLock& lock) : m_lock(lock)
{
	m_lock.Lock();
}

RecursiveLockGuard::~RecursiveLockGuard()
{
	m_lock.Unlock();
}
