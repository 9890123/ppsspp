#pragma once

#include "CommonFuncs.h"

#define NOTICE_LEVEL  1
#define ERROR_LEVEL   2
#define WARNING_LEVEL 3
#define INFO_LEVEL    4
#define DEBUG_LEVEL   5
#define VERBOSE_LEVEL 6

namespace LogTypes {
enum LOG_TYPE {
	SYSTEM = 0,
	BOOT,
	COMMON,
	CPU,
	FILESYS,
	G3D,
	HLE,
	JIT,
	LOADER,
	ME,
	MEMMAP,
	SASMIX,
	SAVESTATE,
	FRAMEBUF,
	AUDIO,
	IO,

	SCEAUDIO,
	SCECTRL,
	SCEDISPLAY,
	SCEFONT,
	SCEGE,
	SCEINTC,
	SCEIO,
	SCEKERNEL,
	SCEMODULE,
	SCENET,
	SCERTC,
	SCESAS,
	SCEUTILITY,
	SCEMISC,

	NUMBER_OF_LOGS,
};

enum LOG_LEVELS : int {
	LNOTICE = NOTICE_LEVEL,
	LERROR = ERROR_LEVEL,
	LWARNING = WARNING_LEVEL,
	LINFO = INFO_LEVEL,
	LDEBUG = DEBUG_LEVEL,
	LVERBOSE = VERBOSE_LEVEL,
};

}

void GenericLog(LogTypes::LOG_LEVELS level, LogTypes::LOG_TYPE type,
		const char *file, int line, const char *fmt, ...)
#ifdef __GNUC__
		__attribute__((format(printf, 5, 6)))
#endif
		;

bool GenericLogEnabled(LogTypes::LOG_LEVELS level, LogTypes::LOG_TYPE type);

#if defined(_DEBUG) || defined(_WIN32)

#define MAX_LOGLEVEL DEBUG_LEVEL

#else

#ifndef MAX_LOGLEVEL
#define MAX_LOGLEVEL INFO_LEVEL
#endif

#endif

#define GENERIC_LOG(t, v, ...) { \
	if (v <= MAX_LOGLEVEL) \
		GenericLog(v, t, __FILE__, __LINE__, __VA_ARGS__); \
	}

#define ERROR_LOG(t,...)   do { GENERIC_LOG(LogTypes::t, LogTypes::LERROR,   __VA_ARGS__) } while (false)
#define WARN_LOG(t,...)    do { GENERIC_LOG(LogTypes::t, LogTypes::LWARNING, __VA_ARGS__) } while (false)
#define NOTICE_LOG(t,...)  do { GENERIC_LOG(LogTypes::t, LogTypes::LNOTICE,  __VA_ARGS__) } while (false)
#define INFO_LOG(t,...)    do { GENERIC_LOG(LogTypes::t, LogTypes::LINFO,    __VA_ARGS__) } while (false)
#define DEBUG_LOG(t,...)   do { GENERIC_LOG(LogTypes::t, LogTypes::LDEBUG,   __VA_ARGS__) } while (false)
#define VERBOSE_LOG(t,...) do { GENERIC_LOG(LogTypes::t, LogTypes::LVERBOSE, __VA_ARGS__) } while (false)

bool HandleAssert(const char* function, const char* file, int line, const char* expression, const char* format, ...)
#ifdef __GNUC__
__attribute__((format(printf, 5, 6)))
#endif
;

#if defined(__ANDROID__)
#define __FILENAME__ (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : (__builtin_strrchr(__FILE__, '\\') ? __builtin_strrchr(__FILE__, '\\') + 1 : __FILE__))
#else
#define __FILENAME__ __FILE__
#endif

#if MAX_LOGLEVEL >= DEBUG_LEVEL

#define _dbg_assert_(_a_) \
	if (!(_a_)) { \
		if (!HandleAssert(__FUNCTION__, __FILENAME__, __LINE__, #_a_, "*** Assertion ***\n")) Crash(); \
	}

#define _dbg_assert_msg_(_a_, ...) \
	if (!(_a_)) { \
		if (!HandleAssert(__FUNCTION__, __FILENAME__, __LINE__, #_a_, __VA_ARGS__)) Crash(); \
	}

#else

#ifndef _dbg_assert_
#define _dbg_assert_(_a_) {}
#define _dbg_assert_msg_(_a_, _desc_, ...) {}
#endif

#define _assert_(_a_) \
	if (!(_a_)) { \
		if (!HandleAssert(__FUNCTION__, __FILENAME__, __LINE__, #_a_, "*** Assertion ***\n")) Crash(); \
	}

#define _assert_msg_(_a_, ...) \
	if (!(_a_)) { \
		if (!HandleAssert(__FUNCTION__, __FILENAME__, __LINE__, #_a_, __VA_ARGS__)) Crash(); \
	}

void OutputDebugStringUTF8(const char *p);

#endif
