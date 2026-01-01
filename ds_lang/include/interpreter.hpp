// ds_lang/include/interpreter.hpp
#pragma once

#include <expected>

#include "types.hpp"
#include "env.hpp"
#include "ast.hpp"

namespace ds_lang {
enum class EvaluateExpressionError
{
    InvalidExpression, // Catch all
    UnsupportedOperator,
    MissingExpression,
    MissingVariable,
    DivisionByZero
};
[[nodiscard]] std::expected<i64, EvaluateExpressionError>
evaluate_expression(const Expression &expr, const Environment &env);

struct AssignmentStatement
{
    std::string identifier;
    Expression expr;
};

struct PrintStatement
{
    Expression expr;
};

// Statements either don't do anything, have side effects on the context (set a variable)
// or side effects outside of the context (how function calling is handled idk, but things like printing to console)
using Statement = std::variant<AssignmentStatement, PrintStatement>;

enum class ExecuteStatementError
{
    Generic,
    ExpressionError
};
[[nodiscard]] std::expected<void, ExecuteStatementError>
execute_statement(Environment &env, const Statement &statement);
} // namespace ds_lang