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

#ifndef EXPRESSION_HH
#define EXPRESSION_HH

#include <ostream>
#include <string>

#include "polymorphic_value.hh"
#include "util.hh"

class Expression {
public:
    Expression() noexcept             = default;
    virtual ~Expression() noexcept    = default;
    Expression(Expression const&)     = default;
    Expression(Expression&&) noexcept = default;

    auto operator=(Expression const&) -> Expression& = default;
    auto operator=(Expression&&) noexcept -> Expression& = default;

    auto write(std::ostream& out, bool needParens) const noexcept
        -> std::ostream& {
        if (needParens && !is_simple()) {
            out << '(';
            return write_impl(out) << ')';
        }
        return write_impl(out);
    }
    [[nodiscard]] virtual auto is_simple() const noexcept -> bool {
        return true;
    }

private:
    virtual auto write_impl(std::ostream& out) const noexcept -> std::ostream& {
        return out;
    }
};

// A generic statement containing content
class ContentExpression : public Expression {
public:
    ContentExpression() = default;
    explicit ContentExpression(std::string text) : content(std::move(text)) {}

protected:
    auto write_impl(std::ostream& out) const noexcept
        -> std::ostream& override {
        return out << '"' << content << '"';
    }

private:
    std::string content;
};

class DivertExpression : public Expression {
public:
    explicit DivertExpression(std::string trg) : target(std::move(trg)) {}

private:
    auto write_impl(std::ostream& out) const noexcept
        -> std::ostream& override {
        return out << "  -> " << target;
    }
    std::string target;
};

class VariableRValueExpression : public Expression {
public:
    explicit VariableRValueExpression(std::string name)
        : varName(std::move(name)) {}

private:
    auto write_impl(std::ostream& out) const noexcept
        -> std::ostream& override {
        return out << varName;
    }
    std::string varName;
};

class VariableLValueExpression : public Expression {
public:
    explicit VariableLValueExpression(std::string name)
        : varName(std::move(name)) {}

private:
    auto write_impl(std::ostream& out) const noexcept
        -> std::ostream& override {
        return out << varName;
    }
    std::string varName;
};

enum class UnaryOps : uint8_t {
    Log10,
    Not,
    FlagIsSet,
    FlagIsNotSet,
    HasNotRead,
    HasRead
};

class UnaryOpExpression : public Expression {
public:
    UnaryOpExpression(UnaryOps kind, nonstd::polymorphic_value<Expression> ex)
        : oper(kind), expr(std::move(ex)) {}

private:
    auto write_impl(std::ostream& out) const noexcept
        -> std::ostream& override {
        switch (oper) {
        case UnaryOps::Log10:
            out << "Log10(";
            expr->write(out, false);
            return out << ')';
        case UnaryOps::Not:
        case UnaryOps::FlagIsNotSet:
        case UnaryOps::HasNotRead:
            out << "!";
            [[fallthrough]];
        case UnaryOps::FlagIsSet:
        case UnaryOps::HasRead:
            expr->write(out, true);
            return out;
        }
        return out;
    }
    UnaryOps                              oper;
    nonstd::polymorphic_value<Expression> expr;
};

enum class PostfixOps : uint8_t { Increment, Decrement };

class PostfixOpExpression : public Expression {
public:
    PostfixOpExpression(PostfixOps kind, VariableLValueExpression var)
        : oper(kind), variable(std::move(var)) {}

private:
    auto write_impl(std::ostream& out) const noexcept
        -> std::ostream& override {
        if (oper == PostfixOps::Increment) {
            return variable.write(out, false) << "++\n";
        }
        return variable.write(out, false) << "--\n";
    }
    PostfixOps               oper;
    VariableLValueExpression variable;
};

enum class BinaryOps : uint8_t {
    Add,
    Subtract,
    Divide,
    Mod,
    Multiply,
    And,
    Or,
    Equals,
    NotEquals,
    GreaterThan,
    GreaterThanOrEqualTo,
    LessThan,
    LessThanOrEqualTo
};

class BinaryOpExpression : public Expression {
public:
    BinaryOpExpression(
        BinaryOps kind, nonstd::polymorphic_value<Expression> ll,
        nonstd::polymorphic_value<Expression> rr)
        : oper(kind), lhs(std::move(ll)), rhs(std::move(rr)) {}
    [[nodiscard]] auto is_simple() const noexcept -> bool override {
        return false;
    }

private:
    auto write_impl(std::ostream& out) const noexcept
        -> std::ostream& override {
        lhs->write(out, true);
        switch (oper) {
        case BinaryOps::Add:
            out << " + ";
            break;
        case BinaryOps::Subtract:
            out << " - ";
            break;
        case BinaryOps::Divide:
            out << " / ";
            break;
        case BinaryOps::Mod:
            out << " % ";
            break;
        case BinaryOps::Multiply:
            out << " * ";
            break;
        case BinaryOps::And:
            out << " && ";
            break;
        case BinaryOps::Or:
            out << " || ";
            break;
        case BinaryOps::Equals:
            out << " == ";
            break;
        case BinaryOps::NotEquals:
            out << " != ";
            break;
        case BinaryOps::GreaterThan:
            out << " > ";
            break;
        case BinaryOps::GreaterThanOrEqualTo:
            out << " >= ";
            break;
        case BinaryOps::LessThan:
            out << " < ";
            break;
        case BinaryOps::LessThanOrEqualTo:
            out << " <= ";
            break;
        }
        rhs->write(out, true);
        return out;
    }
    BinaryOps                             oper;
    nonstd::polymorphic_value<Expression> lhs;
    nonstd::polymorphic_value<Expression> rhs;
};

#endif
