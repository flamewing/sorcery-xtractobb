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

#ifndef STATEMENT_HH
#define STATEMENT_HH

#include <map>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "expression.hh"
#include "polymorphic_value.hh"
#include "util.hh"

class driver;

// Generic statement
class Statement {
public:
    Statement() noexcept            = default;
    virtual ~Statement() noexcept   = default;
    Statement(Statement const&)     = default;
    Statement(Statement&&) noexcept = default;

    Statement& operator=(Statement const&) = default;
    Statement& operator=(Statement&&) noexcept = default;

    std::ostream& write(std::ostream& out, size_t indent) const noexcept {
        return write_impl(out, indent);
    }
    [[nodiscard]] std::string getIndent(size_t indent) const {
        return std::string(indent, ' ');
    }

private:
    [[nodiscard]] virtual bool is_simple() const noexcept { return true; }

protected:
    virtual std::ostream& write_impl(std::ostream& out, size_t indent) const
        noexcept {
        ignore_unused_variable_warning(indent);
        return out;
    }
};

// A generic statement containing content
class ContentStatement : public Statement {
public:
    ContentStatement() = default;
    explicit ContentStatement(std::string text) : content(std::move(text)) {}

protected:
    std::ostream& write_impl(std::ostream& out, size_t indent) const
        noexcept override {
        return out << getIndent(indent) << content << '\n';
    }

private:
    std::string content;
};

// A generic statement containing a choice
class ChoiceStatement : public Statement {
public:
    ChoiceStatement() = default;
    explicit ChoiceStatement(
        std::string text, nonstd::polymorphic_value<Expression> cond = {})
        : content(std::move(text)), condition(std::move(cond)) {}

protected:
    std::ostream& write_impl(std::ostream& out, size_t indent) const
        noexcept override {
        out << getIndent(indent) << "* ";
        if (condition) {
            out << "{ ";
            condition->write(out, false) << " }  ";
        }
        return out << content << '\n';
    }

private:
    std::string                           content;
    nonstd::polymorphic_value<Expression> condition;
};

// A variable assignment statement
class AssignmentStatement : public Statement {
public:
    explicit AssignmentStatement(
        std::string name, nonstd::polymorphic_value<Expression> expr, bool decl)
        : varName(std::move(name)), expression(std::move(expr)), declare(decl) {
    }

protected:
    std::ostream& write_impl(std::ostream& out, size_t indent) const
        noexcept override {
        out << getIndent(indent) << "~ ";
        if (declare) {
            out << "temp ";
        }
        out << varName;
        return expression->write(out, false) << '\n';
    }

private:
    std::string                           varName;
    nonstd::polymorphic_value<Expression> expression;
    bool                                  declare;
};

// A generic statement containing an expression
class ExpressionStatement : public Statement {
public:
    ExpressionStatement() = default;
    explicit ExpressionStatement(nonstd::polymorphic_value<Expression> expr)
        : expression(std::move(expr)) {}

protected:
    std::ostream& write_impl(std::ostream& out, size_t indent) const
        noexcept override {
        out << getIndent(indent) << "~ ";
        return expression->write(out, false) << '\n';
    }

private:
    nonstd::polymorphic_value<Expression> expression;
};

// A generic statement containing an expression
class ReturnStatement : public Statement {
public:
    ReturnStatement() = default;
    explicit ReturnStatement(nonstd::polymorphic_value<Expression> expr)
        : expression(std::move(expr)) {}

protected:
    std::ostream& write_impl(std::ostream& out, size_t indent) const
        noexcept override {
        out << getIndent(indent) << "~ return ";
        return expression->write(out, false) << '\n';
    }

private:
    nonstd::polymorphic_value<Expression> expression;
};

// A generic statement block statement
class BlockStatement : public Statement {
public:
    using StatementList = std::vector<nonstd::polymorphic_value<Statement>>;
    BlockStatement()    = default;

    void add_statement(nonstd::polymorphic_value<Statement> stmt) {
        statements.emplace_back(std::move(stmt));
    }
    void steal_statements(StatementList& other) noexcept {
        statements.swap(other);
    }
    StatementList& get_statements() noexcept { return statements; }

protected:
    std::ostream& write_impl(std::ostream& out, size_t indent) const
        noexcept override {
        for (auto const& elem : statements) {
            elem->write(out, indent);
        }
        return out;
    }

private:
    StatementList statements;
};

// An if statement
class ElseStatement : public BlockStatement {
public:
    ElseStatement() = default;

protected:
    std::ostream& write_impl(std::ostream& out, size_t indent) const
        noexcept override {
        out << getIndent(indent) << "- else:\n";
        return BlockStatement::write_impl(out, indent + 4);
    }
};

