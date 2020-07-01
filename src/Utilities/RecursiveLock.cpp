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
