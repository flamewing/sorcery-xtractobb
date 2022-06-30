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

#pragma once

#include "polymorphic_value.hh"
#include "util.hh"

#include <ostream>
#include <string>

class Expression {
public:
    Expression() noexcept             = default;
    virtual ~Expression() noexcept    = default;
    Expression(Expression const&)     = default;
    Expression(Expression&&) noexcept = default;

    auto operator=(Expression const&) -> Expression&     = default;
    auto operator=(Expression&&) noexcept -> Expression& = default;

    auto write(std::ostream& output, bool needParens) const noexcept
            -> std::ostream& {
        if (needParens && !is_simple()) {
            output << '(';
            return write_impl(output) << ')';
        }
        return write_impl(output);
    }
    [[nodiscard]] virtual auto is_simple() const noexcept -> bool {
        return true;
    }

private:
    virtual auto write_impl(std::ostream& output) const noexcept -> std::ostream& {
        return output;
    }
};

// A generic statement containing content
class ContentExpression : public Expression {
public:
    ContentExpression() = default;
    explicit ContentExpression(std::string text) : content(std::move(text)) {}

protected:
    auto write_impl(std::ostream& output) const noexcept
            -> std::ostream& override {
        return output << '"' << content << '"';
    }

private:
    std::string content;
};

class DivertExpression : public Expression {
public:
    explicit DivertExpression(std::string trg) : target(std::move(trg)) {}

private:
    auto write_impl(std::ostream& output) const noexcept
            -> std::ostream& override {
        return output << "  -> " << target;
    }
    std::string target;
};

class VariableRValueExpression : public Expression {
public:
    explicit VariableRValueExpression(std::string name)
            : varName(std::move(name)) {}

private:
    auto write_impl(std::ostream& output) const noexcept
            -> std::ostream& override {
        return output << varName;
    }
    std::string varName;
};

class VariableLValueExpression : public Expression {
public:
    explicit VariableLValueExpression(std::string name)
            : varName(std::move(name)) {}

private:
    auto write_impl(std::ostream& output) const noexcept
            -> std::ostream& override {
        return output << varName;
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
    UnaryOpExpression(UnaryOps kind, nonstd::polymorphic_value<Expression> expr_)
            : oper(kind), expr(std::move(expr_)) {}

private:
    auto write_impl(std::ostream& output) const noexcept
            -> std::ostream& override {
        switch (oper) {
        case UnaryOps::Log10:
            output << "Log10(";
            expr->write(output, false);
            return output << ')';
        case UnaryOps::Not:
        case UnaryOps::FlagIsNotSet:
        case UnaryOps::HasNotRead:
            output << "!";
            [[fallthrough]];
        case UnaryOps::FlagIsSet:
        case UnaryOps::HasRead:
            expr->write(output, true);
            return output;
        }
        return output;
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
    auto write_impl(std::ostream& output) const noexcept
            -> std::ostream& override {
        if (oper == PostfixOps::Increment) {
            return variable.write(output, false) << "++\n";
        }
        return variable.write(output, false) << "--\n";
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
            BinaryOps kind, nonstd::polymorphic_value<Expression> lhs_,
            nonstd::polymorphic_value<Expression> rhs_)
            : oper(kind), lhs(std::move(lhs_)), rhs(std::move(rhs_)) {}
    [[nodiscard]] auto is_simple() const noexcept -> bool override {
        return false;
    }

private:
    auto write_impl(std::ostream& output) const noexcept
            -> std::ostream& override {
        lhs->write(output, true);
        switch (oper) {
        case BinaryOps::Add:
            output << " + ";
            break;
        case BinaryOps::Subtract:
            output << " - ";
            break;
        case BinaryOps::Divide:
            output << " / ";
            break;
        case BinaryOps::Mod:
            output << " % ";
            break;
        case BinaryOps::Multiply:
            output << " * ";
            break;
        case BinaryOps::And:
            output << " && ";
            break;
        case BinaryOps::Or:
            output << " || ";
            break;
        case BinaryOps::Equals:
            output << " == ";
            break;
        case BinaryOps::NotEquals:
            output << " != ";
            break;
        case BinaryOps::GreaterThan:
            output << " > ";
            break;
        case BinaryOps::GreaterThanOrEqualTo:
            output << " >= ";
            break;
        case BinaryOps::LessThan:
            output << " < ";
            break;
        case BinaryOps::LessThanOrEqualTo:
            output << " <= ";
            break;
        }
        rhs->write(output, true);
        return output;
    }
    BinaryOps                             oper;
    nonstd::polymorphic_value<Expression> lhs;
    nonstd::polymorphic_value<Expression> rhs;
};
