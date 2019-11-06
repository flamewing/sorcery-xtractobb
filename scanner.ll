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
    std::string genString(const char* text, size_t len) {
        std::string ret;
        ret.reserve(len);
        for (size_t ii = 0; ii < len; ii++) {
            if (char c = text[ii]; c != '\\') {
                ret += c;
                continue;
            }
            ++ii;
            // assert(ii < len);  // Scanner made sure of this
            switch (char c = text[ii]; c) {
            case 'u':
                ret += '\\';
                [[fallthrough]];
            case '"':
            case '\\':
            case '/':
                ret += c;
                break;
            case 'b':
                ret += '\b';
                break;
            case 'f':
                ret += '\f';
                break;
            case 'n':
                ret += '\n';
                break;
            case 'r':
                ret += '\r';
                break;
            case 't':
                ret += '\t';
                break;
            };
        }
        return ret;
    }
%}

%option noyywrap nounput noinput batch debug
%option noyy_top_state

%option stack
%s math

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

\"get\"             return yy::parser::make_GET(loc);
\"set\"             return yy::parser::make_SET(loc);
\"func\"            return yy::parser::make_FUNC(loc);
\"params\"          return yy::parser::make_PARAMS(loc);
\"return\"          return yy::parser::make_RETURN(loc);
\"condition\"       return yy::parser::make_CONDITION(loc);
\"then\"            return yy::parser::make_THEN(loc);
\"otherwise\"       return yy::parser::make_OTHERWISE(loc);


<math>\"Add\"                     return yy::parser::make_ADD(loc);
<math>\"Subtract\"                return yy::parser::make_SUB(loc);
<math>\"Increment\"               return yy::parser::make_INC(loc);
<math>\"Decrement\"               return yy::parser::make_DEC(loc);
<math>\"Divide\"                  return yy::parser::make_DIV(loc);
<math>\"Mod\"                     return yy::parser::make_MOD(loc);
<math>\"Multiply\"                return yy::parser::make_MUL(loc);
<math>\"Log10\"                   return yy::parser::make_LOG(loc);
<math>\"And\"                     return yy::parser::make_AND(loc);
<math>\"Or\"                      return yy::parser::make_OR(loc);
<math>\"Not\"                     return yy::parser::make_NOT(loc);
<math>\"FlagIsSet\"               return yy::parser::make_FLAGSET(loc);
<math>\"FlagIsNotSet\"            return yy::parser::make_FLAGCLEAR(loc);
<math>\"HasNotRead\"              return yy::parser::make_NOTREAD(loc);
<math>\"HasRead\"                 return yy::parser::make_HASREAD(loc);
<math>\"Equals\"                  return yy::parser::make_EQ(loc);
<math>\"NotEquals\"               return yy::parser::make_NE(loc);
<math>\"GreaterThan\"             return yy::parser::make_GT(loc);
<math>\"GreaterThanOrEqualTo\"    return yy::parser::make_GE(loc);
<math>\"LessThan\"                return yy::parser::make_LT(loc);
<math>\"LessThanOrEqualTo\"       return yy::parser::make_LE(loc);

\"variables\"       return yy::parser::make_VARIABLES(loc);
\"buildingBlocks\"  return yy::parser::make_BUILDINGBLOCKS(loc);
\"initial\"         return yy::parser::make_INITIAL("initial"s, loc);
\"stitches\"        return yy::parser::make_STITCHES(loc);

"{"                 return yy::parser::make_LCURLY(loc);
"}"                 return yy::parser::make_RCURLY(loc);
"["                 return yy::parser::make_LSQUARE(loc);
"]"                 return yy::parser::make_RSQUARE(loc);
","                 return yy::parser::make_COMMA(loc);
":"                 return yy::parser::make_COLON(loc);
true                return yy::parser::make_BOOL("true"s, loc);
false               return yy::parser::make_BOOL("false"s, loc);
null                return yy::parser::make_NULL("null"s, loc);
{STRING}            return yy::parser::make_STRING(genString(yytext+1, yyleng-2), loc);
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

void start_math() {
    yy_push_state(math);
}
void end_math() {
    yy_pop_state();
}
