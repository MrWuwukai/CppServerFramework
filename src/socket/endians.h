#pragma once

#define THIS_LITTLE_ENDIAN 1
#define THIS_BIG_ENDIAN 2

#include <byteswap.h>
#include <stdint.h>

#include <type_traits>

namespace Framework {
    constexpr uint64_t bswap_64(uint64_t value) {
        return ((value & 0xFF00000000000000ULL) >> 56) |
            ((value & 0x00FF000000000000ULL) >> 40) |
            ((value & 0x0000FF0000000000ULL) >> 24) |
            ((value & 0x000000FF00000000ULL) >> 8) |
            ((value & 0x00000000FF000000ULL) << 8) |
            ((value & 0x0000000000FF0000ULL) << 24) |
            ((value & 0x000000000000FF00ULL) << 40) |
            ((value & 0x00000000000000FFULL) << 56);
    }

    constexpr uint32_t bswap_32(uint32_t value) {
        return ((value & 0xFF000000) >> 24) |
            ((value & 0x00FF0000) >> 8) |
            ((value & 0x0000FF00) << 8) |
            ((value & 0x000000FF) << 24);
    }

    constexpr uint16_t bswap_16(uint16_t value) {
        return ((value & 0xFF00) >> 8) | ((value & 0x00FF) << 8);
    }

    template<class T>
    constexpr typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type byteswap(T value) {
        return static_cast<T>(bswap_64(static_cast<uint64_t>(value)));
    }

    template<class T>
    constexpr typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type byteswap(T value) {
        return static_cast<T>(bswap_32(static_cast<uint32_t>(value)));
    }

    template<class T>
    constexpr typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type byteswap(T value) {
        return static_cast<T>(bswap_16(static_cast<uint16_t>(value)));
    }

    #if BYTE_ORDER == BIG_ENDIAN
    #define THIS_BYTE_ORDER THIS_BIG_ENDIAN
    #else
    #define THIS_BYTE_ORDER THIS_LITTLE_ENDIAN
    #endif

    #if THIS_BYTE_ORDER == THIS_BIG_ENDIAN
    template<class T>
    T byteswapOnLittleEndian(T t) {
        return t;
    }

    template<class T>
    T byteswapOnBigEndian(T t) {
        return byteswap(t);
    }

    #else

    template<class T>
    T byteswapOnLittleEndian(T t) {
        return byteswap(t);
    }

    template<class T>
    T byteswapOnBigEndian(T t) {
        return t;
    }

    #endif
}