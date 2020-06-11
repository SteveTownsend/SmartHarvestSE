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
