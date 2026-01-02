// ds_lang/app/main.cpp
#include <print>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

#include "lexer.hpp"
#include "parser.hpp"
#include "token.hpp"
#include "util.hpp"

namespace ds_lang {
struct Interpreter {
    std::unordered_map<std::string, i64> vars_;

i64 evaluate_expression(const Expression& expr)
{
    return std::visit(
        overloaded{
            [&](const IntegerExpression& s) -> i64 {
                return s.value;
            },
            [&](const IdentifierExpression& s) -> i64 {
                return vars_.at(s.name);
            },
            [&](const UnaryExpression& s) -> i64 {
                assert(s.rhs);
                const i64 v = evaluate_expression(*s.rhs);

                switch (s.op) {
                case UnaryOp::Neg:
                    return -v;
                case UnaryOp::Not:
                    return (v != 0) ? 0 : 1;
                }
                throw std::runtime_error("Unhandled UnaryOp");
            },
            [&](const BinaryExpression& s) -> i64 {
                assert(s.lhs);
                assert(s.rhs);

                // Avoid computing second if first is already false
                if (s.op == BinaryOp::And) {
                    const i64 lhs = evaluate_expression(*s.lhs);
                    if (lhs == 0) return 0;
                    const i64 rhs = evaluate_expression(*s.rhs);
                    return (rhs != 0) ? 1 : 0;
                }
                // Avoid computing second if second is already true
                if (s.op == BinaryOp::Or) {
                    const i64 lhs = evaluate_expression(*s.lhs);
                    if (lhs != 0) return 1;
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
                    if (rhs == 0) throw std::runtime_error("Division by zero");
                    return lhs / rhs;
                case BinaryOp::Mod:
                    if (rhs == 0) throw std::runtime_error("Modulo by zero");
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

    void process_statement(const Statement &stmt) {
        std::visit(
            overloaded{
                [&](const LetStatement &s) {
                    assert(s.expr);
                    vars_.insert_or_assign(s.identifier, evaluate_expression(*s.expr));
                },
                [&](const PrintStatement &s) {
                    assert(s.expr);
                    std::println("Interpreter Print: [{}]", evaluate_expression(*s.expr));
                },
                [&](const ReturnStatement &s) {
                    assert(s.expr);
                    std::println("Interpreter Returned: [{}]", evaluate_expression(*s.expr));
                },
                [&](const IfStatement &s) {
                    if(evaluate_expression(*s.if_expr) > 0) {
                        process_statement(*s.then_statement);
                    } else {
                        process_statement(*s.else_statement);
                    }
                },
                [&](const WhileStatement &s) {
                    assert(s.while_expr);
                    while((evaluate_expression(*s.while_expr))) {
                        assert(s.do_statement);
                        process_statement(*s.do_statement);
                    }
                },
                [&](const FunctionStatement &s) {
                    (void)s;
                    throw std::runtime_error("not implemented yet");
                },
            },
            stmt.node);
    }
};
} // namespace ds_lang

int main() {
    using namespace ds_lang;

    std::string code = load_code("examples/simple.ds");
    Lexer lexer{code};
    std::vector<Token> tokens = lexer.tokenize_all();
    Parser parser{tokens};

    Interpreter interpreter;
    interpreter.process_statement(parser.parse_statement());
    interpreter.process_statement(parser.parse_statement());
    interpreter.process_statement(parser.parse_statement());
}