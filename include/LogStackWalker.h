#pragma once
#include "StackWalker.h"

class LogStackWalker : public StackWalker {
public:
	LogStackWalker();
	~LogStackWalker();

	static DWORD LogStack(LPEXCEPTION_POINTERS exceptionInfo);

protected:
	virtual void OnOutput(LPCSTR szText);

private:
	std::ostringstream m_fullStack;
};

