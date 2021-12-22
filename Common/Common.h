#pragma once

#include <stdarg.h>

#ifdef _MSC_VER
#pragma warning(disable:4100)
#pragma warning(disable:4244)
#endif

#include "CommonTypes.h"
#include "CommonFuncs.h"

#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(t) \
	t(const t& other) = delete; \
	void operator =(const t &other) = delete;
#endif

#ifndef ENUM_CLASS_BITOPS
#define ENUM_CLASS_BITOPS(T) \
	static inline T operator |(const T &lhs, const T &rhs) { \
		return T((int)lhs | (int)rhs); \
	} \
	static inline T &operator |= (T &lhs, const T &rhs) { \
		lhs = lhs | rhs; \
		return lhs; \
	} \
	static inline bool operator &(const T &lhs, const T &rhs) { \
		return ((int)lhs & (int)rhs) != 0; \
	}
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

#if defined(_WIN32)

	#define CHECK_HEAP_INTEGRITY()

	#if defined(_DEBUG)
		#include <crtdbg.h>
		#undef CHECK_HEAP_INTEGRITY
		#define CHECK_HEAP_INTEGRITY() {if (!_CrtCheckMemory()) _assert_msg_(false, "Memory corruption detected. See log.");}
	#endif
#else

#define CHECK_HEAP_INTEGRITY()

#endif

#ifndef _WIN32
#include <limits.h>
#ifndef MAX_PATH
#define MAX_PATH PATH_MAX
#endif

#define __forceinline inline __attribute__((always_inline))
#endif

#if defined __SSE4_2__
#define _M_SSE 0x402
#elif defined __SSE4_1__
#define _M_SSE 0x401
#elif defined __SSSE3__
#define _M_SSE 0x301
#elif defined __SSE3__
#define _M_SSE 0x300
#elif defined __SSE2__
#define _M_SSE 0x200
#elif !defined(__GNUC__) && (defined(_M_X64) || defined(_M_IX86))
#define _M_SSE 0x402
#endif
