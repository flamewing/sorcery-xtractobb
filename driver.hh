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

#ifndef DRIVER_HH
#define DRIVER_HH

#include <iosfwd>
#include <string>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wsuggest-attribute=malloc"
#pragma GCC diagnostic ignored "-Wsuggest-attribute=pure"
#pragma GCC diagnostic ignored "-Wsuggest-final-methods"
#pragma GCC diagnostic ignored "-Wsuggest-final-types"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#endif
#include "parser.hh"
#pragma GCC diagnostic pop

// Give Flex the prototype of yylex we want ...
#define YY_DECL yy::parser::symbol_type yylex(driver& drv)
// ... and declare it for the parser's sake.
YY_DECL;

// Conducting the whole scanning and parsing of Calc++.
class driver {
public:
    explicit driver(std::ostream& out);
    // Run the parser on file F.  Return 0 on success.
    int parse(const std::string& f);
    // Handling the scanner.
    void scan_begin();
    void scan_end();
    void putIndent();

    // Output stream
    std::ostream& out;
    // The name of the file being parsed.
    std::string file;
    // Current indentation level
    size_t indent = 0;
    // Whether to break line before values
    bool need_break = false;
    // Whether to generate parser debug traces.
    bool trace_parsing = false;
    // Whether to generate scanner debug traces.
    bool trace_scanning = false;
    // The token's location used by the scanner.
    yy::location location;
};

#endif
