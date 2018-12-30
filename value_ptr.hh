//
// Copyright 2017-2018 by Martin Moene
// Simplified version by Flamewing
//
// https://github.com/martinmoene/value-ptr-lite
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef NONSTD_VALUE_PTR_LITE_HPP
#define NONSTD_VALUE_PTR_LITE_HPP

#include <cassert>
#include <functional>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

//
// value_ptr:
//
namespace nonstd {
    using std::in_place;
    using std::in_place_index;
    using std::in_place_t;
    using std::in_place_type;
    namespace std20 {
        // type traits C++20:
        template <typename T>
        struct remove_cvref {
            using type =
                typename std::remove_cv_t<typename std::remove_reference_t<T>>;
        };
        template <typename T>
        using remove_cvref_t = typename remove_cvref<T>::type;
    } // namespace std20

    namespace detail {
        using std::default_delete;

        template <class T>
        struct default_clone {
            default_clone() = default;

            T* operator()(T const& x) const {
                static_assert(
                    sizeof(T) != 0, // NOLINT(bugprone-sizeof-expression)
                    "default_clone cannot clone incomplete type");
                static_assert(
                    !std::is_void_v<T>,
                    "default_clone cannot clone incomplete type");
                // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
                return new T(x);
            }

            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
            T* operator()(T&& x) const { return new T(std::move(x)); }

            template <class... Args>
            T* operator()(std::in_place_t, Args&&... args) const {
                // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
                return new T(std::forward<Args>(args)...);
            }

            template <class U, class... Args>
            T* operator()(
                std::in_place_t, std::initializer_list<U> il,
                Args&&... args) const {
                // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
                return new T(il, std::forward<Args>(args)...);
            }
        };

        template <class T, class Cloner, class Deleter>
        struct compressed_ptr : Cloner, Deleter {
            using element_type = T;
            using pointer      = T*;

            using cloner_type  = Cloner;
            using deleter_type = Deleter;

            // Lifetime:

            ~compressed_ptr() { deleter_type()(ptr); }

            compressed_ptr() noexcept : ptr(nullptr) {}

            explicit compressed_ptr(pointer p) noexcept : ptr(p) {}

            compressed_ptr(compressed_ptr const& other)
                : cloner_type(other), deleter_type(other),
                  ptr(other.ptr ? cloner_type()(*other.ptr) : nullptr) {}

            compressed_ptr(compressed_ptr&& other) noexcept
                : cloner_type(std::move(other)), deleter_type(std::move(other)),
                  ptr(std::move(other.ptr)) {
                other.ptr = nullptr;
            }

            compressed_ptr& operator=(compressed_ptr const& rhs) {
                if (this == &rhs) {
                    return *this;
                }
                cloner_type:: operator=(rhs);
                deleter_type::operator=(rhs);
                ptr = rhs.ptr ? cloner_type()(*rhs.ptr) : nullptr;
                return *this;
            }

            compressed_ptr& operator=(compressed_ptr&& rhs) noexcept {
                cloner_type:: operator=(std::move(rhs));
                deleter_type::operator=(std::move(rhs));

                ptr     = std::move(rhs.ptr);
                rhs.ptr = nullptr;
                return *this;
            }

            explicit compressed_ptr(element_type const& value)
                : ptr(cloner_type()(value)) {}

            explicit compressed_ptr(element_type&& value) noexcept
                : ptr(cloner_type()(std::move(value))) {}

            template <class... Args>
            explicit compressed_ptr(std::in_place_t, Args&&... args)
                : ptr(cloner_type()(
                      std::in_place_t{}, std::forward<Args>(args)...)) {}

            template <class U, class... Args>
            explicit compressed_ptr(
                std::in_place_t, std::initializer_list<U> il, Args&&... args)
                : ptr(cloner_type()(
                      std::in_place_t{}, il, std::forward<Args>(args)...)) {}

            compressed_ptr(element_type const& value, cloner_type const& cloner)
                : cloner_type(cloner), ptr(cloner_type()(value)) {}

            compressed_ptr(element_type&& value, cloner_type&& cloner) noexcept
                : cloner_type(std::move(cloner)),
                  ptr(cloner_type()(std::move(value))) {}

            compressed_ptr(
                element_type const& value, cloner_type const& cloner,
                deleter_type const& deleter)
                : cloner_type(cloner), deleter_type(deleter),
                  ptr(cloner_type()(value)) {}

