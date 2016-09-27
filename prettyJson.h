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

enum PrettyJSON {
	eNO_WHITESPACE = -1,
	ePRETTY = 0,
	eCOMPACT = 1
};

#include "jsont.h"
using std::string;

#ifndef INDENT_CHAR
#define INDENT_CHAR '\t'
#endif

template <typename Src, typename Dst>
void printJSON(Src const& data, Dst& sint, PrettyJSON pretty) {
	jsont::Tokenizer reader(data.data(), data.size());
	size_t indent = 0;
	bool needValue = false;
	jsont::Token tok = reader.current();
	while (tok != jsont::End) {
		switch (tok) {
		case jsont::ObjectStart:
			if (pretty == ePRETTY && !needValue) {
				sint << string(indent, INDENT_CHAR);
			}
			needValue = false;
			sint << '{';
			tok = reader.next();
			if (tok == jsont::ObjectEnd) {
				sint << '}';
				break;
			} else {
				indent++;
				if (pretty == ePRETTY) {
					sint << '\n';
				}
				continue;
			}
		case jsont::ObjectEnd:
			--indent;
			if (pretty == ePRETTY) {
				sint << string(indent, INDENT_CHAR);
			}
			sint << '}';
			break;
		case jsont::ArrayStart:
			if (pretty == ePRETTY && !needValue) {
				sint << string(indent, INDENT_CHAR);
			}
			needValue = false;
			sint << '[';
			tok = reader.next();
			if (tok == jsont::ArrayEnd) {
				sint << ']';
				break;
			} else {
				indent++;
				if (pretty == ePRETTY) {
					sint << '\n';
				}
				continue;
			}
		case jsont::ArrayEnd:
			--indent;
			if (pretty == ePRETTY) {
				sint << string(indent, INDENT_CHAR);
			}
			sint << ']';
			break;
		case jsont::True:
			if (pretty == ePRETTY && !needValue) {
				sint << string(indent, INDENT_CHAR);
			}
			needValue = false;
			sint << "true";
			break;
		case jsont::False:
			if (pretty == ePRETTY && !needValue) {
				sint << string(indent, INDENT_CHAR);
			}
			needValue = false;
			sint << "false";
			break;
		case jsont::Null:
			if (pretty == ePRETTY && !needValue) {
				sint << string(indent, INDENT_CHAR);
			}
			needValue = false;
			sint << "null";
			break;
		case jsont::Integer:
			if (pretty == ePRETTY && !needValue) {
				sint << string(indent, INDENT_CHAR);
			}
			needValue = false;
			sint << reader.dataValue();
			break;
		case jsont::Float:
			if (pretty == ePRETTY && !needValue) {
				sint << string(indent, INDENT_CHAR);
			}
			needValue = false;
			sint << reader.dataValue();
			break;
		case jsont::String:
			if (pretty == ePRETTY && !needValue) {
				sint << string(indent, INDENT_CHAR);
			}
			needValue = false;
			sint << '"' << reader.dataValue() << '"';
			break;
		case jsont::FieldName:
			if (pretty == ePRETTY) {
				sint << string(indent, INDENT_CHAR);
			}
			needValue = true;
			sint << '"' << reader.dataValue() << '"' << ':';
			if (pretty != eNO_WHITESPACE) {
				sint << ' ';
			}
			tok = reader.next();
			continue;
		case jsont::Error:
			std::cerr << reader.errorMessage() << std::endl;
			break;
		default:
			break;
		}
		tok = reader.next();
		if (indent > 0 && tok != jsont::ObjectEnd && tok != jsont::ArrayEnd && tok != jsont::End) {
			sint << ',';
		}
		if (pretty == ePRETTY) {
			sint << '\n';
		}
	}
}

#endif // PRETTY_JSON_H
