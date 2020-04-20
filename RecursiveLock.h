#pragma once

#include <synchapi.h>

class RecursiveLock
{
public:
	RecursiveLock();
	~RecursiveLock();
	void Lock();
	void Unlock();
private:
	RecursiveLock(const RecursiveLock& b) = delete;
	RecursiveLock& operator=(const RecursiveLock& b) = delete;

	CRITICAL_SECTION m_cs;
};

class RecursiveLockGuard
{
public:
	RecursiveLockGuard(RecursiveLock& lock);
	~RecursiveLockGuard();
private:
	RecursiveLockGuard(const RecursiveLockGuard& b) = delete;
	RecursiveLockGuard& operator=(const RecursiveLockGuard& b) = delete;
	RecursiveLock& m_lock;
};