            compressed_ptr(
                element_type&& value, cloner_type&& cloner,
                deleter_type&& deleter) noexcept
                : cloner_type(std::move(cloner)),
                  deleter_type(std::move(deleter)),
                  ptr(cloner_type()(std::move(value))) {}

            explicit compressed_ptr(cloner_type const& cloner)
                : cloner_type(cloner), ptr(nullptr) {}

            explicit compressed_ptr(cloner_type&& cloner) noexcept
                : cloner_type(std::move(cloner)), ptr(nullptr) {}

            explicit compressed_ptr(deleter_type const& deleter)
                : deleter_type(deleter), ptr(nullptr) {}

            explicit compressed_ptr(deleter_type&& deleter) noexcept
                : deleter_type(std::move(deleter)), ptr(nullptr) {}

            compressed_ptr(
                cloner_type const& cloner, deleter_type const& deleter)
                : cloner_type(cloner), deleter_type(deleter), ptr(nullptr) {}

            compressed_ptr(
                cloner_type&& cloner, deleter_type&& deleter) noexcept
                : cloner_type(std::move(cloner)),
                  deleter_type(std::move(deleter)), ptr(nullptr) {}

            // Observers:

            pointer get() const noexcept { return ptr; }

            cloner_type& get_cloner() noexcept { return *this; }

            deleter_type& get_deleter() noexcept { return *this; }

            // Modifiers:

            pointer release() noexcept {
                using std::swap;
                pointer result = nullptr;
                swap(result, ptr);
                return result;
            }

            void reset(pointer p) noexcept {
                get_deleter()(ptr);
                ptr = p;
            }

            void reset(element_type const& v) { reset(get_cloner()(v)); }

            void reset(element_type&& v) { reset(get_cloner()(std::move(v))); }

            void swap(compressed_ptr& other) noexcept {
                using std::swap;
                swap(ptr, other.ptr);
            }

            pointer ptr;
        };

    } // namespace detail

    //
    // value_ptr access error
    //
    class bad_value_access : public std::logic_error {
    public:
        explicit bad_value_access() : logic_error("bad value_ptr access") {}
    };

    //
    // class value_ptr:
    //
    template <
        class T, class Cloner = detail::default_clone<T>,
        class Deleter = detail::default_delete<T>>
    class value_ptr {
    public:
        using element_type    = T;
        using pointer         = T*;
        using reference       = T&;
        using const_pointer   = const T*;
        using const_reference = const T&;
        using cloner_type     = Cloner;
        using deleter_type    = Deleter;

        template <
            typename Up, typename Ep, typename Up_up = value_ptr<Up, Ep>,
            typename Up_element_type = typename Up_up::element_type>
        using safe_conversion_up = std::conjunction<
            std::is_array<Up>, std::is_same<pointer, element_type*>,
            std::is_same<typename Up_up::pointer, Up_element_type*>,
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
            std::is_convertible<Up_element_type (*)[], element_type (*)[]>,
            std::disjunction<
                std::conjunction<
                    std::is_reference<deleter_type>,
                    std::is_same<deleter_type, Ep>>,
                std::disjunction<
                    std::negation<std::is_reference<deleter_type>>,
                    std::is_convertible<Ep, deleter_type>>>>;
        // Lifetime
        ~value_ptr() = default;

        value_ptr() noexcept : ptr(cloner_type(), deleter_type()) {}

        explicit value_ptr(std::nullptr_t) noexcept
            : ptr(cloner_type(), deleter_type()) {}

        explicit value_ptr(pointer p) noexcept : ptr(p) {}

        value_ptr(value_ptr const& other) : ptr(other.ptr) {}

        value_ptr(value_ptr&& other) noexcept : ptr(std::move(other.ptr)) {}

        explicit value_ptr(element_type const& value) : ptr(value) {}

        explicit value_ptr(element_type&& value) noexcept
            : ptr(std::move(value)) {}

        template <
            class... Args,
            typename = std::enable_if_t<std::is_constructible_v<T, Args&&...>>>
        explicit value_ptr(std::in_place_t, Args&&... args)
            : ptr(std::in_place_t{}, std::forward<Args>(args)...) {}

        template <
            class U, class... Args,
            typename = std::enable_if_t<std::is_constructible_v<
                T, std::initializer_list<U>&, Args&&...>>>
        explicit value_ptr(
            std::in_place_t, std::initializer_list<U> il, Args&&... args)
            : ptr(std::in_place_t{}, il, std::forward<Args>(args)...) {}

