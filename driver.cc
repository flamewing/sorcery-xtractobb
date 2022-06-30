/*
 *	Copyright © 2018 Flamewing <flamewing.sonic@gmail.com>
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

#include "driver.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wnull-dereference"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#ifndef __clang__
#    pragma GCC diagnostic ignored "-Wsuggest-attribute=malloc"
#    pragma GCC diagnostic ignored "-Wsuggest-attribute=pure"
#    pragma GCC diagnostic ignored "-Wsuggest-final-methods"
#    pragma GCC diagnostic ignored "-Wsuggest-final-types"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
#include "parser.hh"
#pragma GCC diagnostic pop

driver::driver(std::ostream& out_) : out(out_) {}

auto driver::parse(const std::string& path) -> int {
    file = path;
    location.initialize(&file);
    scan_begin();
    yy::parser parse(*this);
    parse.set_debug_level(
            static_cast<yy::parser::debug_level_type>(trace_parsing));
    int res = parse.parse();
    scan_end();
    return res;
}

void driver::putIndent() {
    if (indent != 0) {
        out << std::string(indent, '\t');
    }
}
