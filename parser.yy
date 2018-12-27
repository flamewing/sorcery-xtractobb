%{
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
%}

%skeleton "lalr1.cc"
%require "3.0.4"
%defines

%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
    #include <string>
    #include <string_view>
	class driver;

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

// The parsing context.
%param { driver& drv }

%locations

%define parse.trace
%define parse.error verbose

%code {
    #include <ostream>
    #include <string_view>

    using std::string_view;
    using namespace std::literals::string_view_literals;

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

    #include "driver.hh"
}

%define api.token.prefix {TOK_}
%token
    END             0  "end of file"
    LCURLY          "{"
    RCURLY          "}"
    LSQUARE         "["
    RSQUARE         "]"
    COMMA           ","
    COLON           ":"
    VARIABLES       "variables block"
    BUILDINGBLOCKS  "building blocks"
    STITCHES        "knot stitches"
;

%token <std::string> BOOL    "boolean"
%token <std::string> NULL    "null"
%token <std::string> NUMBER  "number"
%token <std::string> STRING  "string"
%token <std::string> INITIAL "initial"

%type <std::string> varName
%type <std::string> varValue
%type <std::string> strings

%type <bool> JsonMapValueListOpt JsonArrayValueListOpt

%printer { yyo << $$; } <*>;

%%
%start unit;
unit
    : LCURLY                    {   drv.indent++;   }
        variables COMMA         {   drv.out << "\n";  }
        buildingBlocks COMMA    {   drv.out << ',';   }
        initialFunction COMMA   {   drv.out << ',';   }
        stitches                {   drv.out << '\n';    }
      RCURLY                    {   drv.indent--;   }
    ;

variables
    : VARIABLES COLON LCURLY
        {   drv.out << "// Variables\n";    }
        varList RCURLY
    ;

varList
    : varList COMMA varDecl
    | varDecl
    ;

varDecl
    : varName COLON varValue
        {   drv.out << "VAR " << $1 << " = " << $3 << '\n'; }
    ;

strings
    : STRING
        {   $$ = $1.substr(1, $1.size()-2);    }
    | INITIAL
        {   $$ = $1.substr(1, $1.size()-2);    }
    ;

varName
    : strings
        {   $$ = $1.substr(1, $1.size()-2);    }
    ;

varValue
    : BOOL
        {   $$ = $1;    }
    | NULL
        {   $$ = $1;    }
    | NUMBER
        {   $$ = $1;    }
    | strings
        {   $$ = $1;    }
    ;

buildingBlocks
    : BUILDINGBLOCKS COLON
        {
            drv.out << '\n';
            drv.putIndent();
            drv.out << R"("buildingBlocks": )";
        }
      JsonObject
    ;

initialFunction
    : INITIAL COLON STRING
        {
            drv.out << '\n';
            drv.putIndent();
            drv.out << $1 << ": " << $3;
        }
    ;

stitches
    : STITCHES COLON
        {
            drv.out << '\n';
            drv.putIndent();
            drv.out << R"("stitches": )";
        }
      JsonMap
    ;

// JSON for remaining unimplemented parts
JsonObject
    : JsonMap
    | JsonArray
    ;

JsonMap
    : LCURLY
        {
            if (drv.need_break) {
                drv.out << '\n';
                drv.putIndent();
            }
            drv.out << '{';
            drv.indent++;
            drv.need_break = false;
        }
      JsonMapValueListOpt
      RCURLY
        {
            drv.need_break = false;
            drv.indent--;
            if ($3) {
                drv.out << '}';
            } else {
                drv.out << '\n';
                drv.putIndent();
                drv.out << '}';
            }
        }
    ;

JsonMapValueListOpt
    : JsonMapValueList
        {   $$ = false; }
    | /* empty */
        {   $$ = true;  }
    ;

JsonMapValueList
    : JsonMapValueList COMMA
        {   drv.out << ','; }
      JsonMapValue
    | JsonMapValue
    ;

JsonMapValue
    : STRING COLON
        {
            drv.out << '\n';
            drv.putIndent();
            drv.out << $1 << ": ";
        }
      JsonValue
    | initialFunction
    | stitches
    ;

JsonArray
    : LSQUARE
        {
            if (drv.need_break) {
                drv.out << '\n';
                drv.putIndent();
            }
            drv.out << '[';
            drv.indent++;
            drv.need_break = true;
        }
      JsonArrayValueListOpt
      RSQUARE
        {
            drv.need_break = false;
            drv.indent--;
            if ($3) {
                drv.out << ']';
            } else {
                drv.out << '\n';
                drv.putIndent();
                drv.out << ']';
            }
        }
    ;

