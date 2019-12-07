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
#include <string_view>
#include <unordered_map>

namespace jsont {
    // Tokens
    enum Token : uint32_t {
        End = 0,     // Input ended
        ObjectStart, // {
        ObjectEnd,   // }
        ArrayStart,  // [
        ArrayEnd,    // ]
        True,        // true
        False,       // false
        Null,        // null
        Integer,     // number value without a fraction part
        Float,       // number value with a fraction part
        String,      // string value
        FieldName,   // field name
        Error,       // An error occured (see `error()` for details)
        Comma
    };

    // Reads a sequence of bytes and produces tokens and values while doing so
    class Tokenizer {
    public:
        Tokenizer(const char* bytes, size_t length) noexcept;
        explicit Tokenizer(std::string_view slice) noexcept;

        // Read next token
        auto next() noexcept -> Token;

        // Access current token
        auto current() const noexcept -> Token;

        // Reset the tokenizer, making it possible to reuse this parser so to
        // avoid unnecessary memory allocation and deallocation.
        void reset(const char* bytes, size_t length) noexcept;
        void reset(std::string_view slice) noexcept;

        // True if the current token has a value
        auto hasValue() const noexcept -> bool;

        // Returns a slice of the input which represents the current value, or
        // nothing (empty etring_view) if the current token has no value (e.g.
        // start of an object).
        auto dataValue() const noexcept -> std::string_view;

        // Returns a *copy* of the current string value.
        auto stringValue() const noexcept -> std::string;

        // Returns the current value as a double-precision floating-point
        // number.
        auto floatValue() const noexcept -> double;

        // Returns the current value as a signed 64-bit integer.
        auto intValue() const noexcept -> int64_t;

        // Returns the current value as a boolean
        auto boolValue() const noexcept -> bool;

        // Error codes
        enum ErrorCode : uint32_t {
            UnspecifiedError = 0,
            UnexpectedComma,
            UnexpectedTrailingComma,
            InvalidByte,
            PrematureEndOfInput,
            MalformedUnicodeEscapeSequence,
            MalformedNumberLiteral,
            UnterminatedString,
            SyntaxError
        };

        // Returns the error code of the last error
        auto error() const noexcept -> ErrorCode;

        // Returns a human-readable message for the last error. Never returns
        // NULL.
        auto errorMessage() const noexcept -> std::string_view;

        // The byte offset into input where the tokenizer is currently looking.
        // In the event of an error, this will point to the source of the error.
        auto inputOffset() const noexcept -> size_t;

        // Total number of input bytes
        auto inputSize() const noexcept -> size_t;

    private:
        auto translateToken(Token tok) const noexcept -> std::string_view;

        void skipWS() noexcept;
        auto readDigits(size_t digits) noexcept -> bool;
        auto readFraction() noexcept -> bool;
        auto readExponent() noexcept -> bool;
        auto readNumber(char b, size_t token_start) noexcept -> Token;
        auto readString(char b, size_t token_start) noexcept -> Token;
        auto readComma() noexcept -> Token;
        auto readEndBracket(Token token) noexcept -> Token;
        auto readAtom(std::string_view atom, Token token) noexcept -> Token;
        auto availableInput() const noexcept -> size_t;
        auto endOfInput() const noexcept -> bool;
        auto setToken(Token t) noexcept -> Token;
        auto setError(ErrorCode error) noexcept -> Token;
        void initConverter() noexcept;

        std::unordered_map<Token, std::string> _convert;
        std::string_view                       _input;
        std::string_view                       _value;
        size_t                                 _offset;
        Token                                  _token;
        ErrorCode                              _error;
    };

    // ------------------- internal ---------------------

    inline Tokenizer::Tokenizer(const char* bytes, size_t length) noexcept
        : _offset(0), _token(End), _error(UnspecifiedError) {
        initConverter();
        reset(bytes, length);
    }

    inline Tokenizer::Tokenizer(std::string_view slice) noexcept
        : _offset(0), _token(End), _error(UnspecifiedError) {
        initConverter();
        reset(slice);
    }

    inline auto Tokenizer::current() const noexcept -> Token { return _token; }

    inline void Tokenizer::reset(const char* bytes, size_t length) noexcept {
        reset(std::string_view(bytes, length));
    }

    inline void Tokenizer::reset(std::string_view slice) noexcept {
        _input  = slice;
        _offset = 0;
        _error  = UnspecifiedError;
        // Advance to first token
        next();
    }

    inline auto Tokenizer::hasValue() const noexcept -> bool {
        return _token >= Integer && _token <= FieldName;
    }

    inline auto Tokenizer::stringValue() const noexcept -> std::string {
        return std::string(dataValue());
    }

    inline auto Tokenizer::boolValue() const noexcept -> bool {
        return _token == True;
    }

    inline auto Tokenizer::translateToken(Token tok) const noexcept
        -> std::string_view {
        return _convert.find(tok)->second;
    }

    inline auto Tokenizer::readComma() noexcept -> Token {
        if (_token == ObjectStart || _token == ArrayStart || _token == Comma) {
            return setError(UnexpectedComma);
        }
        return setToken(Comma);
    }

    inline auto Tokenizer::readEndBracket(Token token) noexcept -> Token {
        if (_token == Comma) {
            return setError(UnexpectedTrailingComma);
        }
        return setToken(token);
    }

    inline auto Tokenizer::availableInput() const noexcept -> size_t {
        return _input.length() - _offset;
    }

    inline auto Tokenizer::endOfInput() const noexcept -> bool {
        return _offset == _input.length();
    }

    inline auto Tokenizer::setToken(Token t) noexcept -> Token {
        return _token = t;
    }

    inline auto Tokenizer::setError(Tokenizer::ErrorCode error) noexcept
        -> Token {
        _error        = error;
        return _token = Error;
    }

    inline void Tokenizer::initConverter() noexcept {
        using namespace std::literals::string_literals;
        _convert = {{End, "<<EOF>>"s},       {ObjectStart, "{"s},
                    {ObjectEnd, "}"s},       {ArrayStart, "["s},
                    {ArrayEnd, "]"s},        {True, "true"s},
                    {False, "false"s},       {Null, "null"s},
                    {Integer, "<<int>>"s},   {Float, "<<float>>"s},
                    {String, "<<string>>"s}, {FieldName, "<<field name>>"s},
                    {Error, "<<error>>"s},   {Comma, ","s}};
    }

    inline auto Tokenizer::error() const noexcept -> Tokenizer::ErrorCode {
        return _error;
    }
} // namespace jsont

#endif // JSONT_CXX_INCLUDED
