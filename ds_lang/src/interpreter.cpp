// ds_lang/src/interpreter.cpp
#include <algorithm>
#include <cstddef>
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
i64 Interpreter::evaluate_expression(const Expression &expr) {
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
            [&](const CallExpression &s) -> i64 {
                auto *id = std::get_if<IdentifierExpression>(&s.callee->node);
                assert(id && "CallExpression must have IdentifierExpression callee");
                FunctionStatement &func = funcs_.at(id->name);

                if (func.vars.size() != s.args.size()) {
                    throw std::runtime_error(std::format(
                        "When calling {} function we expected {} arguments but got {}",
                        func.func_name, func.vars.size(), s.args.size()));
                }

                const auto caller_vars = vars_;

                ScopeExit restore{[&] {
                    vars_ = caller_vars;
                    for (const auto &[k, v] : caller_vars) {
                        auto it = vars_.find(k);
                        if (it == vars_.end() || it->second != v) {
                            throw std::runtime_error(std::format(
                                "Function call corrupted outer scope variable '{}'", k));
                        }
                    }
                }};

                for (usize i = 0; i < func.vars.size(); ++i) {
                    const std::string &name = func.vars[i];
                    if (vars_.contains(name)) {
                        throw std::runtime_error(std::format(
                            "Variable shadowing is not allowed! Tried to set already existing variable '{}'",
                            name));
                    }
                    vars_.emplace(name, evaluate_expression(*s.args[i]));
                }

                const ExecResult retval = process_scope(func.statements);
                if (retval == ExecResult::Error) {
                    throw std::runtime_error(std::format(
                        "Error in function scope of {}", func.func_name));
                }
                if (retval == ExecResult::Continue) {
                    throw std::runtime_error(std::format(
                        "Function {} must RETURN, but did not.", func.func_name));
                }

                assert(return_value_);
                const i64 tmp = *return_value_;
                return_value_ = std::nullopt;
                return tmp;
            }},
        expr.node);
}

Interpreter::ExecResult Interpreter::process_statement(Statement &statement) {
    return std::visit(
        overloaded{
            [&](LetStatement &s) -> ExecResult {
                assert(s.expr);
                vars_.insert_or_assign(s.identifier, evaluate_expression(*s.expr));
                return ExecResult::Continue;
            },
            [&](PrintStatement &s) -> ExecResult {
                assert(s.expr);
                i64 res = evaluate_expression(*s.expr);
                print_buffer_.push_back(res);
                if (immediate_print_) {
                    std::println("Interpreter Print: [{}]", res);
                }
                return ExecResult::Continue;
            },
            [&](ReturnStatement &s) -> ExecResult {
                assert(s.expr);
                return_value_ = evaluate_expression(*s.expr);
                return ExecResult::Return;
            },
            [&](IfStatement &s) -> ExecResult {
                assert(s.if_expr);
                if (evaluate_expression(*s.if_expr) > 0) {
                    process_scope(s.then_scope);
                } else {
                    process_scope(s.else_scope);
                }
                return ExecResult::Continue;
            },
            [&](WhileStatement &s) -> ExecResult {
                assert(s.while_expr);
                while ((evaluate_expression(*s.while_expr))) {
                    assert(!s.do_scope.empty());
                    process_scope(s.do_scope);
                }
                return ExecResult::Continue;
            },
            [&](FunctionStatement &s) -> ExecResult {
                funcs_.insert_or_assign(s.func_name, std::move(s));
                return ExecResult::Continue;
            },
        },
        statement.node);
}

Interpreter::ExecResult Interpreter::process_scope(std::vector<Statement> &statements) {
    for (Statement &statement : statements) {
        const ExecResult res = process_statement(statement);
        if (res != ExecResult::Continue) {
            return res;
        }
    }
    return ExecResult::Continue;
}
} // namespace ds_lang