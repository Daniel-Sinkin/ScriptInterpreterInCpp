// ds_lang/src/parser.cpp
#include <algorithm>
#include <format>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "parser.hpp"
#include "token.hpp"
#include "util.hpp"
#include "formatters.hpp"

namespace ds_lang {
bool Parser::at_end() const noexcept {
    return pos_ >= tokens_.size();
}

const Token &Parser::peek() const {
    return tokens_[pos_];
}

const Token &Parser::previous() const {
    return tokens_[pos_ - 1];
}

const Token &Parser::advance() {
    if (!at_end())
        ++pos_;
    return previous();
}

void Parser::skip_eos() {
    while (match(TokenKind::Eos)) {
    }
}

bool Parser::match(TokenKind k) {
    if (!at_end() && peek().kind == k) {
        (void)advance();
        return true;
    }
    return false;
}

const Token &Parser::consume(TokenKind k, std::string_view msg) {
    if (!at_end() && peek().kind == k) {
        return advance();
    }
    error_here(msg);
}

void Parser::error_here(std::string_view msg) const {
    const Token &t = peek();
    throw std::runtime_error(std::format(
        "{} (got {} {:?} at line={},column={})",
        msg,
        to_string(t.kind),
        t.lexeme,
        t.line,
        t.column));
}


int Parser::infix_lbp(TokenKind k) noexcept {
    switch (k) {
    case TokenKind::OpOrOr:
        return 20;
    case TokenKind::OpAndAnd:
        return 30;

    case TokenKind::OpEqEq:
    case TokenKind::OpNeq:
        return 40;

    case TokenKind::OpLt:
    case TokenKind::OpLe:
    case TokenKind::OpGt:
    case TokenKind::OpGe:
        return 50;

    case TokenKind::OpPlus:
    case TokenKind::OpMinus:
        return 60;

    case TokenKind::OpStar:
    case TokenKind::OpSlash:
    case TokenKind::OpPercent:
        return 70;

    default:
        return -1;
    }
}

int Parser::infix_rbp(TokenKind k) noexcept {
    const int lbp = infix_lbp(k);
    if (lbp < 0)
        return -1;
    return lbp + 1; // left associative
}

bool Parser::is_infix(TokenKind k) noexcept {
    return infix_lbp(k) >= 0;
}

BinaryOp Parser::to_binary_op(TokenKind k) {
    switch (k) {
    case TokenKind::OpPlus:
        return BinaryOp::Add;
    case TokenKind::OpMinus:
        return BinaryOp::Sub;
    case TokenKind::OpStar:
        return BinaryOp::Mul;
    case TokenKind::OpSlash:
        return BinaryOp::Div;
    case TokenKind::OpPercent:
        return BinaryOp::Mod;

    case TokenKind::OpEqEq:
        return BinaryOp::Eq;
    case TokenKind::OpNeq:
        return BinaryOp::Neq;

    case TokenKind::OpLt:
        return BinaryOp::Lt;
    case TokenKind::OpLe:
        return BinaryOp::Le;
    case TokenKind::OpGt:
        return BinaryOp::Gt;
    case TokenKind::OpGe:
        return BinaryOp::Ge;

    case TokenKind::OpAndAnd:
        return BinaryOp::And;
    case TokenKind::OpOrOr:
        return BinaryOp::Or;

    default:
        throw std::runtime_error("TokenKind is not a binary operator");
    }
}

bool Parser::is_prefix(TokenKind k) noexcept {
    switch (k) {
    case TokenKind::OpMinus:
    case TokenKind::OpBang:
        return true;
    default:
        return false;
    }
}

UnaryOp Parser::to_unary_op(TokenKind k) {
    switch (k) {
    case TokenKind::OpMinus:
        return UnaryOp::Neg;
    case TokenKind::OpBang:
        return UnaryOp::Not;
    default:
        throw std::runtime_error("TokenKind is not a unary operator");
    }
}

std::unique_ptr<Expression> Parser::parse_expression() {
    return parse_expr_bp(0);
}

std::unique_ptr<Expression> Parser::parse_expr_bp(int min_bp) {
    if (at_end()) {
        error_here("Expected expression but reached end of input");
    }

    const Token first = advance();
    auto lhs = nud(first);

    while (!at_end()) {
        const TokenKind k = peek().kind;

        if (k == TokenKind::LParen && kCallPrec >= min_bp) { // Handle postfix function calls
            if (!std::holds_alternative<IdentifierExpression>(lhs->node)) {
                error_here("Only identifiers can be called as functions");
            }

            (void)advance(); // consume '('

            std::vector<std::unique_ptr<Expression>> args;
            if (peek().kind != TokenKind::RParen) {
                while (true) {
                    args.push_back(parse_expr_bp(0));
                    if (match(TokenKind::Comma)) {
                        continue;
                    }
                    break;
                }
            }
            (void)consume(TokenKind::RParen, "Expected ')' after function call arguments");
            auto e = std::make_unique<Expression>();
            e->node = CallExpression{
                .callee = std::move(lhs),
                .args = std::move(args),
            };
            lhs = std::move(e);
            continue;
        }

        if (is_expr_terminator(k)) break;

        const int lbp = infix_lbp(k);
        if (lbp < min_bp) break;

        (void)advance(); // consume operator
        const int rbp = infix_rbp(k);
        auto rhs = parse_expr_bp(rbp);

        auto e = std::make_unique<Expression>();
        e->node = BinaryExpression{
            .op = to_binary_op(k),
            .lhs = std::move(lhs),
            .rhs = std::move(rhs),
        };
        lhs = std::move(e);
    }

    return lhs;
}


std::unique_ptr<Expression> Parser::nud(const Token &t) {
    // prefix operators
    if (is_prefix(t.kind)) {
        const UnaryOp op = to_unary_op(t.kind);
        auto rhs = parse_expr_bp(80); // prefix binds tighter than any infix in our table

        auto e = std::make_unique<Expression>();
        e->node = UnaryExpression{.op = op, .rhs = std::move(rhs)};
        return e;
    }

    switch (t.kind) {
    case TokenKind::Integer: {
        auto v = string_to_i64(t.lexeme);
        if (!v) {
            const StringToIntError err = v.error();
            throw std::runtime_error(std::format(
                "Invalid integer literal {:?}: {} [{}] at line={},column={}",
                t.lexeme, explain(err), to_string(err), t.line, t.column));
        }
        auto e = std::make_unique<Expression>();
        e->node = IntegerExpression{.value = *v};
        return e;
    }
    case TokenKind::Identifier: {
        auto e = std::make_unique<Expression>();
        e->node = IdentifierExpression{.name = std::string(t.lexeme)};
        return e;
    }
    case TokenKind::LParen: {
        auto inner = parse_expr_bp(0);
        (void)consume(TokenKind::RParen, "Expected ')' after parenthesized expression");
        return inner;
    }
    default:
        throw std::runtime_error(std::format(
            "Expected expression, got {} {:?} at line={},column={}",
            to_string(t.kind), t.lexeme, t.line, t.column));
    }
}

Statement Parser::parse_statement() {
    skip_eos();
    if (at_end()) {
        throw std::runtime_error("Trying to parse statement at the end");
    }
    TokenKind k = peek().kind;
    if (k == TokenKind::KWLet) {
        return {parse_let_statement()};
    } else if (k == TokenKind::KWPrint) {
        return {parse_print_statement()};
    } else if (k == TokenKind::KWFunc) {
        return {parse_func_statement()};
    } else if (k == TokenKind::KWReturn) {
        return {parse_return_statement()};
    } else if (k == TokenKind::KWIf) {
        return {parse_if_statement()};
    } else if (k == TokenKind::KWWhile) {
        return {parse_while_statement()};
    } else {
        error_here("Peeked TokenKind can't be a start of a statement scope");
    }
    std::unreachable();
}

std::vector<Statement> Parser::parse_program() {
    std::vector<Statement> statements;
    return statements;
}

std::vector<Statement> Parser::parse_scope()
{
    (void)consume(TokenKind::LBrace, "Scope parsing must start at {");
    std::vector<Statement> statements;
    int i = 0;
    while (true) {
        skip_eos();
        const TokenKind k = peek().kind;
        if (k == TokenKind::RBrace) {
            break;
        }
        statements.push_back(parse_statement());
        ++i;
        if(i > 1000) {
            throw std::runtime_error("Overflow on parse scope");
        }
    }
    return statements;
}

LetStatement Parser::parse_let_statement() {
    (void)consume(TokenKind::KWLet, "Expected 'LET' at start of assignment statement");

    const Token &id = consume(TokenKind::Identifier, "Expected identifier after 'LET'");
    (void)consume(TokenKind::OpAssign, "Expected '=' after identifier in LET statement");

    auto rhs = parse_expr_bp(0);

    while (match(TokenKind::Eos)) {
    }
    return LetStatement{.identifier = std::string(id.lexeme), .expr = std::move(rhs)};
}

PrintStatement Parser::parse_print_statement() {
    (void)consume(TokenKind::KWPrint, "Expected 'PRINT' at start of assignment statement");
    auto rhs = parse_expr_bp(0);
    while (match(TokenKind::Eos)) {
    }
    return PrintStatement{.expr = std::move(rhs)};
}

ReturnStatement Parser::parse_return_statement() {
    (void)consume(TokenKind::KWReturn, "Expected 'RETURN' at start of assignment statement");
    auto rhs = parse_expr_bp(0);
    while (match(TokenKind::Eos)) {
    }
    return ReturnStatement{.expr = std::move(rhs)};
}
IfStatement Parser::parse_if_statement() {
    (void)consume(TokenKind::KWIf, "Expected 'IF' at start of assignment statement");
    auto if_expr = parse_expr_bp(0);
    (void)consume(TokenKind::KWThen, "Expected 'THEN' after 'IF' expression");
    std::vector<Statement> then_scope = parse_scope();
    std::vector<Statement> else_scope{};
    if (match(TokenKind::KWElse)) {
        skip_eos();
        else_scope = parse_scope();
    }
    (void)consume(TokenKind::KWEnd, "Expected 'END' after if statement");
    return IfStatement{
        .if_expr = std::move(if_expr),
        .then_scope = std::move(then_scope),
        .else_scope = std::move(else_scope)};
}
WhileStatement Parser::parse_while_statement() {
    (void)consume(TokenKind::KWWhile, "Expected 'WHILE' at start of assignment statement");
    auto while_expr = parse_expr_bp(0);
    (void)consume(TokenKind::KWDo, "Expected 'DO' after 'WHILE' expression");
    std::vector<Statement> do_scope = parse_scope();
    (void)consume(TokenKind::KWEnd, "Expected 'END' after while do statement");
    return WhileStatement{
        .while_expr = std::move(while_expr),
        .do_scope = std::move(do_scope)};
}
FunctionStatement Parser::parse_func_statement() {
    (void)consume(TokenKind::KWFunc, "Expected 'FUNC' at start of assignment statement");
    std::string func_name{consume(TokenKind::Identifier, "Expected Identifier after FUNC").lexeme};
    (void)consume(TokenKind::LParen, "Expected '(' after function name");
    std::vector<std::string> vars;
    while (peek().kind != TokenKind::RParen) {
        if (peek().kind == TokenKind::Identifier) {
            std::string name{consume(TokenKind::Identifier, "Peeked Function Var Identifier, this shouldn't fail").lexeme};
            if (std::ranges::contains(vars, name)) {
                throw std::runtime_error("Duplicate function arguments");
            }
            vars.push_back(name);
        }
        if (peek().kind == TokenKind::Comma) {
            (void)consume(TokenKind::Comma, "Peeked, shouldn't error");
        }
    }
    (void)consume(TokenKind::RParen, "Expected ')' after function arguments");
    std::vector<Statement> statements = parse_scope();

    return FunctionStatement{
        .func_name = std::move(func_name),
        .vars = std::move(vars),
        .statements = std::move(statements)};
}

} // namespace ds_lang