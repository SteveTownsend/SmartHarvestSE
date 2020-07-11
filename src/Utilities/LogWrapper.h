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