        explicit value_ptr(cloner_type const& cloner) : ptr(cloner) {}

        explicit value_ptr(cloner_type&& cloner) noexcept
            : ptr(std::move(cloner)) {}

        explicit value_ptr(deleter_type const& deleter) : ptr(deleter) {}

        explicit value_ptr(deleter_type&& deleter) noexcept
            : ptr(std::move(deleter)) {}

        template <class V, class ClonerOrDeleter>
        value_ptr(
            V&& value, ClonerOrDeleter&& cloner_or_deleter,
            std::enable_if<
                !std::is_same_v<std20::remove_cvref<V>, std::in_place_t>,
                void*> = nullptr)
            : ptr(std::forward<V>(value),
                  std::forward<ClonerOrDeleter>(cloner_or_deleter)) {}

        template <class V, class C, class D>
        value_ptr(
            V&& value, C&& cloner, D&& deleter,
            std::enable_if_t<
                !std::is_same_v<std20::remove_cvref_t<V>, std::in_place_t>,
                void*> = nullptr)
            : ptr(std::forward<V>(value), std::forward<C>(cloner),
                  std::forward<D>(deleter)) {}

        value_ptr& operator=(std::nullptr_t) noexcept {
            ptr.reset(nullptr);
            return *this;
        }

        value_ptr& operator=(T const& value) {
            ptr.reset(value);
            return *this;
        }

        template <
            class U,
            typename = std::enable_if_t<std::is_same_v<std::decay_t<U>, T>>>
        value_ptr& operator=(U&& value) {
            ptr.reset(std::forward<U>(value));
            return *this;
        }

        value_ptr& operator=(value_ptr const& rhs) {
            if (this == &rhs) {
                return *this;
            }
            if (rhs) {
                ptr.reset(*rhs);
            } else {
                ptr.reset(nullptr);
            }
            return *this;
        }

        value_ptr& operator=(value_ptr&& rhs) noexcept {
            if (this == &rhs) {
                return *this;
            }
            swap(rhs);

            return *this;
        }

        template <class... Args>
        void emplace(Args&&... args) {
            ptr.reset(T(std::forward<Args>(args)...));
        }

        template <class U, class... Args>
        void emplace(std::initializer_list<U> il, Args&&... args) {
            ptr.reset(T(il, std::forward<Args>(args)...));
        }

        // Observers:
        pointer get() const noexcept { return ptr.get(); }

        cloner_type& get_cloner() noexcept { return ptr.get_cloner(); }

        deleter_type& get_deleter() noexcept { return ptr.get_deleter(); }

        __attribute__((pure))
        reference operator*() const {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            assert(get() != nullptr);
            return *get();
        }

        __attribute__((pure))
        pointer operator->() const noexcept {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            assert(get() != nullptr);
            return get();
        }

        explicit operator bool() const noexcept { return has_value(); }

        bool has_value() const noexcept { return !!get(); }

        element_type const& value() const {
            if (!has_value()) {
                throw bad_value_access();
            }
            return *get();
        }

        element_type& value() {
            if (!has_value()) {
                throw bad_value_access();
            }
            return *get();
        }

        template <class U>
        element_type value_or(U&& v) const {
            return has_value() ? value()
                               : static_cast<element_type>(std::forward<U>(v));
        }

        // Modifiers:
        pointer release() noexcept { return ptr.release(); }

        void reset(pointer p = pointer()) noexcept { ptr.reset(p); }

        void swap(value_ptr& other) noexcept { ptr.swap(other.ptr); }

    private:
        detail::compressed_ptr<T, Cloner, Deleter> ptr;
    };

    // Non-member functions:
    template <class T>
    inline value_ptr<std::decay_t<T>> make_value(T&& v) {
        return value_ptr<std::decay_t<T>>(std::forward<T>(v));
    }

    template <class T, class... Args>
    inline value_ptr<T> make_value(Args&&... args) {
        return value_ptr<T>(in_place, std::forward<Args>(args)...);
    }

    template <class T, class U, class... Args>
    inline value_ptr<T>
    make_value(std::initializer_list<U> il, Args&&... args) {
        return value_ptr<T>(in_place, il, std::forward<Args>(args)...);
    }

