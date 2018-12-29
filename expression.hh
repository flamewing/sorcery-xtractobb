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

#include <memory>
#include <ostream>
#include <string>

namespace gsl {
    template <class T, class = std::enable_if_t<std::is_pointer<T>::value>>
    using owner = T;
}

template <typename TO, typename FROM>
std::unique_ptr<TO> static_unique_pointer_cast(std::unique_ptr<FROM>&& old) {
    return std::unique_ptr<TO>{static_cast<TO*>(old.release())};
}

class Expression {
public:
    Expression() noexcept             = default;
    virtual ~Expression() noexcept    = default;
    Expression(Expression const&)     = default;
    Expression(Expression&&) noexcept = default;
    Expression& operator=(Expression const&) = default;
    Expression& operator=(Expression&&) noexcept = default;

    std::ostream& write(std::ostream& out) const noexcept {
        write_impl(out);
        return out;
    }
    std::unique_ptr<Expression> clone() const {
        return std::unique_ptr<Expression>(clone_impl());
    }

private:
    virtual gsl::owner<Expression*> clone_impl() const = 0;
    virtual bool                    is_simple() const noexcept { return true; }
    virtual std::ostream& write_impl(std::ostream& out) const noexcept = 0;
};

class VariableRValueExpression : public Expression {
public:
    explicit VariableRValueExpression(std::string name)
        : varName(std::move(name)) {}

private:
    gsl::owner<VariableRValueExpression*> clone_impl() const override {
        return new VariableRValueExpression(*this);
    }
    std::ostream& write_impl(std::ostream& out) const noexcept override {
        return out << varName;
    }
    std::string varName;
};

class VariableLValueExpression : public Expression {
public:
    explicit VariableLValueExpression(std::string name)
        : varName(std::move(name)) {}

private:
    gsl::owner<VariableLValueExpression*> clone_impl() const override {
        return new VariableLValueExpression(*this);
    }
    std::ostream& write_impl(std::ostream& out) const noexcept override {
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
    UnaryOpExpression(UnaryOps kind, VariableRValueExpression var)
        : oper(kind), variable(std::move(var)) {}

private:
    gsl::owner<UnaryOpExpression*> clone_impl() const override {
        return new UnaryOpExpression(*this);
    }
    std::ostream& write_impl(std::ostream& out) const noexcept override {
        switch (oper) {
        case UnaryOps::Log10:
            out << "Log10(";
            variable.write(out);
            return out << ')';
        case UnaryOps::Not:
        case UnaryOps::FlagIsNotSet:
        case UnaryOps::HasNotRead:
            out << "!";
            [[fallthrough]];
        case UnaryOps::FlagIsSet:
        case UnaryOps::HasRead:
            return variable.write(out);
        }
        return out;
    }
    UnaryOps                 oper;
    VariableRValueExpression variable;
};

enum class PostfixOps : uint8_t { Increment, Decrement };

class PostfixOpExpression : public Expression {
public:
    PostfixOpExpression(PostfixOps kind, VariableLValueExpression var)
        : oper(kind), variable(std::move(var)) {}

private:
    gsl::owner<PostfixOpExpression*> clone_impl() const override {
        return new PostfixOpExpression(*this);
    }
    std::ostream& write_impl(std::ostream& out) const noexcept override {
        if (oper == PostfixOps::Increment) {
            return variable.write(out) << "++\n";
        }
        return variable.write(out) << "--\n";
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
        BinaryOps kind, std::unique_ptr<Expression> ll,
        std::unique_ptr<Expression> rr)
        : oper(kind), lhs(std::move(ll)), rhs(std::move(rr)) {}
    ~BinaryOpExpression() noexcept override = default;
    BinaryOpExpression(BinaryOpExpression const& other)
        : Expression(other), oper(other.oper), lhs(other.lhs->clone()),
          rhs(other.rhs->clone()) {}
    BinaryOpExpression(BinaryOpExpression&&) noexcept = default;
    BinaryOpExpression& operator=(BinaryOpExpression const& other) {
        if (this != &other) {
            Expression::operator=(other);
            oper                = other.oper;
            lhs                 = other.lhs->clone();
            rhs                 = other.rhs->clone();
        }
        return *this;
    }
    BinaryOpExpression& operator=(BinaryOpExpression&&) noexcept = default;

private:
    gsl::owner<BinaryOpExpression*> clone_impl() const override {
        return new BinaryOpExpression(*this);
    }
    std::ostream& write_impl(std::ostream& out) const noexcept override {
        lhs->write(out);
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
        return rhs->write(out);
    }
    BinaryOps                   oper;
    std::unique_ptr<Expression> lhs;
    std::unique_ptr<Expression> rhs;
};

inline std::ostream&
operator<<(std::ostream& out, Expression const& expr) noexcept {
    return expr.write(out);
}

#endif
