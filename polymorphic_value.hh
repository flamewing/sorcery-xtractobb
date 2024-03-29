/*

Copyright (c) 2016 Jonathan B. Coe

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#pragma once

#include <cassert>
#include <exception>
#include <memory>
#include <type_traits>
#include <typeinfo>

namespace nonstd {
    namespace detail {
        ////////////////////////////////////////////////////////////////////////////
        // Implementation detail classes
        ////////////////////////////////////////////////////////////////////////////

        template <class T>
        struct default_copy {
            auto operator()(const T& t) const -> T* {
                // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
                return new T(t);
            }
        };

        template <class T>
        struct default_delete {
            void operator()(const T* t) const {
                // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
                delete t;
            }
        };

        template <class T>
        struct control_block {
            constexpr control_block() noexcept      = default;
            virtual ~control_block()                = default;
            control_block(control_block const&)     = default;
            control_block(control_block&&) noexcept = default;

            auto operator=(control_block const&) -> control_block& = default;
            auto operator=(control_block&&) noexcept
                    -> control_block& = default;

            [[nodiscard]] virtual auto clone() const
                    -> std::unique_ptr<control_block> = 0;

            virtual auto ptr() -> T* = 0;
        };

        template <class T, class U = T>
        class direct_control_block final : public control_block<T> {
            static_assert(!std::is_reference_v<U>);
            U u_;

        public:
            template <class... Ts>
            explicit direct_control_block(Ts&&... ts)
                    : u_(U(std::forward<Ts>(ts)...)) {}

            [[nodiscard]] auto clone() const
                    -> std::unique_ptr<control_block<T>> override {
                return std::make_unique<direct_control_block>(*this);
            }

            auto ptr() -> T* override {
                return std::addressof(u_);
            }
        };

        template <
                class T, class U, class C = default_copy<U>,
                class D = default_delete<U>>
        class pointer_control_block final : public control_block<T>, public C {
            std::unique_ptr<U, D> p_;

        public:
            explicit pointer_control_block(U* u, C c = C{}, D d = D{})
                    : C(std::move(c)), p_(u, std::move(d)) {}

            explicit pointer_control_block(std::unique_ptr<U, D> p, C c = C{})
                    : C(std::move(c)), p_(std::move(p)) {}

            auto clone() const -> std::unique_ptr<control_block<T>> override {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
                assert(p_);
                return std::make_unique<pointer_control_block>(
                        C::operator()(*p_), static_cast<const C&>(*this),
                        p_.get_deleter());
            }

            auto ptr() -> T* override {
                return p_.get();
            }
        };

        template <class T, class U>
        class delegating_control_block final : public control_block<T> {
            std::unique_ptr<control_block<U>> delegate_;

        public:
            explicit delegating_control_block(
                    std::unique_ptr<control_block<U>> b)
                    : delegate_(std::move(b)) {}

            auto clone() const -> std::unique_ptr<control_block<T>> override {
                return std::make_unique<delegating_control_block>(
                        delegate_->clone());
            }

            auto ptr() -> T* override {
                return delegate_->ptr();
            }
        };

    }    // end namespace detail

    class bad_polymorphic_value_construction : std::exception {
    public:
        bad_polymorphic_value_construction() noexcept = default;

        [[nodiscard]] auto what() const noexcept -> const char* override {
            return "Dynamic and static type mismatch in polymorphic_value "
                   "construction";
        }
    };

    template <class T>
    class polymorphic_value;

    template <class T>
    struct is_polymorphic_value : std::false_type {};

    template <class T>
    struct is_polymorphic_value<polymorphic_value<T>> : std::true_type {};

    template <class T>
    constexpr inline bool const is_polymorphic_value_v
            = is_polymorphic_value<T>::value;

    ////////////////////////////////////////////////////////////////////////////////
    // `polymorphic_value` class definition
    ////////////////////////////////////////////////////////////////////////////////

    template <class T>
    class polymorphic_value {
        static_assert(!std::is_union_v<T>);
        static_assert(std::is_class_v<T>);

        template <class U>
        friend class polymorphic_value;

        template <class T_, class U, class... Ts>
        // NOLINTNEXTLINE(readability-redundant-declaration)
        friend auto make_polymorphic_value(Ts&&... ts) -> polymorphic_value<T_>;
        template <class T_, class... Ts>
        // NOLINTNEXTLINE(readability-redundant-declaration)
        friend auto make_polymorphic_value(Ts&&... ts) -> polymorphic_value<T_>;

        T* ptr_ = nullptr;

        std::unique_ptr<detail::control_block<T>> cb_;

    public:
        //
        // Destructor
        //

        ~polymorphic_value() noexcept = default;

        //
        // Constructors
        //

        constexpr polymorphic_value() noexcept = default;

        template <
                class U, class C = detail::default_copy<U>,
                class D = detail::default_delete<U>,
                class V = std::enable_if_t<std::is_convertible_v<U*, T*>>>
        explicit polymorphic_value(U* u, C copier = C{}, D deleter = D{}) {
            if (!u) {
                return;
            }

            if (std::is_same_v<
                        D,
                        detail::default_delete<
                                U>> && std::is_same_v<C, detail::default_copy<U>> && typeid(*u) != typeid(U)) {
                throw bad_polymorphic_value_construction();
            }
            std::unique_ptr<U, D> p(u, std::move(deleter));

            cb_ = std::make_unique<detail::pointer_control_block<T, U, C, D>>(
                    std::move(p), std::move(copier));
            ptr_ = u;
        }

        //
        // Copy-constructors
        //

        polymorphic_value(const polymorphic_value& p) {
            if (!p) {
                return;
            }
            auto tmp_cb = p.cb_->clone();
            ptr_        = tmp_cb->ptr();
            cb_         = std::move(tmp_cb);
        }

        //
        // Move-constructors
        //

        polymorphic_value(polymorphic_value&& p) noexcept
                : ptr_(p.ptr_), cb_(std::move(p.cb_)) {
            p.ptr_ = nullptr;
        }

        //
        // Converting constructors
        //

        template <
                class U,
                class V = std::enable_if_t<
                        !std::is_same_v<T, U> && std::is_convertible_v<U*, T*>>>
        explicit polymorphic_value(const polymorphic_value<U>& p) {
            polymorphic_value<U> tmp(p);
            ptr_ = tmp.ptr_;
            cb_  = std::make_unique<detail::delegating_control_block<T, U>>(
                    std::move(tmp.cb_));
        }

        template <
                class U,
                class V = std::enable_if_t<
                        !std::is_same_v<T, U> && std::is_convertible_v<U*, T*>>>
        explicit polymorphic_value(polymorphic_value<U>&& p)
                : ptr_(p.ptr_),
                  cb_(std::make_unique<detail::delegating_control_block<T, U>>(
                          std::move(p.cb_))) {
            p.ptr_ = nullptr;
        }

        //
        // Forwarding constructor
        //

        template <
                class U,
                class V = std::enable_if_t<
                        std::is_convertible_v<
                                std::decay_t<U>*,
                                T*> && !is_polymorphic_value_v<std::decay_t<U>>>>
        explicit polymorphic_value(U&& u)
                : cb_(std::make_unique<
                        detail::direct_control_block<T, std::decay_t<U>>>(
                        std::forward<U>(u))) {
            ptr_ = cb_->ptr();
        }

        //
        // Assignment
        //

        auto operator=(const polymorphic_value& p) -> polymorphic_value& {
            if (std::addressof(p) == this) {
                return *this;
            }

            if (!p) {
                cb_.reset();
                ptr_ = nullptr;
                return *this;
            }

            auto tmp_cb = p.cb_->clone();
            ptr_        = tmp_cb->ptr();
            cb_         = std::move(tmp_cb);
            return *this;
        }

        //
        // Move-assignment
        //

        auto operator=(polymorphic_value&& p) noexcept -> polymorphic_value& {
            if (std::addressof(p) == this) {
                return *this;
            }

            cb_    = std::move(p.cb_);
            ptr_   = p.ptr_;
            p.ptr_ = nullptr;
            return *this;
        }

        //
        // Modifiers
        //

        void swap(polymorphic_value& p) noexcept {
            using std::swap;
            swap(ptr_, p.ptr_);
            swap(cb_, p.cb_);
        }

        //
        // Observers
        //

        explicit operator bool() const {
            return static_cast<bool>(cb_);
        }

        __attribute__((pure)) auto operator->() const -> const T* {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            assert(ptr_);
            return ptr_;
        }

        __attribute__((pure)) auto operator*() const -> const T& {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            assert(*this);
            return *ptr_;
        }

        __attribute__((pure)) auto operator->() -> T* {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            assert(*this);
            return ptr_;
        }

        __attribute__((pure)) auto operator*() -> T& {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            assert(*this);
            return *ptr_;
        }
    };

    //
    // polymorphic_value creation
    //
    template <class T, class... Ts>
    auto make_polymorphic_value(Ts&&... ts) -> polymorphic_value<T> {
        polymorphic_value<T> p;
        p.cb_ = std::make_unique<detail::direct_control_block<T, T>>(
                std::forward<Ts>(ts)...);
        p.ptr_ = p.cb_->ptr();
        return std::move(p);
    }
    template <class T, class U, class... Ts>
    auto make_polymorphic_value(Ts&&... ts) -> polymorphic_value<T> {
        polymorphic_value<T> p;
        p.cb_ = std::make_unique<detail::direct_control_block<T, U>>(
                std::forward<Ts>(ts)...);
        p.ptr_ = p.cb_->ptr();
        return std::move(p);
    }

    //
    // non-member swap
    //
    template <class T>
    void swap(polymorphic_value<T>& t, polymorphic_value<T>& u) noexcept {
        t.swap(u);
    }
}    // namespace nonstd
