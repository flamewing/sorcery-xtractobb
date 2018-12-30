/*
 *	Copyright © 2016 Flamewing <flamewing.sonic@gmail.com>
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

#ifndef UTIL_HH
#define UTIL_HH

#include <memory>
#include <type_traits>

namespace gsl {
    template <class T, class = std::enable_if_t<std::is_pointer<T>::value>>
    using owner = T;
}

// Ignore warnings for unused variables.
template <typename... Args>
void ignore_unused_variable_warning(Args&...) {}

// Cast operators for unique_ptr.
template <typename TO, typename FROM>
std::unique_ptr<TO> static_unique_pointer_cast(std::unique_ptr<FROM>&& old) {
    return std::unique_ptr<TO>{static_cast<TO*>(old.release())};
}

template <typename TO, typename FROM>
std::unique_ptr<TO> dynamic_unique_pointer_cast(std::unique_ptr<FROM>&& old) {
    return std::unique_ptr<TO>{dynamic_cast<TO*>(old.release())};
}

template <typename TO, typename FROM>
std::unique_ptr<TO>
reinterpret_unique_pointer_cast(std::unique_ptr<FROM>&& old) {
    return std::unique_ptr<TO>{reinterpret_cast<TO*>(old.release())};
}

// Clone suppport, shamelessly copied from Fluent C++.
template <typename T>
class abstract_method {};

template <typename T>
class virtual_inherit_from : virtual public T {
    using T::T;
};

template <typename Derived, typename... Bases>
// NOLINTNEXTLINE(hicpp-special-member-functions,cppcoreguidelines-special-member-functions)
class clone_inherit : public Bases... {
public:
    // Boilerplate
    clone_inherit() noexcept(true)                = default;
    ~clone_inherit() override                     = default;
    clone_inherit(clone_inherit const&)           = default;
    clone_inherit(clone_inherit&&) noexcept(true) = default;
    clone_inherit& operator=(clone_inherit const&) = default;
    clone_inherit& operator=(clone_inherit&&) noexcept(true) = default;
    // Clone method
    std::unique_ptr<Derived> clone() const {
        return std::unique_ptr<Derived>(
            static_cast<Derived*>(this->clone_impl()));
    }

private:
    gsl::owner<clone_inherit*> clone_impl() const override {
        return new Derived(static_cast<const Derived&>(*this));
    }
};

template <typename Derived, typename... Bases>
// NOLINTNEXTLINE(hicpp-special-member-functions,cppcoreguidelines-special-member-functions)
class clone_inherit<abstract_method<Derived>, Bases...> : public Bases... {
public:
    // Boilerplate
    clone_inherit() noexcept(true)                = default;
    virtual ~clone_inherit()                      = default;
    clone_inherit(clone_inherit const&)           = default;
    clone_inherit(clone_inherit&&) noexcept(true) = default;
    clone_inherit& operator=(clone_inherit const&) = default;
    clone_inherit& operator=(clone_inherit&&) noexcept(true) = default;
    // Clone method
    std::unique_ptr<Derived> clone() const {
        return std::unique_ptr<Derived>(
            static_cast<Derived*>(this->clone_impl()));
    }

private:
    virtual gsl::owner<clone_inherit*> clone_impl() const = 0;
};

template <typename Derived>
// NOLINTNEXTLINE(hicpp-special-member-functions,cppcoreguidelines-special-member-functions)
class clone_inherit<Derived> {
public:
    // Boilerplate
    clone_inherit() noexcept(true)                = default;
    virtual ~clone_inherit()                      = default;
    clone_inherit(clone_inherit const&)           = default;
    clone_inherit(clone_inherit&&) noexcept(true) = default;
    clone_inherit& operator=(clone_inherit const&) = default;
    clone_inherit& operator=(clone_inherit&&) noexcept(true) = default;
    // Clone method
    std::unique_ptr<Derived> clone() const {
        return std::unique_ptr<Derived>(
            static_cast<Derived*>(this->clone_impl()));
    }

private:
    virtual gsl::owner<clone_inherit*> clone_impl() const {
        return new Derived(static_cast<const Derived&>(*this));
    }
};

template <typename Derived>
// NOLINTNEXTLINE(hicpp-special-member-functions,cppcoreguidelines-special-member-functions)
class clone_inherit<abstract_method<Derived>> {
public:
    // Boilerplate
    clone_inherit() noexcept(true)                = default;
    virtual ~clone_inherit()                      = default;
    clone_inherit(clone_inherit const&)           = default;
    clone_inherit(clone_inherit&&) noexcept(true) = default;
    clone_inherit& operator=(clone_inherit const&) = default;
    clone_inherit& operator=(clone_inherit&&) noexcept(true) = default;
    // Clone method
    std::unique_ptr<Derived> clone() const {
        return std::unique_ptr<Derived>(
            static_cast<Derived*>(this->clone_impl()));
    }

private:
    virtual gsl::owner<clone_inherit*> clone_impl() const = 0;
};
#endif
