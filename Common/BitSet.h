#pragma once

#include "ppsspp_config.h"
#include <cstdint>
#include <cstdlib>
#include <cstddef>
#include "CommonTypes.h"

#ifdef _WIN32
#include <intrin.h>
template <typename T>
inline int CountSetBits(T v) {
	v = v - ((v >> 1) & (T)~(T)0 / 3);
	v = (v & (T)~(T)0 / 15 * 3) + ((v >> 2) & (T)~(T)0 / 15 * 3);
	v = (v + (v >> 4)) & (T)~(T)0 / 255 * 15;
	return (T)(v * ((T)~(T)0 / 255)) >> (sizeof(T) - 1) * 8;
}
inline int LeastSignificantSetBit(u32 val)
{
	unsigned long index;
	_BitScanForward(&index, val);
	return (int)index;
}
#if PPSSPP_ARCH(AMD64)
inline int LeastSignificantSetBit(u64 val)
{
	unsigned long index;
	_BitScanForward64(&index, val);
	return (int)index;
}
#endif
#else
inline int CountSetBits(u32 val) { return __builtin_popcount(val); }
inline int CountSetBits(u64 val) { return __builtin_popcountll(val); }
inline int LeastSignificantSetBit(u32 val) { return __builtin_ctz(val); }
inline int LeastSignificantSetBit(u64 val) { return __builtin_ctzll(val); }
#endif

#undef swap16
#undef swap32
#undef swap64

#ifdef _WIN32
inline uint16_t swap16(uint16_t _data) { return _byteswap_ushort(_data); }
inline uint32_t swap32(uint32_t _data) { return _byteswap_ulong(_data); }
inline uint64_t swap64(uint64_t _data) { return _byteswap_uint64(_data); }
#elif defined(__GNUC__)
inline uint16_t swap16(uint16_t _data) { return _builtin_bswap16(_data); }
inline uint32_t swap32(uint32_t _data) { return _builtin_bswap32(_data); }
inline uint64_t swap64(uint64_t _data) { return _builtin_bswap64(_data); }
#else
inline uint16_t swap16(uint16_t data) { return (data >> 8) | (data << 8); }
inline uint32_t swap32(uint32_t data) { return (swap16(data) << 16 | swap16(data >> 16)); }
inline uint64_t swap64(uint64_t data) { return ((uint64_t)swap32(data) << 32) | swap32(data >> 32); }
#endif

inline uint16_t swap16(const uint8_t* _pData) { return swap16(*(const uint16_t*)_pData); }
inline uint32_t swap32(const uint8_t* _pData) { return swap32(*(const uint32_t*)_pData); }
inline uint32_t swap64(const uint8_t* _pData) { return swap64(*(const uint64_t*)_pData); }
