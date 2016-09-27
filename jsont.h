// JSON Tokenizer and builder. Copyright (c) 2012, Rasmus Andersson. All rights
// reserved. Use of this source code is governed by a MIT-style license that can
// be found in the LICENSE file.
#ifndef JSONT_CXX_INCLUDED
#define JSONT_CXX_INCLUDED

#include <cstdint>  // uint8_t, int64_t
#include <cstdlib>  // size_t
#include <cstring>  // strlen
#include <cstdbool> // bool
#include <math.h>
#include <assert.h>
#include <string>
#include <stdexcept>
#include <cinttypes>

// After c++17, these should be swapped.
#if 0
#include <experimental/string_view>
#else
#	include <boost/utility/string_ref.hpp>
	namespace std {
		using string_view = boost::string_ref;
	}
#endif

namespace jsont {

	// Tokens
	typedef enum {
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
	} Token;

	class TokenizerInternal;

	// Reads a sequence of bytes and produces tokens and values while doing so
	class Tokenizer {
	public:
		Tokenizer(const char* bytes, size_t length) noexcept;
		~Tokenizer() noexcept;

		// Read next token
		const Token& next() noexcept;

		// Access current token
		const Token& current() const noexcept;

		// Reset the tokenizer, making it possible to reuse this parser so to avoid
		// unnecessary memory allocation and deallocation.
		void reset(const char* bytes, size_t length) noexcept;

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
		typedef enum {
			UnspecifiedError = 0,
			UnexpectedComma,
			UnexpectedTrailingComma,
			InvalidByte,
			PrematureEndOfInput,
			MalformedUnicodeEscapeSequence,
			MalformedNumberLiteral,
			UnterminatedString,
			SyntaxError,
		} ErrorCode;

		// Returns the error code of the last error
		ErrorCode error() const noexcept;

		// Returns a human-readable message for the last error. Never returns NULL.
		const char* errorMessage() const noexcept;

		// The byte offset into input where the tokenizer is currently looking. In the
		// event of an error, this will point to the source of the error.
		size_t inputOffset() const noexcept;

		// Total number of input bytes
		size_t inputSize() const noexcept;

		// A pointer to the input data as passed to `reset` or the constructor.
		const char* inputBytes() const noexcept;

		friend class TokenizerInternal;
	private:
		size_t availableInput() const noexcept;
		size_t endOfInput() const noexcept;
		const Token& setToken(Token t) noexcept;
		const Token& setError(ErrorCode error) noexcept;

		struct {
			const uint8_t* bytes;
			size_t length;
			size_t offset;
		} _input;
		struct Value {
			Value() noexcept : offset(0), length(0) {}
			void beginAtOffset(size_t z) noexcept;
			size_t offset; // into _input.bytes
			size_t length;
			std::string buffer;
		} _value;
		Token _token;
		struct {
			ErrorCode code;
		} _error;
	};

	// ------------------- internal ---------------------

	inline Tokenizer::Tokenizer(const char* bytes, size_t length) noexcept
	: _token(End) {
		reset(bytes, length);
	}

	inline const Token& Tokenizer::current() const noexcept {
		return _token;
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
		return _input.length - _input.offset;
	}

	inline size_t Tokenizer::endOfInput() const noexcept {
		return _input.offset == _input.length;
	}

	inline const Token& Tokenizer::setToken(Token t) noexcept {
		return _token = t;
	}

	inline const Token& Tokenizer::setError(Tokenizer::ErrorCode error) noexcept {
		_error.code = error;
		return _token = Error;
	}

	inline size_t Tokenizer::inputOffset() const noexcept {
		return _input.offset;
	}

	inline size_t Tokenizer::inputSize() const noexcept {
		return _input.length;
	}

	inline const char* Tokenizer::inputBytes() const noexcept {
		return (const char*)_input.bytes;
	}

	inline void Tokenizer::Value::beginAtOffset(size_t z) noexcept {
		offset = z;
		length = 0;
	}

	inline Tokenizer::ErrorCode Tokenizer::error() const noexcept {
		return _error.code;
	}
}

#endif // JSONT_CXX_INCLUDED