JsonArrayValueListOpt
    : JsonArrayValueList
        {   $$ = false; }
    | /* empty */
        {   $$ = true;  }
    ;

JsonArrayValueList
    : JsonArrayValueList COMMA
        {
            drv.out << ",\n";
            drv.putIndent();
        }
      JsonValue
    | JsonValue
    ;

JsonValue
    : JsonMap
    | JsonArray
    | varValue
        {
            if (drv.need_break) {
                drv.out << '\n';
                drv.putIndent();
                drv.need_break = false;
            }
            drv.out << $1;
        }
    ;

%%

/* Grammar:
 * unit           := LCURLY
 *                      variables COMMA
 *                      buildingBlocks COMMA
 *                      initial COMMA
 *                      stitches
 *                   RCURLY
 *
 * variables      := "variables" COLON LCURLY
 *                      varList
 *                   RCURLY
 *
 * varList        := varList COMMA varDecl
 *                 | varDecl
 *
 * varDecl        := FieldName COLON String
 *
 * buildingBlocks := "buildingBlocks" COLON LCURLY
 *                      blockList
 *                   RCURLY
 *
 * blockList      := blockList COMMA block
 *                 | block
 *
 * block          := LSQUARE statementList RSQUARE
 *
 * statementList  := statementList COMMA statement
 *                 | statement
 *
 * statement      := LCURLY blockstmt RCURLY
 *
 * blockstmt      := doFuncs
 *                 | action
 *                 | condition
 *                 | return
 *                 | callBlock
 *                 | String
 *                 | cycle
 *                 | sequence
 *
 * doFuncs        := "doFuncs" COLON LSQUARE
 *                      doFunc
 *                   RSQUARE
 *
 * doFunc         := LCURLY
 *                      callable COMMA "params" COLON optParamList
 *                   RCURLY
 *                 | LCURLY set RCURLY
 *
 * callable       := func
 *                 | buildingBlock
 *
 * optParamList   := LSQUARE
 *                      paramList
 *                   RSQUARE
 *                 | LCURLY RCURLY
 *
 * paramList      := paramList COMMA param
 *                 | param
 *
 * param          := Expression
 *
 * func           := "func" COLON function
 *
 * function       := "Add"
 *                 | "Subtract"
 *                 | "Increment"
 *                 | "Decrement"
 *                 | "Divide"
 *                 | "Mod"
 *                 | "Multiply"
 *                 | "Log10"
 *                 | "And"
 *                 | "Or"
 *                 | "Not"
 *                 | "FlagIsSet"
 *                 | "FlagIsNotSet"
 *                 | "HasNotRead"
 *                 | "HasRead"
 *                 | "Equals"
 *                 | "NotEquals"
 *                 | "GreaterThan"
 *                 | "GreaterThanOrEqualTo"
 *                 | "LessThan"
 *                 | "LessThanOrEqualTo"
 *
 * buildingBlock  := "buildingBlock" COLON String
 *
 * action         := "action" COLON actionName optUserInfo
 *
 * optUserInfo    := COMMA "userInfo" COLON LCURLY
 *                      userInfoList
 *                   RCURLY
 *                 | // empty
 *
 * userInfoList   := userInfoList COMMA userInfo
 *                 | userInfo
 *
 * userInfo       := FieldName COLON Expression
 *
 * actionName     := "ActiveRewindTooltip"
 *                 | "Analytic"
 *                 | "bookFinished"
 *                 | "CustomPlayerImage"
 *                 | "CustomPlayerImage"
 *                 | "Dead"
 *                 | "Dice"
 *                 | "Fight"
 *                 | "Magic"
 *                 | "Map"
 *                 | "PlaceMapObject"
 *                 | "PlayAudio"
 *                 | "RecordRewindPersistentVariable"
 *                 | "RemoveMapObject"
 *                 | "RotateTower"
 *                 | "saveState"
 *                 | "SetFixedMapLayer"
 *                 | "SetModel"
 *                 | "setTweet"
 *                 | "TimeRewind"
 *                 | "Transition"
 *
 * set            := "set" COLON LSQUARE
 *                      String COMMA Expression
 *                   RSQUARE
 *
 * initial        := FieldName COLON String COMMA
 */

void yy::parser::error(const location_type& l, const std::string& m) {
    std::cerr << l << ": " << m << '\n';
}