    // Comparison between value_ptr-s:
    // compare content:
    template <class T1, class D1, class C1, class T2, class D2, class C2>
    inline bool operator==(
        value_ptr<T1, D1, C1> const& lhs, value_ptr<T2, D2, C2> const& rhs) {
        return bool(lhs) != bool(rhs) ? false
                                      : !bool(lhs) ? true : *lhs == *rhs;
    }

    template <class T1, class D1, class C1, class T2, class D2, class C2>
    inline bool operator!=(
        value_ptr<T1, D1, C1> const& lhs, value_ptr<T2, D2, C2> const& rhs) {
        return !(lhs == rhs);
    }

    template <class T1, class D1, class C1, class T2, class D2, class C2>
    inline bool operator<(
        value_ptr<T1, D1, C1> const& lhs, value_ptr<T2, D2, C2> const& rhs) {
        return (!rhs) ? false : (!lhs) ? true : *lhs < *rhs;
    }

    template <class T1, class D1, class C1, class T2, class D2, class C2>
    inline bool operator<=(
        value_ptr<T1, D1, C1> const& lhs, value_ptr<T2, D2, C2> const& rhs) {
        return !(rhs < lhs);
    }

    template <class T1, class D1, class C1, class T2, class D2, class C2>
    inline bool operator>(
        value_ptr<T1, D1, C1> const& lhs, value_ptr<T2, D2, C2> const& rhs) {
        return rhs < lhs;
    }

    template <class T1, class D1, class C1, class T2, class D2, class C2>
    inline bool operator>=(
        value_ptr<T1, D1, C1> const& lhs, value_ptr<T2, D2, C2> const& rhs) {
        return !(lhs < rhs);
    }

    // compare with value:
    template <class T, class C, class D>
    bool operator==(value_ptr<T, C, D> const& vp, T const& value) {
        return bool(vp) ? *vp == value : false;
    }

    template <class T, class C, class D>
    bool operator==(T const& value, value_ptr<T, C, D> const& vp) {
        return bool(vp) ? value == *vp : false;
    }

    template <class T, class C, class D>
    bool operator!=(value_ptr<T, C, D> const& vp, T const& value) {
        return bool(vp) ? *vp != value : true;
    }

    template <class T, class C, class D>
    bool operator!=(T const& value, value_ptr<T, C, D> const& vp) {
        return bool(vp) ? value != *vp : true;
    }

    template <class T, class C, class D>
    bool operator<(value_ptr<T, C, D> const& vp, T const& value) {
        return bool(vp) ? *vp < value : true;
    }

    template <class T, class C, class D>
    bool operator<(T const& value, value_ptr<T, C, D> const& vp) {
        return bool(vp) ? value < *vp : false;
    }

    template <class T, class C, class D>
    bool operator<=(value_ptr<T, C, D> const& vp, T const& value) {
        return bool(vp) ? *vp <= value : true;
    }

    template <class T, class C, class D>
    bool operator<=(T const& value, value_ptr<T, C, D> const& vp) {
        return bool(vp) ? value <= *vp : false;
    }

    template <class T, class C, class D>
    bool operator>(value_ptr<T, C, D> const& vp, T const& value) {
        return bool(vp) ? *vp > value : false;
    }

    template <class T, class C, class D>
    bool operator>(T const& value, value_ptr<T, C, D> const& vp) {
        return bool(vp) ? value > *vp : true;
    }

    template <class T, class C, class D>
    bool operator>=(value_ptr<T, C, D> const& vp, T const& value) {
        return bool(vp) ? *vp >= value : false;
    }

    template <class T, class C, class D>
    bool operator>=(T const& value, value_ptr<T, C, D> const& vp) {
        return bool(vp) ? value >= *vp : true;
    }

    // swap:
    template <class T, class D, class C>
    inline void
    swap(value_ptr<T, D, C>& lhs, value_ptr<T, D, C>& rhs) noexcept {
        lhs.swap(rhs);
    }
} // namespace nonstd

// Specialize the std::hash algorithm:

namespace std {
    template <class T, class D, class C>
    struct hash<nonstd::value_ptr<T, D, C>> {
        using argument_type = nonstd::value_ptr<T, D, C>;
        using result_type   = size_t;

        result_type operator()(argument_type const& p) const noexcept {
            return hash<typename argument_type::const_pointer>()(p.get());
        }
    };

} // namespace std

#endif // NONSTD_VALUE_PTR_LITE_HPP
