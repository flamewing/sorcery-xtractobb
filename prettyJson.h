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
#ifndef PRETTY_JSON_H
#define PRETTY_JSON_H

enum PrettyJSON { eNO_WHITESPACE = -1, ePRETTY = 0, eCOMPACT = 1 };

#include "jsont.h"

#include <iostream>

#ifndef INDENT_CHAR
#    define INDENT_CHAR '\t'
#endif

template <typename Src, typename Dst>
void printJSON(Src const& data, Dst& sint, PrettyJSON const pretty) {
    jsont::Tokenizer reader(data.data(), data.size());
    size_t           indent             = 0;
    bool             needValue          = false;
    jsont::Token     tok                = reader.current();
    auto             printIndentedValue = [&needValue, &sint, &indent,
                               pretty](auto valuePrinter, bool newNeedValue) {
        if (pretty == ePRETTY && (newNeedValue || !needValue)) {
            sint << std::string(indent, INDENT_CHAR);
        }
        needValue = newNeedValue;
        valuePrinter();
    };
    auto printValueRaw = [&sint, &reader]() -> decltype(auto) {
        return sint << reader.dataValue();
    };
    auto printValueQuoted = [&sint, &reader]() -> decltype(auto) {
        return sint << '"' << reader.dataValue() << '"';
    };
    auto printValueObject = [&sint, &printValueQuoted,
                             pretty]() -> decltype(auto) {
        printValueQuoted() << ':';
        if (pretty != eNO_WHITESPACE) {
            sint << ' ';
        }
        return sint;
    };
    auto lineBreak = [&sint, &tok, pretty]() {
        if (tok != jsont::Comma && pretty == ePRETTY) {
            sint << '\n';
        }
    };
    while (true) {
        switch (tok) {
        case jsont::Error:
            std::cerr << reader.errorMessage() << std::endl;
            [[fallthrough]];
        case jsont::End:
            return;
        case jsont::ObjectStart:
        case jsont::ArrayStart: {
            printIndentedValue(printValueRaw, false);
            auto const next = static_cast<jsont::Token>(
                static_cast<uint8_t>(tok) + uint8_t(1));
            tok = reader.next();
            if (tok == next) {
                sint << reader.dataValue();
                break;
            }
            indent++;
            lineBreak();
            continue;
        }
        case jsont::ObjectEnd:
        case jsont::ArrayEnd:
            --indent;
            [[fallthrough]];
        case jsont::True:
        case jsont::False:
        case jsont::Null:
        case jsont::Integer:
        case jsont::Float:
            printIndentedValue(printValueRaw, false);
            break;
        case jsont::String:
            printIndentedValue(printValueQuoted, false);
            break;
        case jsont::FieldName:
            printIndentedValue(printValueObject, true);
            tok = reader.next();
            continue;
        case jsont::Comma:
            sint << ',';
            break;
        }
        tok = reader.next();
        lineBreak();
    }
    __builtin_unreachable();
}

#endif // PRETTY_JSON_H
