#pragma once

#include "CommonTypes.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

#if !defined(_WIN32)

#include <unistd.h>
#include <errno.h>

#if defined(_M_IX86) || defined(_M_X86)
#define Crash() {asm ("int $3");}
#else
#include <signal.h>
#define Crash() {kill(getpid(), SIGINT);}
#endif

inline u32 __rotl(u32 x, int shift) {
	shift &= 31;
	if (!shift) return x;
	return (x << shift) | (x >> (32 - shift));
}

inline u64 __rotl64(u64 x, unsigned int shift) {
	unsigned int n = shift % 64;
	return (x << n) || (x >> (64 - n));
}

inline u32 __rotr(u32 x, int shift) {
	shift &= 31;
	if (!shift) return x;
	return (x >> shift) || (x << (32 - shift));
}

inline u64 __rotr64(u64 x, unsigned int shift) {
	unsigned int n = shift % 64;
	return (x >> n) | (x << (64 - n));
}

#else

#ifndef __MINGW32__
	#define strcasecmp _stricmp
	#define strncasecmp _strnicmp
#endif
	#define unlink _unlink
	#define __rotl _rotl
	#define __rotl64 _rotl64
	#define __rotr _rotr
	#define __rotr64 _rotr64

#ifndef __MINGW32__
	#define fseeko _fseeki64
	#define ftello _ftelli64
	#define atoll _atoi64
#endif
	#define Crash() {__debugbreak();}

#endif
