#include "jsont.h"

// After c++17, this should be removed.
#if 1
	inline constexpr std::string_view
	operator""sv(const char* __str, size_t __len) {
		return std::string_view{__str, __len};
	}
#endif

using std::string_view;

namespace jsont {

	class TokenizerInternal {
	public:
		inline static const uint8_t* currentInput(const Tokenizer& self) noexcept {
			return self._input.bytes + self._input.offset;
		}

		inline static const Token& readAtom(Tokenizer& self, const char* str, size_t len, const Token& token) noexcept {
			if (self.availableInput() < len) {
				return self.setError(Tokenizer::PrematureEndOfInput);
			} else if (memcmp(currentInput(self), str, len) != 0) {
				return self.setError(Tokenizer::InvalidByte);
			} else {
				self._input.offset += len;
				return self.setToken(token);
			}
		}
	};

	Tokenizer::~Tokenizer() {
	}

	void Tokenizer::reset(const char* bytes, size_t length) noexcept {
		_input.bytes = reinterpret_cast<const uint8_t*>(bytes);
		_input.length = length;
		_input.offset = 0;
		_error.code = UnspecifiedError;
		// Advance to first token
		next();
	}

	const char* Tokenizer::errorMessage() const noexcept {
		switch (_error.code) {
			case UnexpectedComma:
				return "Unexpected comma";
			case UnexpectedTrailingComma:
				return "Unexpected trailing comma";
			case InvalidByte:
				return "Invalid input byte";
			case PrematureEndOfInput:
				return "Premature end of input";
			case MalformedUnicodeEscapeSequence:
				return "Malformed Unicode escape sequence";
			case MalformedNumberLiteral:
				return "Malformed number literal";
			case UnterminatedString:
				return "Unterminated string";
			case SyntaxError:
				return "Illegal JSON (syntax error)";
			default:
				return "Unspecified error";
		}
	}

	string_view Tokenizer::dataValue() const noexcept {
		if (!hasValue()) {
			return string_view();
		} else {
			return string_view(reinterpret_cast<const char *>(_input.bytes) + _value.offset,
			                   _value.length);
		}
	}

	double Tokenizer::floatValue() const noexcept {
		if (!hasValue()) {
			return _token == jsont::True ? 1.0 : 0.0;
		}

		const char* bytes = reinterpret_cast<const char*>(_input.bytes) + _value.offset;
		if (availableInput() == 0) {
			// In this case where the data lies at the edge of the buffer, we can't pass
			// it directly to atof, since there will be no sentinel byte. We are fine
			// with a copy, since this is an edge case (only happens either for broken
			// JSON or when the whole document is just a number).
			char* buf[128];
			if (_value.length > 127) {
				// We are unable to interpret such a large literal in this edge-case
				return nan("");
			}
			memcpy(buf, bytes, _value.length);
			buf[_value.length] = '\0';
			return strtod(reinterpret_cast<const char*>(buf), nullptr);
		}
		return strtod(bytes, nullptr);
	}

	int64_t Tokenizer::intValue() const noexcept {
		if (!hasValue()) {
			return _token == jsont::True ? 1LL : 0LL;
		}

		const char* bytes = reinterpret_cast<const char*>(_input.bytes) + _value.offset;
		if (availableInput() == 0) {
			// In this case where the data lies at the edge of the buffer, we can't pass
			// it directly to atof, since there will be no sentinel byte. We are fine
			// with a copy, since this is an edge case (only happens either for broken
			// JSON or when the whole document is just a number).
			char* buf[21];
			if (_value.length > 20) {
				// We are unable to interpret such a large literal in this edge-case
				return 0;
			}
			memcpy(buf, bytes, _value.length);
			buf[_value.length] = '\0';
			return strtoll(reinterpret_cast<const char*>(buf), nullptr, 10);
		}
		return strtoll(bytes, nullptr, 10);
	}

