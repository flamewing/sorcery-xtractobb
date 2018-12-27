%top {
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
    #pragma GCC diagnostic ignored "-Wnull-dereference"
    #pragma GCC diagnostic ignored "-Wold-style-cast"
    #pragma GCC diagnostic ignored "-Wsign-conversion"
    #pragma GCC diagnostic ignored "-Wsign-compare"
    #pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
    #ifndef __clang__
    #   pragma GCC diagnostic ignored "-Wsuggest-attribute=malloc"
    #   pragma GCC diagnostic ignored "-Wsuggest-attribute=pure"
    #   pragma GCC diagnostic ignored "-Wsuggest-final-methods"
    #   pragma GCC diagnostic ignored "-Wsuggest-final-types"
    #   pragma GCC diagnostic ignored "-Wuseless-cast"
    #endif
}
%{
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

    #include <cerrno>
    #include <cstdlib>
    #include <cstring>
    #include <string>
    #include <string_view>

    #include "driver.hh"
    #include "parser.hh"

    using namespace std::literals::string_literals;
%}

%option noyywrap nounput noinput batch debug

DIGIT           [0-9]
ONENINE         [1-9]
INT             -?(0|{ONENINE}{DIGIT}*)
FRAC            "."{DIGIT}+
SIGN            [+-]
EXP             [eE]{SIGN}?{DIGIT}+
NUMBER          {INT}{FRAC}?{EXP}?

UNICODE         \\u[0-9A-Fa-f]{4}
ESCAPECHAR      \\["\\/bfnrt]
CHAR            [^"\\]|{ESCAPECHAR}|{UNICODE}
STRING          \"{CHAR}*\"

BLANK           [ \t\r]
EOL             \r?\n

%{
    // Includes go here
    // Code run each time a pattern is matched.
    #define YY_USER_ACTION loc.columns(yyleng);
%}

%%

%{
    // A handy shortcut to the location held by the driver.
    yy::location& loc = drv.location;
    // Code run each time yylex is called.
    loc.step();
%}

\"variables\"       return yy::parser::make_VARIABLES(loc);
\"buildingBlocks\"  return yy::parser::make_BUILDINGBLOCKS(loc);
\"initial\"         return yy::parser::make_INITIAL(std::string(yytext, yyleng), loc);
\"stitches\"        return yy::parser::make_STITCHES(loc);
"{"                 return yy::parser::make_LCURLY(loc);
"}"                 return yy::parser::make_RCURLY(loc);
"["                 return yy::parser::make_LSQUARE(loc);
"]"                 return yy::parser::make_RSQUARE(loc);
","                 return yy::parser::make_COMMA(loc);
":"                 return yy::parser::make_COLON(loc);
true                return yy::parser::make_BOOL(std::string(yytext, yyleng), loc);
false               return yy::parser::make_BOOL(std::string(yytext, yyleng), loc);
null                return yy::parser::make_NULL(std::string(yytext, yyleng), loc);
{STRING}            return yy::parser::make_STRING(std::string(yytext, yyleng), loc);
{NUMBER}            return yy::parser::make_NUMBER(std::string(yytext, yyleng), loc);
{BLANK}+            loc.step();
{EOL}+              loc.lines(yyleng); loc.step();
.                   {
                        throw yy::parser::syntax_error(loc, "invalid character: "s + *yytext);
                    }
<<EOF>>             return yy::parser::make_END(loc);
%%

#pragma GCC diagnostic pop

void driver::scan_begin() {
    yy_flex_debug = trace_scanning;
    if (file.empty() || file == "-") {
        yyin = stdin;
    } else if (!(yyin = fopen(file.c_str(), "r"))) {
        std::cerr << "cannot open " << file << ": " << strerror(errno) << '\n';
        exit(EXIT_FAILURE);
    }
}

void driver::scan_end() { fclose(yyin); }
