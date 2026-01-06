// ds_lang/include/parser.hpp
#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "formatters.hpp" // IWYU pragma: keep
#include "token.hpp"
#include "types.hpp"

namespace ds_lang {

enum class BinaryOp {
    Add, // +
    Sub, // -
    Mul, // *
    Div, // /
    Mod, // %

    Eq,  // ==
    Neq, // !=

    Lt, // <
    Le, // <=
    Gt, // >
    Ge, // >=

    And, // &&
    Or,  // ||
};

enum class UnaryOp {
    Neg, // -
    Not, // !
};

struct Expression;

struct IntegerExpression {
    i64 value;
};

struct IdentifierExpression {
    std::string name;
};

struct UnaryExpression {
    UnaryOp op;
    std::unique_ptr<Expression> rhs;
};

struct BinaryExpression {
    BinaryOp op;
    std::unique_ptr<Expression> lhs;
    std::unique_ptr<Expression> rhs;
};

struct CallExpression {
    std::unique_ptr<Expression> callee;
    std::vector<std::unique_ptr<Expression>> args;
};

struct StructAccessExpression {
    std::unique_ptr<Expression> lhs;
    std::string field_name;
};

struct Expression {
    using Variant = std::variant<
        IntegerExpression,
        IdentifierExpression,
        UnaryExpression,
        BinaryExpression,
        CallExpression,
        StructAccessExpression>;
    Variant node;
};

struct Statement;

struct IntDeclarationAssignmentStatement { // int x = 1;
    std::string identifier;
    std::unique_ptr<Expression> expr;
};

struct IntDeclarationStatement { // int x;
    std::string identifier;
};

struct IntAssignmentStatement { // x = 1;
    std::string identifier;
    std::unique_ptr<Expression> expr;
};

struct PrintStatement {
    std::unique_ptr<Expression> expr;
};

struct PrintStringStatement {
    std::string content;
};

struct ReturnStatement {
    std::unique_ptr<Expression> expr;
};

struct ScopeStatement {
    std::vector<Statement> scope;
};

struct IfStatement {
    std::unique_ptr<Expression> if_expr;
    std::vector<Statement> then_scope;
    std::vector<Statement> else_scope;
};

struct WhileStatement {
    std::unique_ptr<Expression> while_expr;
    std::vector<Statement> do_scope;
};

struct FunctionStatement {
    std::string func_name;
    std::vector<std::string> vars;
    std::vector<Statement> statements;
};

struct StructStatement {
    std::string struct_name;
    std::vector<std::string> vars;
};

struct StructDeclarationAssignmentStatement {
    std::string struct_name;
    std::string var_name;
    std::vector<Expression> exprs;
};

struct StructDeclarationStatement {
    std::string struct_name;
    std::string var_name;
};

struct StructAssignmentStatement {
    std::string var_name;
    std::vector<Expression> exprs;
};

struct StructVariableScopeStatement {
    std::vector<Expression> exprs;
};


struct Statement {
    using Variant = std::variant<
        IntDeclarationAssignmentStatement,
        IntDeclarationStatement,
        IntAssignmentStatement,
        PrintStatement,
        PrintStringStatement,
        ReturnStatement,
        IfStatement,
        WhileStatement,
        FunctionStatement,
        StructStatement,
        StructDeclarationAssignmentStatement,
        StructDeclarationStatement,
        StructAssignmentStatement,
        ScopeStatement>;
    Variant node;
};

class Parser {
public:
    explicit Parser(const std::vector<Token> &tokens) noexcept : tokens_(tokens), pos_(0) {}

    Parser() = delete;
    Parser(const Parser &) = default;
    Parser &operator=(const Parser &) = delete;

    static constexpr int kUnaryPrec = 80;
    static constexpr int kCallPrec = 90;
    static constexpr int kAccessPrec = 100;

    // Program is a sequence of *top-level declarations*:
    // - func ...
    // - struct ...
    [[nodiscard]] std::vector<Statement> parse_program();

    // "Normal" statements allowed inside { ... } blocks (no struct allowed).
    [[nodiscard]] Statement parse_statement();
    [[nodiscard]] std::vector<Statement> parse_scope();

    [[nodiscard]] IntDeclarationAssignmentStatement parse_int_declaration_assignment_statement();
    [[nodiscard]] IntDeclarationStatement parse_int_declaration_statement();
    [[nodiscard]] IntAssignmentStatement parse_int_assignment_statement();
    [[nodiscard]] PrintStatement parse_print_statement();
    [[nodiscard]] PrintStringStatement parse_print_string_statement();
    [[nodiscard]] ReturnStatement parse_return_statement();
    [[nodiscard]] ScopeStatement parse_scope_statement();
    [[nodiscard]] IfStatement parse_if_statement();
    [[nodiscard]] WhileStatement parse_while_statement();
    [[nodiscard]] FunctionStatement parse_func_statement();
    [[nodiscard]] StructVariableScopeStatement parse_struct_variable_scope_statement();
    [[nodiscard]] StructStatement parse_struct_statement();
    [[nodiscard]] StructDeclarationAssignmentStatement parse_struct_declaration_assignment_statement();
    [[nodiscard]] StructDeclarationStatement parse_struct_declaration_statement();
    [[nodiscard]] StructAssignmentStatement parse_struct_assignment_statement();

private:
    const std::vector<Token> &tokens_;
    usize pos_;

    enum class Context { TopLevel,
                         Block };
    Context ctx_{Context::TopLevel};

    [[nodiscard]] std::unique_ptr<Expression> parse_expression();

    [[nodiscard]] bool at_end() const noexcept;
    [[nodiscard]] const Token &peek(usize offset = 0) const;
    [[nodiscard]] TokenKind peek_kind(usize offset) const;
    [[nodiscard]] const Token &previous() const;
    [[nodiscard]] const Token &advance();
    [[nodiscard]] bool match(TokenKind k);
    [[nodiscard]] const Token &consume(TokenKind kind, std::string_view msg);

    void skip_eos();

    [[nodiscard]] std::unique_ptr<Expression> parse_expr_bp(int min_bp);
    [[nodiscard]] std::unique_ptr<Expression> nud(const Token &t);

    [[nodiscard]] static int infix_lbp(TokenKind k) noexcept;
    [[nodiscard]] static int infix_rbp(TokenKind k) noexcept;

    [[nodiscard]] static bool is_infix(TokenKind k) noexcept;
    [[nodiscard]] static BinaryOp to_binary_op(TokenKind k);
    [[nodiscard]] static bool is_prefix(TokenKind k) noexcept;
    [[nodiscard]] static UnaryOp to_unary_op(TokenKind k);

    [[noreturn]] void error_here(std::string_view msg) const;

    static bool is_expr_terminator(TokenKind k) noexcept {
        switch (k) {
        case TokenKind::Eos:
        case TokenKind::Eof:
        case TokenKind::RParen:
        case TokenKind::Comma:
        case TokenKind::KWElse:
            return true;
        default:
            return false;
        }
    }
};

} // namespace ds_lang