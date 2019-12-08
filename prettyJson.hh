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

#include "jsont.hh"

#include <boost/interprocess/streams/vectorstream.hpp>
#include <boost/iostreams/filter/aggregate.hpp>
#include <iostream>

using vectorstream = boost::interprocess::basic_vectorstream<std::vector<char>>;

#ifndef INDENT_CHAR
#    define INDENT_CHAR '\t'
#endif

template <typename Src, typename Dst>
void printJSON(Src const& data, Dst& sint, PrettyJSON const pretty) {
    jsont::Tokenizer reader(data.data(), data.size());
    size_t           indent    = 0;
    bool             needValue = false;
    jsont::Token     tok       = reader.current();

    auto printIndentedValue = [&needValue, &sint, &indent,
                               &pretty](auto valuePrinter, bool newNeedValue) {
        if (pretty == ePRETTY && (newNeedValue || !needValue)) {
            sint << std::string(indent, INDENT_CHAR);
        }
        needValue = newNeedValue;
        valuePrinter();
    };
    auto printValueRaw = [&sint, &reader]() -> decltype(auto) {
        return sint << reader.dataValue();
    };
    auto printValueObject
            = [&printValueRaw, &sint, &reader, &pretty]() -> decltype(auto) {
        printValueRaw() << ':';
        if (pretty != eNO_WHITESPACE) {
            sint << ' ';
        }
        return sint;
    };
    auto lineBreak = [&indent, &sint, &tok, &pretty]() {
        if (tok != jsont::Comma && (indent == 0 || pretty == ePRETTY)) {
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
                printValueRaw();
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
        case jsont::String:
            printIndentedValue(printValueRaw, false);
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

// JSON pretty-print filter for boost::filtering_ostream
template <typename Ch, typename Alloc = std::allocator<Ch>>
class basic_json_filter : public boost::iostreams::aggregate_filter<Ch, Alloc> {
private:
    using base_type = boost::iostreams::aggregate_filter<Ch, Alloc>;

public:
    using char_type = typename base_type::char_type;
    using category  = typename base_type::category;

    explicit basic_json_filter(PrettyJSON _pretty, size_t* _length = nullptr)
            : pretty(_pretty)
            , length(_length) {}

private:
    using vector_type = typename base_type::vector_type;
    void set_length(size_t value) {
        if (length != nullptr) {
            *length = value;
        }
    }
    void do_filter(vector_type const& src, vector_type& dest) final {
        if (src.empty()) {
            set_length(0);
            return;
        }
        vectorstream sint(std::ios::in | std::ios::out | std::ios::binary);
        sint.reserve(src.size() * 3 / 2);
        printJSON(src, sint, pretty);
        sint.swap_vector(dest);
        set_length(dest.size());
    }
    PrettyJSON const pretty;
    size_t*          length;
};
// NOLINTNEXTLINE(modernize-use-trailing-return-type)
BOOST_IOSTREAMS_PIPABLE(basic_json_filter, 2)

using json_filter  = basic_json_filter<char>;
using wjson_filter = basic_json_filter<wchar_t>;

#endif    // PRETTY_JSON_H
