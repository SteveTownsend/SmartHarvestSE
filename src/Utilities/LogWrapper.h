#pragma once

// wrappers for CommonLibSSE macros to make release/debug logging easier

// Debug build only
#if _DEBUG
#define DBG_DMESSAGE(a_fmt, ...) SKSE::Impl::MacroLogger::VPrint(__FILE__, __LINE__, SKSE::Logger::Level::kDebugMessage, a_fmt, __VA_ARGS__)
#define DBG_VMESSAGE(a_fmt, ...) SKSE::Impl::MacroLogger::VPrint(__FILE__, __LINE__, SKSE::Logger::Level::kVerboseMessage, a_fmt, __VA_ARGS__)
#define DBG_MESSAGE(a_fmt, ...) SKSE::Impl::MacroLogger::VPrint(__FILE__, __LINE__, SKSE::Logger::Level::kMessage, a_fmt, __VA_ARGS__)
#define DBG_WARNING(a_fmt, ...) SKSE::Impl::MacroLogger::VPrint(__FILE__, __LINE__, SKSE::Logger::Level::kWarning, a_fmt, __VA_ARGS__)
#define DBG_ERROR(a_fmt, ...) SKSE::Impl::MacroLogger::VPrint(__FILE__, __LINE__, SKSE::Logger::Level::kError, a_fmt, __VA_ARGS__)
#define DBG_FATALERROR(a_fmt, ...) SKSE::Impl::MacroLogger::VPrint(__FILE__, __LINE__, SKSE::Logger::Level::kFatalError, a_fmt, __VA_ARGS__)
#else
#define DBG_DMESSAGE(a_fmt, ...)
#define DBG_VMESSAGE(a_fmt, ...)
#define DBG_MESSAGE(a_fmt, ...)
#define DBG_WARNING(a_fmt, ...)
#define DBG_ERROR(a_fmt, ...)
#define DBG_FATALERROR(a_fmt, ...)
#endif

// Always log
#define REL_DMESSAGE(a_fmt, ...) SKSE::Impl::MacroLogger::VPrint(__FILE__, __LINE__, SKSE::Logger::Level::kDebugMessage, a_fmt, __VA_ARGS__)
#define REL_VMESSAGE(a_fmt, ...) SKSE::Impl::MacroLogger::VPrint(__FILE__, __LINE__, SKSE::Logger::Level::kVerboseMessage, a_fmt, __VA_ARGS__)
#define REL_MESSAGE(a_fmt, ...) SKSE::Impl::MacroLogger::VPrint(__FILE__, __LINE__, SKSE::Logger::Level::kMessage, a_fmt, __VA_ARGS__)
#define REL_WARNING(a_fmt, ...) SKSE::Impl::MacroLogger::VPrint(__FILE__, __LINE__, SKSE::Logger::Level::kWarning, a_fmt, __VA_ARGS__)
#define REL_ERROR(a_fmt, ...) SKSE::Impl::MacroLogger::VPrint(__FILE__, __LINE__, SKSE::Logger::Level::kError, a_fmt, __VA_ARGS__)
#define REL_FATALERROR(a_fmt, ...) SKSE::Impl::MacroLogger::VPrint(__FILE__, __LINE__, SKSE::Logger::Level::kFatalError, a_fmt, __VA_ARGS__)
