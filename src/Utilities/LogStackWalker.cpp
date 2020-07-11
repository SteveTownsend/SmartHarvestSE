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
#include "LogStackWalker.h"

LogStackWalker::LogStackWalker() : StackWalker() {}

LogStackWalker::~LogStackWalker() {
	DBG_MESSAGE("Callstack dump :\n%s", m_fullStack.str().c_str());
}

void LogStackWalker::OnOutput(LPCSTR szText) {
	m_fullStack << szText;
	StackWalker::OnOutput(szText);
}

DWORD LogStackWalker::LogStack(LPEXCEPTION_POINTERS exceptionInfo)
{
	LogStackWalker stackWalker;
	stackWalker.ShowCallstack(GetCurrentThread(), exceptionInfo->ContextRecord);
	return EXCEPTION_CONTINUE_SEARCH;
}
