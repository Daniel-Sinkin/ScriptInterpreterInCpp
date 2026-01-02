// ds_lang/include/token.hpp
#pragma once
#include <string_view>
#include <format>

namespace ds_lang
{
enum class TokenKind {
    Identifier,
    Integer,
    KWLet,
    KWPrint,
    OpEqual,
    Newline,
    Eof
};
struct Token {
    TokenKind kind;
    std::string_view lexeme;
    int line;
    int column;
};
constexpr std::string_view to_string(TokenKind k) noexcept {
    switch (k) {
    case TokenKind::Identifier:
        return "Identifier";
    case TokenKind::Integer:
        return "Integer";
    case TokenKind::KWLet:
        return "KWLet";
    case TokenKind::KWPrint:
        return "KWPrint";
    case TokenKind::OpEqual:
        return "OpEqual";
    case TokenKind::Newline:
        return "Newline";
    case TokenKind::Eof:
        return "Eof";
    }
    return "UnknownTokenKind";
}

constexpr std::string_view explain(TokenKind k) noexcept {
    switch (k) {
    case TokenKind::Identifier:
        return "A user-defined name (variable or function identifier).";
    case TokenKind::Integer:
        return "A base-10 integer literal.";
    case TokenKind::KWLet:
        return "Keyword introducing a variable definition or assignment.";
    case TokenKind::KWPrint:
        return "Keyword for printing a value to standard output.";
    case TokenKind::OpEqual:
        return "Assignment operator “=”.";
    case TokenKind::Newline:
        return "Newline token that terminates a statement.";
    case TokenKind::Eof:
        return "End-of-file marker indicating no more input.";
    }
    return "Unknown token kind.";
}

} // namespace ds_lang

template <>
struct std::formatter<ds_lang::TokenKind> : std::formatter<std::string_view> {
    auto format(ds_lang::TokenKind k, format_context &ctx) const {
        return std::formatter<std::string_view>::format(ds_lang::to_string(k),
                                                        ctx);
    }
};

template <>
struct std::formatter<ds_lang::Token> : std::formatter<std::string_view> {
    auto format(const ds_lang::Token &t, format_context &ctx) const {
        // Example: Token{kind=KWLet, lexeme="LET", line=0, column=0}
        return std::format_to(
            ctx.out(), "Token{{kind={}, lexeme={:?}, line={}, column={}}}",
            t.kind, t.lexeme, t.line, t.column);
    }
};