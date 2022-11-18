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

#include <spdlog/sinks/basic_file_sink.h>

extern std::shared_ptr<spdlog::logger> SHSELogger;

// wrappers for spdLog to make release/debug logging easier
// Debug build only
#if _DEBUG || defined(_FULL_LOGGING)
#define DBG_DMESSAGE(a_fmt, ...) SHSELogger->debug(a_fmt __VA_OPT__(,) __VA_ARGS__)
#define DBG_VMESSAGE(a_fmt, ...) SHSELogger->trace(a_fmt __VA_OPT__(,) __VA_ARGS__)
#define DBG_MESSAGE(a_fmt, ...) SHSELogger->info(a_fmt __VA_OPT__(,) __VA_ARGS__)
#define DBG_WARNING(a_fmt, ...) SHSELogger->warn(a_fmt __VA_OPT__(,) __VA_ARGS__)
#define DBG_ERROR(a_fmt, ...) SHSELogger->error(a_fmt __VA_OPT__(,) __VA_ARGS__)
#define DBG_FATALERROR(a_fmt, ...) SHSELogger->critical(a_fmt __VA_OPT__(,) __VA_ARGS__)
#else
#define DBG_DMESSAGE(a_fmt, ...)
#define DBG_VMESSAGE(a_fmt, ...)
#define DBG_MESSAGE(a_fmt, ...)
#define DBG_WARNING(a_fmt, ...)
#define DBG_ERROR(a_fmt, ...)
#define DBG_FATALERROR(a_fmt, ...)
#endif

// Always log
#define REL_DMESSAGE(a_fmt, ...) SHSELogger->debug(a_fmt __VA_OPT__(,) __VA_ARGS__)
#define REL_VMESSAGE(a_fmt, ...) SHSELogger->trace(a_fmt __VA_OPT__(,) __VA_ARGS__)
#define REL_MESSAGE(a_fmt, ...) SHSELogger->info(a_fmt __VA_OPT__(,) __VA_ARGS__)
#define REL_WARNING(a_fmt, ...) SHSELogger->warn(a_fmt __VA_OPT__(,) __VA_ARGS__)
#define REL_ERROR(a_fmt, ...) SHSELogger->error(a_fmt __VA_OPT__(,) __VA_ARGS__)
#define REL_FATALERROR(a_fmt, ...) SHSELogger->critical(a_fmt __VA_OPT__(,) __VA_ARGS__)
