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

#endif
