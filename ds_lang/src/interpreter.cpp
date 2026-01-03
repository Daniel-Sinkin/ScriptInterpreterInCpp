// ds_lang/src/interpreter.cpp
#include <algorithm>
#include <optional>
#include <print>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "interpreter.hpp"
#include "parser.hpp"
#include "util.hpp"

namespace ds_lang {
Interpreter::~Interpreter() noexcept {
    try {
        std::println("Interpreter result");

        if (return_value_) {
            std::println("Return: {}", *return_value_);
        } else {
            std::println("Return: <none>");
        }

        std::println("Print buffer: {}", print_buffer_);

        std::vector<std::pair<std::string, i64>> vars_sorted;
        vars_sorted.reserve(vars_.size());
        for (const auto &[k, v] : vars_) {
            vars_sorted.emplace_back(k, v);
        }
        std::sort(vars_sorted.begin(), vars_sorted.end(),
                  [](const auto &a, const auto &b) {
                      return a.first < b.first;
                  });

        std::println("Variables:");
        if (vars_sorted.empty()) {
            std::println("  <empty>");
        } else {
            for (const auto &[name, value] : vars_sorted) {
                std::println("  {} = {}", name, value);
            }
        }
    } catch (...) {
    }
}
i64 Interpreter::evaluate_expression(const Expression &expr) const {
    // TODO: Add error handling and propagating
    return std::visit(
        overloaded{
            [&](const IntegerExpression &s) -> i64 {
                return s.value;
            },
            [&](const IdentifierExpression &s) -> i64 {
                return vars_.at(s.name);
            },
            [&](const UnaryExpression &s) -> i64 {
                assert(s.rhs);
                const i64 v = evaluate_expression(*s.rhs);

                switch (s.op) {
                case UnaryOp::Neg:
                    return -v;
                case UnaryOp::Not:
                    return (v != 0) ? 0 : 1;
                }
                std::unreachable();
            },
            [&](const BinaryExpression &s) -> i64 {
                assert(s.lhs);
                assert(s.rhs);

                // Avoid computing second if first is already false
                if (s.op == BinaryOp::And) {
                    const i64 lhs = evaluate_expression(*s.lhs);
                    if (lhs == 0)
                        return 0;
                    const i64 rhs = evaluate_expression(*s.rhs);
                    return (rhs != 0) ? 1 : 0;
                }
                // Avoid computing second if first is already true
                if (s.op == BinaryOp::Or) {
                    const i64 lhs = evaluate_expression(*s.lhs);
                    if (lhs != 0)
                        return 1;
                    const i64 rhs = evaluate_expression(*s.rhs);
                    return (rhs != 0) ? 1 : 0;
                }

                const i64 lhs = evaluate_expression(*s.lhs);
                const i64 rhs = evaluate_expression(*s.rhs);

                switch (s.op) {
                case BinaryOp::Add:
                    return lhs + rhs;
                case BinaryOp::Sub:
                    return lhs - rhs;
                case BinaryOp::Mul:
                    return lhs * rhs;
                case BinaryOp::Div:
                    if (rhs == 0)
                        throw std::runtime_error("Division by zero");
                    return lhs / rhs;
                case BinaryOp::Mod:
                    if (rhs == 0)
                        throw std::runtime_error("Modulo by zero");
                    return lhs % rhs;
                case BinaryOp::Eq:
                    return (lhs == rhs) ? 1 : 0;
                case BinaryOp::Neq:
                    return (lhs != rhs) ? 1 : 0;
                case BinaryOp::Lt:
                    return (lhs < rhs) ? 1 : 0;
                case BinaryOp::Le:
                    return (lhs <= rhs) ? 1 : 0;
                case BinaryOp::Gt:
                    return (lhs > rhs) ? 1 : 0;
                case BinaryOp::Ge:
                    return (lhs >= rhs) ? 1 : 0;

                case BinaryOp::And:
                case BinaryOp::Or:
                    break;
                }
                std::unreachable();
            },
        },
        expr.node);
}

Interpreter::ExecResult Interpreter::process_statement(const Statement &statement) {
    return std::visit(
        overloaded{
            [&](const LetStatement &s) -> ExecResult {
                assert(s.expr);
                vars_.insert_or_assign(s.identifier, evaluate_expression(*s.expr));
                return ExecResult::Continue;
            },
            [&](const PrintStatement &s) -> ExecResult {
                assert(s.expr);
                i64 res = evaluate_expression(*s.expr);
                print_buffer_.push_back(res);
                if (immediate_print_) {
                    std::println("Interpreter Print: [{}]", res);
                }
                return ExecResult::Continue;
            },
            [&](const ReturnStatement &s) -> ExecResult {
                assert(s.expr);
                return_value_ = evaluate_expression(*s.expr);
                return ExecResult::Return;
            },
            [&](const IfStatement &s) -> ExecResult {
                assert(s.if_expr);
                if (evaluate_expression(*s.if_expr) > 0) {
                    process_statement(*s.then_statement);
                } else {
                    process_statement(*s.else_statement);
                }
                return ExecResult::Continue;
            },
            [&](const WhileStatement &s) -> ExecResult {
                assert(s.while_expr);
                while ((evaluate_expression(*s.while_expr))) {
                    assert(!s.do_scope.empty());
                    process_scope(s.do_scope);
                }
                return ExecResult::Continue;
            },
            [&](const FunctionStatement &s) -> ExecResult {
                (void)s;
                throw std::runtime_error("not implemented yet");
                return ExecResult::Continue;
            },
        },
        statement.node);
}

Interpreter::ExecResult Interpreter::process_scope(const std::vector<Statement> &statements) {
    for (const Statement &statement : statements) {
        const ExecResult res = process_statement(statement);
        if (res != ExecResult::Continue) {
            return res;
        }
    }
    return ExecResult::Continue;
}
} // namespace ds_lang