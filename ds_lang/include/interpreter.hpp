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
constexpr std::string_view to_string(EvaluateExpressionError e) noexcept
{
    switch (e)
    {
    case EvaluateExpressionError::InvalidExpression:
        return "InvalidExpression";
    case EvaluateExpressionError::UnsupportedOperator:
        return "UnsupportedOperator";
    case EvaluateExpressionError::MissingExpression:
        return "MissingExpression";
    case EvaluateExpressionError::MissingVariable:
        return "MissingVariable";
    case EvaluateExpressionError::DivisionByZero:
        return "DivisionByZero";
    }
    return "UnknownEvaluateExpressionError";
}

constexpr std::string_view explain(EvaluateExpressionError e) noexcept
{
    switch (e)
    {
    case EvaluateExpressionError::InvalidExpression:
        return "The expression is malformed or internally inconsistent.";
    case EvaluateExpressionError::UnsupportedOperator:
        return "The operator used in the expression is not supported.";
    case EvaluateExpressionError::MissingExpression:
        return "A required sub-expression is missing.";
    case EvaluateExpressionError::MissingVariable:
        return "The expression references a variable that is not defined.";
    case EvaluateExpressionError::DivisionByZero:
        return "Division by zero occurred during expression evaluation.";
    }
    return "Unknown expression evaluation error.";
}

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

struct ExecuteStatementError
{
    EvaluateExpressionError expression_error;
};
[[nodiscard]] std::expected<void, ExecuteStatementError>
execute_statement(Environment &env, const Statement &statement);
} // namespace ds_lang