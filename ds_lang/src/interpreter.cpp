// ds_lang/src/interpreter.cpp
#include <expected>
#include <variant>
#include <print>

#include "types.hpp"
#include "env.hpp"
#include "ast.hpp"
#include "interpreter.hpp"

namespace ds_lang {
[[nodiscard]] std::expected<i64, EvaluateExpressionError>
evaluate_expression(const Expression &expr, const Environment &env)
{
    using E = EvaluateExpressionError;
    return std::visit([&env](const auto &e) -> std::expected<i64, EvaluateExpressionError>
        {
        using TT = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<TT, BinaryExpression>) {
            if(!e.exp1) return std::unexpected{E::MissingExpression};
            auto res1 = evaluate_expression(*e.exp1, env);
            if(!res1) {
                return std::unexpected{res1.error()};
            }
            if(!e.exp2) return std::unexpected{E::MissingExpression};
            auto res2 = evaluate_expression(*e.exp2, env);
            if(!res2) {
                return std::unexpected{res2.error()};
            }
            switch(e.op) { // Maybe should have seperate BinaryArithmeic and so on Operators
                case BinaryOperator::Plus:
                    return *res1 + *res2;
                case BinaryOperator::Minus:
                    return *res1 - *res2;
                case BinaryOperator::Star:
                    return (*res1) * (*res2);
                case BinaryOperator::Slash:
                    if (*res2 == 0) return std::unexpected{E::DivisionByZero};
                    return *res1 / *res2;
                case BinaryOperator::GreaterThan:
                    return static_cast<i64>(*res1 > *res2);
                case BinaryOperator::LessThan:
                    return static_cast<i64>(*res1 < *res2);
                case BinaryOperator::GreaterEqualThan:
                    return static_cast<i64>(*res1 >= *res2);
                case BinaryOperator::LessEqualThan:
                    return static_cast<i64>(*res1 <= *res2);
                case BinaryOperator::DoubleEqual:
                    return static_cast<i64>(*res1 == *res2);
                default: 
                    return std::unexpected{E::UnsupportedOperator};
            }
        }
        else if constexpr (std::is_same_v<TT, ValueExpression>)
        {
            return e.value;
        }
        else if constexpr (std::is_same_v<TT, VariableExpression>)
        {
            auto res = env.get(e.name);
            if(!res.has_value()) return std::unexpected{E::MissingVariable};
            return *res;
        } }, expr.node);
}

[[nodiscard]] std::expected<void, ExecuteStatementError>
execute_statement(Environment &env, const Statement &statement)
{
    using E = ExecuteStatementError;
    return std::visit([&env](const auto &s) -> std::expected<void, ExecuteStatementError>
        {
        using TT = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<TT, AssignmentStatement>) {
            auto res = evaluate_expression(s.expr, env);
            if(!res) {
                return std::unexpected{E::ExpressionError};
            }
            env.set(s.identifier, *res);
        }
        else if constexpr (std::is_same_v<TT, PrintStatement>) {
            auto res = evaluate_expression(s.expr, env);
            if(!res) {
                return std::unexpected{E::ExpressionError};
            }
            std::println("Interpreter Printout: '{}'", *res);
        }
        return {}; }, statement);
}

} // namespace ds_lang