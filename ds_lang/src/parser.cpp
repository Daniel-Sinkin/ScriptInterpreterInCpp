// ds_lang/src/parser.cpp
#include <algorithm>
#include <format>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "formatters.hpp"
#include "parser.hpp"
#include "token.hpp"
#include "util.hpp"

namespace ds_lang {
bool adjacent_no_space(const ds_lang::Token &a, const ds_lang::Token &b) {
    if (a.line != b.line)
        return false;
    const int a_end = a.column + static_cast<int>(a.lexeme.size());
    return b.column == a_end;
}

bool Parser::at_end() const noexcept {
    return pos_ >= tokens_.size();
}

const Token &Parser::peek(usize offset) const {
    if (pos_ + offset >= tokens_.size()) {
        error_here("Trying to peek after EOF");
    }
    return tokens_[pos_ + offset];
}

TokenKind Parser::peek_kind(usize offset = 0) const {
    if (pos_ + offset >= tokens_.size()) {
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
    if (k == TokenKind::OpMinus) {
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

        if (k == TokenKind::OpPeriod && kAccessPrec >= min_bp) {
            const Token &lhs_end_tok = previous(); // last consumed token of lhs
            const Token &dot_tok = peek(0);
            const Token &field_tok = peek(1);

            if (field_tok.kind != TokenKind::Identifier) {
                error_here("Expected identifier after '.'");
            }

            if (!adjacent_no_space(lhs_end_tok, dot_tok) || !adjacent_no_space(dot_tok, field_tok)) {
                error_here("No whitespace allowed around '.' (use a.b)");
            }

            const bool ok_lhs =
                std::holds_alternative<IdentifierExpression>(lhs->node) ||
                std::holds_alternative<StructAccessExpression>(lhs->node);
            if (!ok_lhs) {
                error_here("Field access lhs must be an identifier or a field access");
            }

            (void)advance(); // consume '.'
            (void)advance(); // consume field identifier

            auto e = std::make_unique<Expression>();
            e->node = StructAccessExpression{
                .lhs = std::move(lhs),
                .field_name = std::string(field_tok.lexeme),
            };
            lhs = std::move(e);
            continue;
        }

        if (k == TokenKind::LParen && kCallPrec >= min_bp) {
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

        if (is_expr_terminator(k))
            break;

        const int lbp = infix_lbp(k);
        if (lbp < min_bp)
            break;

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

    if (pos_ + 1 >= tokens_.size()) {
        error_here("Not enough tokens left to make a valid statement.");
    }
    if (k == TokenKind::Identifier && peek_kind(1) == TokenKind::OpAssign) {
        if (peek_kind(2) == TokenKind::LBrace) {
            return {parse_struct_assignment_statement()};
        } else {
            return {parse_int_assignment_statement()};
        }
    }

    if (k == TokenKind::Identifier && peek_kind(1) == TokenKind::Identifier) {
        if (peek_kind(2) == TokenKind::OpAssign) {
            return {parse_struct_declaration_assignment_statement()};
        } else if (peek_kind(2) == TokenKind::Eos) {
            return {parse_struct_declaration_statement()};
        } else {
            error_here("Invalid struct statement");
        }
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
    (void)consume(TokenKind::KWStruct, "Expected 'struct'");
    std::string struct_name{consume(TokenKind::Identifier, "Expected struct name after 'struct'").lexeme};

    (void)consume(TokenKind::LBrace, "Expected '{' to start struct body");

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

        if (peek().kind == TokenKind::OpAssign) {
            error_here("Struct fields cannot have initializers");
        }

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

StructVariableScopeStatement Parser::parse_struct_variable_scope_statement() {
    (void)consume(TokenKind::LBrace, "Expected '{' at start of struct variable scope statement");
    std::vector<Expression> exprs;
    while (peek_kind() != TokenKind::RBrace) {
        auto expr = parse_expression();
        if (!expr) {
            error_here("Failed to parse struct variable scope expression");
        }
        exprs.push_back(std::move(*expr));
        if (peek_kind() == TokenKind::RBrace) {
            break;
        } else {
            (void)consume(TokenKind::Comma, "Struct variable scope expressions must be ',' seperated");
        }
    }
    (void)consume(TokenKind::RBrace, "Expected '{' at start of struct variable scope statement");
    return StructVariableScopeStatement{
        .exprs = std::move(exprs)};
}

StructDeclarationAssignmentStatement Parser::parse_struct_declaration_assignment_statement() {
    std::string struct_name{consume(TokenKind::Identifier, "Expected struct name at start of struct declaration assignment statement").lexeme};
    std::string var_name{consume(TokenKind::Identifier, "Expected var name after struct name").lexeme};
    (void)consume(TokenKind::OpAssign, "Expected = sign after var name");
    return {
        .struct_name = std::move(struct_name),
        .var_name = std::move(var_name),
        .exprs = parse_struct_variable_scope_statement().exprs};
}

StructDeclarationStatement Parser::parse_struct_declaration_statement() {
    std::string struct_name{consume(TokenKind::Identifier, "Expected struct name at start of struct declaration assignment statement").lexeme};
    std::string var_name{consume(TokenKind::Identifier, "Expected var name after struct name").lexeme};
    return {
        .struct_name = std::move(struct_name),
        .var_name = std::move(var_name),
    };
}

StructAssignmentStatement Parser::parse_struct_assignment_statement() {
    std::string var_name{consume(TokenKind::Identifier, "Expected var name after struct name").lexeme};
    (void)consume(TokenKind::OpAssign, "Expected = sign after var name");
    return {
        .var_name = std::move(var_name),
        .exprs = parse_struct_variable_scope_statement().exprs};
}

} // namespace ds_lang
