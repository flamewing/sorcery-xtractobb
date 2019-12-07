/*
 *	Copyright © 2019 Flamewing <flamewing.sonic@gmail.com>
 *
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ENDIANIO_HH
#define ENDIANIO_HH

#include <array>
#include <cstdint>
#include <cstring>
#include <ostream>

// Utility to convert "fancy pointer" to pointer (ported from C++20).
template <typename T>
__attribute__((always_inline, const)) inline constexpr auto
to_address(T* in) noexcept -> T* {
    return in;
}

template <typename Iter>
__attribute__((always_inline, pure)) inline constexpr auto to_address(Iter in) {
    return to_address(in.operator->());
}

template <typename T>
__attribute__((always_inline)) inline auto Read4(T&& in) -> uint32_t {
    auto ptr{to_address(in)};
    alignas(alignof(uint32_t)) std::array<uint8_t, sizeof(uint32_t)> buf{};
    uint32_t                                                         val;
    std::memcpy(buf.data(), ptr, sizeof(uint32_t));
    std::memcpy(&val, buf.data(), sizeof(uint32_t));
    std::advance(in, sizeof(uint32_t));
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return val;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return __builtin_bswap32(val);
#else
#    error                                                                     \
        "Byte order is neither little endian nor big endian. Do not know how to proceed."
    return val;
#endif
}

template <typename T>
inline void Write4(T&& out, uint32_t val) {
    auto ptr{to_address(out)};
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    val = __builtin_bswap32(val);
#elif __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#    error                                                                     \
        "Byte order is neither little endian nor big endian. Do not know how to proceed."
#endif
    std::memcpy(ptr, &val, sizeof(uint32_t));
    std::advance(out, sizeof(uint32_t));
}

template <typename T>
inline void Write4(std::ostream& out, uint32_t val) {
    alignas(alignof(uint32_t))
        typename std::ostream::char_type buffer[sizeof(uint32_t)];
    Write4(std::begin(buffer), val);
    out.write(std::cbegin(buffer), sizeof(uint32_t));
}

#endif