// An if statement
class IfStatement : public Statement {
public:
    IfStatement() = default;
    IfStatement(
        nonstd::polymorphic_value<Expression> cond,
        nonstd::polymorphic_value<Statement>  then)
        : condExpr(std::move(cond)), thenStmt(std::move(then)) {}
    IfStatement(
        nonstd::polymorphic_value<Expression> cond,
        nonstd::polymorphic_value<Statement>  then,
        nonstd::polymorphic_value<Statement>  else_)
        : condExpr(std::move(cond)), thenStmt(std::move(then)),
          elseStmt(std::move(else_)) {}

protected:
    std::ostream& write_impl(std::ostream& out, size_t indent) const
        noexcept override {
        out << getIndent(indent) << "- ";
        condExpr->write(out, false) << "\n";
        thenStmt->write(out, indent + 4);
        if (elseStmt) {
            elseStmt->write(out, indent) << "\n";
        }
        return out;
    }

private:
    nonstd::polymorphic_value<Expression> condExpr;
    nonstd::polymorphic_value<Statement>  thenStmt;
    nonstd::polymorphic_value<Statement>  elseStmt;
};

// A global variable definition
class GlobalVariableStatement final : public Statement {
public:
    GlobalVariableStatement() noexcept = default;
    explicit GlobalVariableStatement(std::string name, std::string value)
        : varName(std::move(name)), varValue(std::move(value)) {
        add_global(varName);
    }

    static bool add_global(std::string var) {
        return variables.insert(std::move(var)).second;
    }
    static bool is_global(const std::string& var) {
        return variables.find(var) != variables.cend();
    }

private:
    // Global variables
    static inline std::set<std::string> variables;

protected:
    std::ostream& write_impl(std::ostream& out, size_t indent) const
        noexcept final {
        ignore_unused_variable_warning(indent);
        return out << "VAR " << varName << " = " << varValue << '\n';
    }

private:
    std::string varName;
    std::string varValue;
};

// Base for knots, stitches, and functions
class TopLevelStatement : public BlockStatement {
public:
    TopLevelStatement() noexcept = default;
    TopLevelStatement(std::string name_, driver& drv_)
        : drv(&drv_), name(std::move(name_)) {}
    [[nodiscard]] std::string const& getName() const noexcept { return name; }
    [[nodiscard]] bool has_variable(std::string const& var) const {
        return headerVariables.find(var) != headerVariables.cend();
    }
    void add_variable(std::string const& var, bool ref) {
        headerVariables.emplace(var, ref);
    }

protected:
    void init(std::string name_, driver& drv_) {
        drv  = &drv_;
        name = std::move(name_);
    }
    std::ostream& write_header_base(std::ostream& out) const noexcept {
        out << name;
        if (!headerVariables.empty()) {
            out << '(';
            bool firstVar = true;
            for (auto const& [varName, isRef] : headerVariables) {
                if (!firstVar) {
                    out << ", ";
                    firstVar = true;
                }
                if (isRef) {
                    out << "ref ";
                }
                out << varName;
            }
            out << ')';
        }
        return out;
    }
    virtual std::ostream& write_header(std::ostream& out) const noexcept {
        return out;
    }

    std::ostream& write_impl(std::ostream& out, size_t indent) const
        noexcept override {
        write_header(out);
        return BlockStatement::write_impl(out, indent) << '\n';
    }

private:
    driver*                     drv = nullptr;
    std::string                 name;
    std::map<std::string, bool> headerVariables;
};

// Class representing a stitch and all its statements
class StitchStatement final : public TopLevelStatement {
public:
    StitchStatement() noexcept = default;
    StitchStatement(std::string const& name_, driver& drv_) {
        init(name_, drv_);
    }

private:
    std::ostream& write_header(std::ostream& out) const noexcept final {
        out << "= ";
        return write_header_base(out) << '\n';
    }
};

// Class representing a knot and all its stitches and statements
class KnotStatement final : public TopLevelStatement {
public:
    KnotStatement() noexcept = default;
    KnotStatement(std::string const& name_, driver& drv_) { init(name_, drv_); }

    void add_stitch(nonstd::polymorphic_value<StitchStatement> stitch) {
        if (stitch->getName() == getName()) {
            steal_statements(stitch->get_statements());
        } else {
            stitches.emplace_back(std::move(stitch));
        }
    }

private:
    std::ostream& write_header(std::ostream& out) const noexcept final {
        out << "=== ";
        return write_header_base(out) << " ===\n";
    }

protected:
    std::ostream& write_impl(std::ostream& out, size_t indent) const
        noexcept final {
        write_header(out);
        TopLevelStatement::write_impl(out, indent) << '\n';
        for (auto const& elem : stitches) {
            elem->write(out, indent);
        }
        return out << '\n';
    }

private:
    std::vector<nonstd::polymorphic_value<StitchStatement>> stitches;
};

// Class representing a function and all its statements
class FunctionStatement final : public TopLevelStatement {
public:
    FunctionStatement() noexcept = default;
    FunctionStatement(std::string const& name_, driver& drv_) {
        init(name_, drv_);
    }

private:
    std::ostream& write_header(std::ostream& out) const noexcept final {
        out << "=== function ";
        return write_header_base(out) << " ===\n";
    }
};

#endif
