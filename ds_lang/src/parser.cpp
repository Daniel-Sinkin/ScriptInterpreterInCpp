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

const Token &Parser::peek(usize offset) const {
    if (pos_ + offset >= tokens_.size()) {
        error_here("Trying to peek after EOF");
    }
    return tokens_[pos_ + offset];
}

TokenKind Parser::peek_kind(usize offset) const {
    if(pos_ + offset >= tokens_.size()) {
        return TokenKind::Eof;
    }
    return tokens_[pos_ + offset].kind;
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
    case TokenKind::OpOr:
        return 20;
    case TokenKind::OpAnd:
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

    case TokenKind::OpAnd:
        return BinaryOp::And;
    case TokenKind::OpOr:
        return BinaryOp::Or;
    default:
        throw std::runtime_error(std::format("TokenKind {} is not valid for to_binary_op.", k));
    }
}

bool Parser::is_prefix(TokenKind k) noexcept {
    return (k == TokenKind::OpMinus || k == TokenKind::OpBang);
}

UnaryOp Parser::to_unary_op(TokenKind k) {
    if(k == TokenKind::OpMinus) {
        return UnaryOp::Neg;
    } else if (k == TokenKind::OpBang) {
        return UnaryOp::Not;
    } else {
        throw std::runtime_error(std::format("Invalid TokenKind '{}' for to_unary_op", k));
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

static std::unique_ptr<Expression> make_int_literal(i64 v) {
    auto e = std::make_unique<Expression>();
    e->node = IntegerExpression{.value = v};
    return e;
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
        return make_int_literal(*v);
    }
    case TokenKind::KWTrue: {
        return make_int_literal(1);
    }
    case TokenKind::KWFalse: {
        return make_int_literal(0);
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
    if (at_end())
        throw std::runtime_error("Trying to parse statement at the end");

    // Enforce: no struct or func inside block scope.
    if (ctx_ != Context::TopLevel) {
        if (peek().kind == TokenKind::KWStruct) {
            error_here("struct is only allowed at global scope");
        }
        if (peek().kind == TokenKind::KWFunc) {
            error_here("func is only allowed at global scope");
        }
    }

    const TokenKind k = peek().kind;

    if (k == TokenKind::KWInt) {
        // Decide between:
        //   int x = expr;
        //   int x;
        if (peek_kind(1) != TokenKind::Identifier) {
            error_here("Expected identifier after 'int'");
        }
        if (peek_kind(2) == TokenKind::OpAssign) {
            return {parse_int_declaration_assignment_statement()};
        }
        return {parse_int_declaration_statement()};
    }

    if (k == TokenKind::KWPrint) {
        if (peek_kind(1) == TokenKind::String)
            return {parse_print_string_statement()};
        return {parse_print_statement()};
    }
    if (k == TokenKind::KWReturn)
        return {parse_return_statement()};
    if (k == TokenKind::KWIf)
        return {parse_if_statement()};
    if (k == TokenKind::KWWhile)
        return {parse_while_statement()};
    if (k == TokenKind::LBrace)
        return {parse_scope_statement()};

    if (k == TokenKind::Identifier && peek_kind(1) == TokenKind::OpAssign) {
        return {parse_int_assignment_statement()};
    }

    error_here("Peeked TokenKind can't be a start of a statement");
}

std::vector<Statement> Parser::parse_program() {
    std::vector<Statement> out;

    ctx_ = Context::TopLevel;

    skip_eos();
    while (peek().kind != TokenKind::Eof) {
        const TokenKind k = peek().kind;

        if (k == TokenKind::KWFunc) {
            out.push_back(Statement{parse_func_statement()});
        } else if (k == TokenKind::KWStruct) {
            out.push_back(Statement{parse_struct_statement()});
        } else {
            error_here("Only 'func' and 'struct' declarations are allowed at global scope");
        }

        skip_eos();
    }
    return out;
}

std::vector<Statement> Parser::parse_scope() {
    // Called for normal block scopes (function body, if/else, while, or standalone { }).
    // Enforce block context.
    (void)consume(TokenKind::LBrace, "Scope parsing must start at '{'");

    const auto prev = ctx_;
    ctx_ = Context::Block;

    std::vector<Statement> statements;
    int guard = 0;

    while (true) {
        skip_eos();
        if (peek().kind == TokenKind::RBrace) {
            (void)consume(TokenKind::RBrace, "Expected '}'");
            break;
        }

        statements.push_back(parse_statement());

        if (++guard > 100000) {
            throw std::runtime_error("parse_scope: runaway parse (guard hit)");
        }
    }

    ctx_ = prev;
    return statements;
}

IntDeclarationAssignmentStatement Parser::parse_int_declaration_assignment_statement() {
    (void)consume(TokenKind::KWInt, "Expected 'int' at start of declaration statement");
    const Token &id = consume(TokenKind::Identifier, "Expected identifier after 'int'");
    (void)consume(TokenKind::OpAssign, "Expected '=' after identifier in int declaration");
    auto rhs = parse_expr_bp(0);
    while (match(TokenKind::Eos)) {
    }
    return IntDeclarationAssignmentStatement{.identifier = std::string(id.lexeme), .expr = std::move(rhs)};
}

IntDeclarationStatement Parser::parse_int_declaration_statement() {
    (void)consume(TokenKind::KWInt, "Expected 'int' at start of declaration statement");
    const Token &id = consume(TokenKind::Identifier, "Expected identifier after 'int'");
    while (match(TokenKind::Eos)) {
    }
    return IntDeclarationStatement{.identifier = std::string(id.lexeme)};
}

IntAssignmentStatement Parser::parse_int_assignment_statement() {
    const Token &id = consume(TokenKind::Identifier, "Expected identifier");
    (void)consume(TokenKind::OpAssign, "Expected '=' after identifier in assignment");
    auto rhs = parse_expr_bp(0);
    while (match(TokenKind::Eos)) {
    }
    return IntAssignmentStatement{.identifier = std::string(id.lexeme), .expr = std::move(rhs)};
}

PrintStatement Parser::parse_print_statement() {
    (void)consume(TokenKind::KWPrint, "Expected 'print' at start of PrintStatement");
    auto rhs = parse_expr_bp(0);
    while (match(TokenKind::Eos)) {
    }
    return PrintStatement{.expr = std::move(rhs)};
}

PrintStringStatement Parser::parse_print_string_statement() {
    (void)consume(TokenKind::KWPrint, "Expected 'print' at start of PrintStringStatement");
    const Token &token = consume(TokenKind::String, "Expected string literal after 'print'");
    while (match(TokenKind::Eos)) {
    }
    return PrintStringStatement{.content = std::string{token.lexeme}};
}

ReturnStatement Parser::parse_return_statement() {
    (void)consume(TokenKind::KWReturn, "Expected 'return' at start of ReturnStatement");
    auto rhs = parse_expr_bp(0);
    while (match(TokenKind::Eos)) {
    }
    return ReturnStatement{.expr = std::move(rhs)};
}

ScopeStatement Parser::parse_scope_statement() { return ScopeStatement{.scope = parse_scope()}; }

WhileStatement Parser::parse_while_statement() {
    (void)consume(TokenKind::KWWhile, "Expected 'while'");
    (void)consume(TokenKind::LParen, "Expected '(' after while");
    auto while_expr = parse_expr_bp(0);
    (void)consume(TokenKind::RParen, "Expected ')' after while condition");
    auto do_scope = parse_scope();
    return WhileStatement{.while_expr = std::move(while_expr), .do_scope = std::move(do_scope)};
}

IfStatement Parser::parse_if_statement() {
    (void)consume(TokenKind::KWIf, "Expected 'if'");
    (void)consume(TokenKind::LParen, "Expected '(' after if");
    auto if_expr = parse_expr_bp(0);
    (void)consume(TokenKind::RParen, "Expected ')' after if condition");

    auto then_scope = parse_scope();
    std::vector<Statement> else_scope{};
    if (match(TokenKind::KWElse)) {
        skip_eos();
        else_scope = parse_scope();
    }

    return IfStatement{
        .if_expr = std::move(if_expr),
        .then_scope = std::move(then_scope),
        .else_scope = std::move(else_scope),
    };
}

FunctionStatement Parser::parse_func_statement() {
    // Must be top-level (enforced by parse_program)
    (void)consume(TokenKind::KWFunc, "Expected 'func'");
    std::string func_name{consume(TokenKind::Identifier, "Expected function name after 'func'").lexeme};

    (void)consume(TokenKind::LParen, "Expected '(' after function name");
    std::vector<std::string> vars;

    while (peek().kind != TokenKind::RParen) {
        if (peek().kind == TokenKind::Identifier) {
            std::string name{consume(TokenKind::Identifier, "Expected parameter name").lexeme};
            if (std::ranges::contains(vars, name)) {
                throw std::runtime_error("Duplicate function arguments");
            }
            vars.push_back(std::move(name));
        } else {
            error_here("Expected identifier in parameter list");
        }

        if (peek().kind == TokenKind::Comma) {
            (void)consume(TokenKind::Comma, "Expected ','");
        } else {
            break;
        }
    }

    (void)consume(TokenKind::RParen, "Expected ')' after function arguments");
    auto statements = parse_scope();

    return FunctionStatement{
        .func_name = std::move(func_name),
        .vars = std::move(vars),
        .statements = std::move(statements),
    };
}

StructStatement Parser::parse_struct_statement() {
    // Must be top-level (enforced by parse_program)
    (void)consume(TokenKind::KWStruct, "Expected 'struct'");
    std::string struct_name{consume(TokenKind::Identifier, "Expected struct name after 'struct'").lexeme};

    (void)consume(TokenKind::LBrace, "Expected '{' to start struct body");

    // Struct body parsing is custom: only "int <ident> ;" allowed.
    std::vector<std::string> vars;
    int guard = 0;

    while (true) {
        skip_eos();
        if (peek().kind == TokenKind::RBrace) {
            (void)consume(TokenKind::RBrace, "Expected '}'");
            break;
        }

        (void)consume(TokenKind::KWInt, "Struct fields must start with 'int'");
        std::string field{consume(TokenKind::Identifier, "Expected field name").lexeme};

        // No assignments inside struct for now.
        if (peek().kind == TokenKind::OpAssign) {
            error_here("Struct fields cannot have initializers");
        }

        // Require at least one ';' (allows extra ';' via skip_eos in loop).
        (void)consume(TokenKind::Eos, "Expected ';' after struct field declaration");

        if (std::ranges::contains(vars, field)) {
            throw std::runtime_error("Duplicate struct field: " + field);
        }
        vars.push_back(std::move(field));

        if (++guard > 100000) {
            throw std::runtime_error("parse_struct_statement: runaway parse (guard hit)");
        }
    }

    return StructStatement{
        .struct_name = std::move(struct_name),
        .vars = std::move(vars),
    };
}

} // namespace ds_lang