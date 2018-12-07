/*
 *	JSON Tokenizer and builder.
 *
 *	Copyright © 2012  Rasmus Andersson.
 *	Copyright © 2016 Flamewing <flamewing.sonic@gmail.com>
 *
 *	All rights reserved. Use of this source code is governed by a
 *	MIT-style license that can be found in the copying.md file.
 */
#ifndef JSONT_CXX_INCLUDED
#define JSONT_CXX_INCLUDED

#include <string>

// After c++17, these should be swapped.
#if 1
#	include <experimental/string_view>
	namespace std {
		using string_view = std::experimental::string_view;
	}
	using namespace std::experimental::string_view_literals;
#else
#	include <string_view>
	using namespace std::literals::string_view_literals;
#endif

namespace jsont {
	// Tokens
	enum Token {
		End = 0,       // Input ended
		ObjectStart,   // {
		ObjectEnd,     // }
		ArrayStart,    // [
		ArrayEnd,      // ]
		True,          // true
		False,         // false
		Null,          // null
		Integer,       // number value without a fraction part
		Float,         // number value with a fraction part
		String,        // string value
		FieldName,     // field name
		Error,         // An error occured (see `error()` for details)
		_Comma,
	};

	// Reads a sequence of bytes and produces tokens and values while doing so
	class Tokenizer {
	public:
		Tokenizer(const char* bytes, size_t length) noexcept;
		Tokenizer(std::string_view slice) noexcept;
		~Tokenizer() noexcept;

		// Read next token
		const Token& next() noexcept;

		// Access current token
		const Token& current() const noexcept;

		// Reset the tokenizer, making it possible to reuse this parser so to avoid
		// unnecessary memory allocation and deallocation.
		void reset(const char* bytes, size_t length) noexcept;
		void reset(std::string_view slice) noexcept;

		// True if the current token has a value
		bool hasValue() const noexcept;

		// Returns a slice of the input which represents the current value, or nothing
		// (empty etring_view) if the current token has no value (e.g. start of an object).
		std::string_view dataValue() const noexcept;

		// Returns a *copy* of the current string value.
		std::string stringValue() const noexcept;

		// Returns the current value as a double-precision floating-point number.
		double floatValue() const noexcept;

		// Returns the current value as a signed 64-bit integer.
		int64_t intValue() const noexcept;

		// Returns the current value as a boolean
		bool boolValue() const noexcept;

		// Error codes
		enum ErrorCode {
			UnspecifiedError = 0,
			UnexpectedComma,
			UnexpectedTrailingComma,
			InvalidByte,
			PrematureEndOfInput,
			MalformedUnicodeEscapeSequence,
			MalformedNumberLiteral,
			UnterminatedString,
			SyntaxError,
		};

		// Returns the error code of the last error
		ErrorCode error() const noexcept;

		// Returns a human-readable message for the last error. Never returns NULL.
		std::string_view errorMessage() const noexcept;

		// The byte offset into input where the tokenizer is currently looking. In the
		// event of an error, this will point to the source of the error.
		size_t inputOffset() const noexcept;

		// Total number of input bytes
		size_t inputSize() const noexcept;

	private:
		const Token& readAtom(std::string_view atom, const Token& token) noexcept;
		size_t availableInput() const noexcept;
		size_t endOfInput() const noexcept;
		const Token& setToken(Token t) noexcept;
		const Token& setError(ErrorCode error) noexcept;

		std::string_view _input;
		std::string_view _value;
		size_t _offset;
		Token _token;
		ErrorCode _error;
	};

	// ------------------- internal ---------------------

	inline Tokenizer::Tokenizer(const char* bytes, size_t length) noexcept
	: _token(End) {
		reset(bytes, length);
	}

	inline Tokenizer::Tokenizer(std::string_view slice) noexcept
	: _token(End) {
		reset(slice);
	}

	inline const Token& Tokenizer::current() const noexcept {
		return _token;
	}

	inline void Tokenizer::reset(const char* bytes, size_t length) noexcept {
		reset(std::string_view(bytes, length));
	}

	inline void Tokenizer::reset(std::string_view slice) noexcept {
		_input = slice;
		_offset = 0;
		_error = UnspecifiedError;
		// Advance to first token
		next();
	}

	inline bool Tokenizer::hasValue() const noexcept {
		return _token >= Integer && _token <= FieldName;
	}

	inline std::string Tokenizer::stringValue() const noexcept {
		return dataValue().to_string();
	}

	inline bool Tokenizer::boolValue() const noexcept {
		return _token == True;
	}

	inline size_t Tokenizer::availableInput() const noexcept {
		return _input.length() - _offset;
	}

	inline size_t Tokenizer::endOfInput() const noexcept {
		return _offset == _input.length();
	}

	inline const Token& Tokenizer::setToken(Token t) noexcept {
		return _token = t;
	}

	inline const Token& Tokenizer::setError(Tokenizer::ErrorCode error) noexcept {
		_error = error;
		return _token = Error;
	}

	inline Tokenizer::ErrorCode Tokenizer::error() const noexcept {
		return _error;
	}
}

#endif // JSONT_CXX_INCLUDED
