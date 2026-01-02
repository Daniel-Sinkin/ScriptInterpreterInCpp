// ds_lang/include/parser.hpp
#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "token.hpp"
#include "types.hpp"

namespace ds_lang {
enum class BinaryOp {
    Add,    // +
    Sub,    // -
    Mul,    // *
    Div,    // /
    Mod,    // %

    Eq,     // ==
    Neq,    // !=

    Lt,     // <
    Le,     // <=
    Gt,     // >
    Ge,     // >=

    And,    // &&
    Or,     // ||
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

struct Expression {
    using Variant = std::variant<
        IntegerExpression,
        IdentifierExpression,
        UnaryExpression,
        BinaryExpression>;

    Variant node;
};

struct Statement;

struct LetStatement {
    std::string identifier;
    std::unique_ptr<Expression> expr;
};

struct PrintStatement {
    std::unique_ptr<Expression> expr;
};

struct ReturnStatement {
    std::unique_ptr<Expression> expr;
};

struct IfStatement {
    std::unique_ptr<Expression> if_expr;
    // Need a variable SCOPE not a single statement, a scope would be a std::vector of statements
    std::unique_ptr<Statement> then_statement;
    // Need a variable SCOPE not a single statement, a scope would be a std::vector of statements
    std::unique_ptr<Statement> else_statement;
};

struct WhileStatement {
    std::unique_ptr<Expression> while_expr;
    // Need a variable SCOPE not a single statement, a scope would be a std::vector of statements
    std::unique_ptr<Statement> do_statement;
};

struct FunctionStatement {
    std::string_view func_name;
    std::vector<std::string_view> vars;
    std::unique_ptr<Statement> statement;
};

struct Statement {
    using Variant = std::variant<
        LetStatement,
        PrintStatement,
        ReturnStatement,
        IfStatement,
        WhileStatement,
        FunctionStatement>;
    Variant node;
};

// ===== Parser =====

class Parser {
public:
    explicit Parser(const std::vector<Token> &tokens) noexcept
        : tokens_(tokens), pos_(0) {}

    Parser() = delete;
    Parser(const Parser &) = default;
    Parser &operator=(const Parser &) = delete;

    [[nodiscard]] Statement parse_statement();
    [[nodiscard]] LetStatement parse_let_statement();
    [[nodiscard]] PrintStatement parse_print_statement();
    [[nodiscard]] ReturnStatement parse_return_statement();
    [[nodiscard]] IfStatement parse_if_statement();
    [[nodiscard]] WhileStatement parse_while_statement();
    [[nodiscard]] FunctionStatement parse_func_statement();

private:
    const std::vector<Token> &tokens_;
    usize pos_;

    [[nodiscard]] std::unique_ptr<Expression> parse_expression();

    [[nodiscard]] bool at_end() const noexcept;
    [[nodiscard]] const Token &peek() const;
    [[nodiscard]] const Token &previous() const;
    [[nodiscard]] const Token &advance();
    [[nodiscard]] bool match(TokenKind k);
    [[nodiscard]] const Token &consume(TokenKind kind, std::string_view msg);

    void skip_newlines();

    [[nodiscard]] std::unique_ptr<Expression> parse_expr_bp(int min_bp);
    [[nodiscard]] std::unique_ptr<Expression> nud(const Token &t);

    [[nodiscard]] static int infix_lbp(TokenKind k) noexcept;
    [[nodiscard]] static int infix_rbp(TokenKind k) noexcept;

    [[nodiscard]] static bool is_infix(TokenKind k) noexcept;
    [[nodiscard]] static BinaryOp to_binary_op(TokenKind k);
    [[nodiscard]] static bool is_prefix(TokenKind k) noexcept;
    [[nodiscard]] static UnaryOp to_unary_op(TokenKind k);

    [[noreturn]] void error_here(std::string_view msg) const;

};

} // namespace ds_lang