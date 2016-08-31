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
void printJSON(Src const &data, Dst &sint, PrettyJSON pretty) {
	jsont::Tokenizer reader(data.data(), data.size(), jsont::UTF8TextEncoding);
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
			sint << reader.intValue();
			break;
		case jsont::Float:
			if (pretty == ePRETTY && !needValue) {
				sint << string(indent, INDENT_CHAR);
			}
			needValue = false;
			sint << reader.floatValue();
			break;
		case jsont::String:
			if (pretty == ePRETTY && !needValue) {
				sint << string(indent, INDENT_CHAR);
			}
			needValue = false;
			sint << '"' << reader.stringValue() << '"';
			break;
		case jsont::FieldName:
			if (pretty == ePRETTY) {
				sint << string(indent, INDENT_CHAR);
			}
			needValue = true;
			sint << '"' << reader.stringValue() << '"' << ':';
			if (pretty != eNO_WHITESPACE) {
				sint << ' ';
			}
			tok = reader.next();
			continue;
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