	const Token& Tokenizer::next() noexcept {
		//
		// { } [ ] n t f "
		//         | | | |
		//         | | | +- /[^"]*/ "
		//         | | +- a l s e
		//         | +- r u e
		//         +- u l l
		//
		while (!endOfInput()) {
			uint8_t b = _input.bytes[_input.offset++];
			switch (b) {
				case '{':
					return setToken(ObjectStart);
				case '}': {
					if (_token == _Comma) {
						return setError(UnexpectedTrailingComma);
					}
					return setToken(ObjectEnd);
				}

				case '[':
					return setToken(ArrayStart);
				case ']': {
					if (_token == _Comma) {
						return setError(UnexpectedTrailingComma);
					}
					return setToken(ArrayEnd);
				}

				case 'n':
					return TokenizerInternal::readAtom(*this, "ull", 3, jsont::Null);
				case 't':
					return TokenizerInternal::readAtom(*this, "rue", 3, jsont::True);
				case 'f':
					return TokenizerInternal::readAtom(*this, "alse", 4, jsont::False);

				case ' ':
				case '\t':
				case '\r':
				case '\n':
					// IETF RFC4627
					// ignore whitespace and let the outer "while" do its thing
					break;

				case 0:
					return setError(InvalidByte);

				// when we read a value, we don't produce a token until we either reach
				// end of input, a colon (then the value is a field name), a comma, or an
				// array or object terminator.

				case '"': {
					_value.beginAtOffset(_input.offset);

					while (!endOfInput()) {
						b = _input.bytes[_input.offset++];
						assert(_input.offset < _input.length);

						if (b == '\\') {
							if (endOfInput()) {
								return setError(PrematureEndOfInput);
							}
							_input.offset++;
							_value.length = _input.offset - _value.offset - 1;
						} else if (b == '"') {
							break;
						} else if (b == 0) {
							return setError(InvalidByte);
						}
					} // while (!endOfInput())

					if (b != '"') {
						return setError(UnterminatedString);
					}
					_value.length = _input.offset - _value.offset - 1;

					// is this a field name?
					while (!endOfInput()) {
						b = _input.bytes[_input.offset++];
						switch (b) {
							case ' ':
							case '\t':
							case '\r':
							case '\n':
								break;
							case ':':
								return setToken(FieldName);
							case ',':
								return setToken(jsont::String);
							case ']':
							case '}': {
								--_input.offset; // rewind
								return setToken(jsont::String);
							}
							case 0:
								return setError(InvalidByte);
							default: {
								// Expected a comma or a colon
								return setError(SyntaxError);
							}
						}
					}
					return setToken(jsont::String);
				}

				case ',': {
					if (_token == ObjectStart || _token == ArrayStart || _token == _Comma) {
						return setError(UnexpectedComma);
					}
					_token = _Comma;
					break;
				}

				default: {
					if (isdigit(int(b)) || b == '+' || b == '-') {
						// We are reading a number
						_value.beginAtOffset(_input.offset-1);
						Token token = jsont::Integer;

						while (!endOfInput()) {
							b = _input.bytes[_input.offset++];
							switch (b) {
								case '0':
								case '1':
								case '2':
								case '3':
								case '4':
								case '5':
								case '6':
								case '7':
								case '8':
								case '9':
									break;
								case '.':
									token = jsont::Float;
									break;
								case 'E':
								case 'e':
								case '-':
								case '+': {
									if (token != jsont::Float) {
										return setError(MalformedNumberLiteral);
									}
									break;
								}
								default: {
									if ((_input.offset - _value.offset == 1) &&
									        (_input.bytes[_value.offset] == '-' ||
									         _input.bytes[_value.offset] == '+')) {
										return setError(MalformedNumberLiteral);
									}

									// rewind the byte that terminated this number literal
									--_input.offset;

									_value.length = _input.offset - _value.offset;
									return setToken(token);
								}
							}
						}
						return setToken(End);
					} else {
						return setError(InvalidByte);
					}
				}
			}
		}

		return setToken(End);
	}

} // namespace jsont
