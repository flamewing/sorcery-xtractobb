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

#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <type_traits>

// Utility to convert "fancy pointer" to pointer (ported from C++20).
template <typename T>
__attribute__((always_inline, const)) inline constexpr auto to_address(
        T* input) noexcept -> T* {
    return input;
}

template <typename Iter>
__attribute__((always_inline, pure)) inline constexpr auto to_address(
        Iter input) {
    return to_address(input.operator->());
}

#define MPL_DEFINE_HAS_ALIAS(alias_name) \
    template <typename T>                \
    using alias_name = typename T::alias_name;    // NOLINT

namespace detail {

    // is_detected and related (ported from Library Fundamentals v2).
    namespace {
        template <
                typename Default, typename AlwaysVoid,
                template <typename...> class Op, typename... Args>
        struct detector {
            using value_t = std::false_type;
            using type    = Default;
        };

        template <
                typename Default, template <typename...> class Op,
                typename... Args>
        struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
            using value_t = std::true_type;
            using type    = Op<Args...>;
        };

        // NOLINTNEXTLINE(hicpp-special-member-functions,cppcoreguidelines-special-member-functions)
        struct nonesuch final {
            nonesuch(nonesuch const&)       = delete;
            nonesuch()                      = delete;
            ~nonesuch()                     = delete;
            void operator=(nonesuch const&) = delete;
        };
    }    // namespace

    template <template <typename...> class Op, typename... Args>
    using is_detected = typename detector<nonesuch, void, Op, Args...>::value_t;

    template <template <typename...> class Op, typename... Args>
    inline constexpr bool is_detected_v = is_detected<Op, Args...>::value;

    template <template <typename...> class Op, typename... Args>
    using detected_t = typename detector<nonesuch, void, Op, Args...>::type;

    template <
            typename Default, template <typename...> class Op, typename... Args>
    using detected_or = detector<Default, void, Op, Args...>;

    template <
            typename Default, template <typename...> class Op, typename... Args>
    using detected_or_t = typename detector<Default, void, Op, Args...>::type;

    template <
            typename Expected, template <typename...> class Op,
            typename... Args>
    using is_detected_exact = std::is_same<Expected, detected_t<Op, Args...>>;

    template <
            typename Expected, template <typename...> class Op,
            typename... Args>
    inline constexpr bool is_detected_exact_v
            = is_detected_exact<Expected, Op, Args...>::value;

    template <typename To, template <typename...> class Op, typename... Args>
    using is_detected_convertible
            = std::is_convertible<detected_t<Op, Args...>, To>;

    template <typename To, template <typename...> class Op, typename... Args>
    inline constexpr bool is_detected_convertible_v
            = is_detected_convertible<To, Op, Args...>::value;

    // Meta-programming stuff.
    // Utility for finding if a type appears in a list of types.
    template <typename Value, typename...>
    struct is_any_of : std::false_type {};

    template <typename Value, typename B1, typename... Bn>
    struct is_any_of<Value, B1, Bn...>
            : std::bool_constant<
                      std::is_same_v<
                              Value, B1> || is_any_of<Value, Bn...>::value> {};

    template <typename Value, typename... Bs>
    inline constexpr bool is_any_of_v = is_any_of<Value, Bs...>::value;

    // Concepts-like syntax. Based on code by Isabella Muerte.
    template <bool... Bs>
    inline constexpr bool require
            = std::conjunction_v<std::bool_constant<Bs>...>;

    template <bool... Bs>
    inline constexpr bool either
            = std::disjunction_v<std::bool_constant<Bs>...>;

    template <template <typename...> class Op, typename... Args>
    inline constexpr bool exists = is_detected_v<Op, Args...>;

    template <typename To, template <typename...> class Op, typename... Args>
    inline constexpr bool converts_to
            = is_detected_convertible_v<To, Op, Args...>;

    namespace aliases {
        MPL_DEFINE_HAS_ALIAS(value_type)
        MPL_DEFINE_HAS_ALIAS(iterator_category)
        MPL_DEFINE_HAS_ALIAS(container_type)
    }    // namespace aliases

    namespace traits {
        template <typename T>
        using iterator_traits = typename std::iterator_traits<T>;
    }

    namespace ops {
        template <class T>
        using dereference = decltype(*std::declval<T>());

        template <class T>
        using postfix_increment = decltype(std::declval<T>()++);

        template <class T>
        using prefix_increment = decltype(++std::declval<T>());
    }    // namespace ops

    namespace adl {
        using std::swap;
        template <class T, class U = T>
        using swap_with = decltype(swap(std::declval<T>(), std::declval<U>()));
    }    // namespace adl

    namespace concepts {
        template <class T>
        inline constexpr bool CopyConstructible
                = std::is_copy_constructible<T>::value;

        template <class T>
        inline constexpr bool CopyAssignable
                = std::is_copy_assignable<T>::value;

        template <class T>
        inline constexpr bool Destructible = std::is_destructible<T>::value;

        template <class T, class U>
        inline constexpr bool SwappableWith = exists<adl::swap_with, T, U>;

        template <class T>
        inline constexpr bool Swappable = SwappableWith<T&, T&>;
        template <class T>
        inline constexpr bool Pointer = std::is_pointer<T>::value;

        template <class T>
        inline constexpr bool Iterator = require<
                CopyConstructible<T>, CopyAssignable<T>, Destructible<T>,
                Swappable<T>, exists<ops::postfix_increment, T>,
                exists<ops::prefix_increment, T>, exists<ops::dereference, T>>;
    }    // namespace concepts

    template <template <typename...> class Op, typename... Args>
    inline constexpr bool is_character_type_v = is_any_of_v<
            std::remove_cv_t<detected_t<Op, Args...>>,
#if __cplusplus >= 201703L
            std::byte,
#endif
            char, unsigned char>;

    template <typename T>
    struct is_pointer_like
            : std::bool_constant<either<
                      require<concepts::Pointer<T>,
                              is_character_type_v<
                                      aliases::value_type,
                                      detected_t<traits::iterator_traits, T>>>,
                      require<concepts::Iterator<T>,
                              either<
                                      // For (back_|front_)?insert_iterator:
                                      require<converts_to<
                                                      std::output_iterator_tag,
                                                      aliases::
                                                              iterator_category,
                                                      detected_t<
                                                              traits::iterator_traits,
                                                              T>>,
                                              is_character_type_v<
                                                      aliases::value_type,
                                                      detected_t<
                                                              aliases::
                                                                      container_type,
                                                              T>>>,
                                      // For all other iterators:
                                      require<converts_to<
                                                      std::forward_iterator_tag,
                                                      aliases::
                                                              iterator_category,
                                                      detected_t<
                                                              traits::iterator_traits,
                                                              T>>,
                                              is_character_type_v<
                                                      aliases::value_type,
                                                      detected_t<
                                                              traits::iterator_traits,
                                                              T>>>>>>> {};

    template <typename T>
    struct is_pointer_like<T&> : is_pointer_like<T> {};

    template <typename T>
    inline constexpr bool is_pointer_like_v = is_pointer_like<T>::value;

    template <typename T>
    using is_pointer_like_t = std::enable_if_t<is_pointer_like_v<T>, bool>;
}    // namespace detail
#undef MPL_DEFINE_HAS_ALIAS

template <typename T, detail::is_pointer_like_t<T> = true>
__attribute__((always_inline)) inline auto Read4(T&& input) -> uint32_t {
    auto ptr{to_address(input)};
    alignas(alignof(uint32_t)) std::array<uint8_t, sizeof(uint32_t)> buffer{};
    std::memcpy(buffer.data(), ptr, sizeof(uint32_t));
    uint32_t val = 0;
    std::memcpy(&val, buffer.data(), sizeof(uint32_t));
    std::advance(input, sizeof(uint32_t));
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return val;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    return __builtin_bswap32(val);
#else
#    error "Byte order is neither little endian nor big endian. Do not know how to proceed."
    return val;
#endif
}

template <typename T, detail::is_pointer_like_t<T> = true>
inline void Write4(T&& output, uint32_t val) {
    auto ptr{to_address(output)};
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    val = __builtin_bswap32(val);
#elif __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#    error "Byte order is neither little endian nor big endian. Do not know how to proceed."
#endif
    std::memcpy(ptr, &val, sizeof(uint32_t));
    std::advance(output, sizeof(uint32_t));
}

inline void Write4(std::ostream& output, uint32_t val) {
    using oschar_t = std::ostream::char_type;
    alignas(alignof(uint32_t)) std::array<oschar_t, sizeof(uint32_t)> buffer{};
    Write4(std::begin(buffer), val);
    output.write(std::cbegin(buffer), sizeof(uint32_t));
}
