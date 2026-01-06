// ds_lang/include/token.hpp
#pragma once

#include <format>
#include <string_view>

namespace ds_lang {
enum class TokenKind {
    Identifier, // variable / function name
    Integer,    // integer literal
    String,     // string literal, must not contain "

    KWInt,      // int
    KWPrint,    // print
    KWFunc,     // func
    KWReturn,   // return
    KWIf,       // if
    KWElse,     // else
    KWWhile,    // while

    LParen,     // (
    RParen,     // )
    LBrace,     // {
    RBrace,     // }
    LBracket,   // [
    RBracket,   // ]
    Comma,      // ,

    OpAssign,   // =
    OpPlus,     // +
    OpMinus,    // -
    OpStar,     // *
    OpSlash,    // /
    OpPercent,  // %
    OpEqEq,     // ==
    OpNeq,      // !=
    OpLt,       // <
    OpLe,       // <=
    OpGt,       // >
    OpGe,       // >=
    OpAndAnd,   // &&
    OpOrOr,     // ||
    OpBang,     // !

    Eos,        // end of statement
    Eof         // end of input
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
    case TokenKind::String:
        return "String";

    case TokenKind::KWInt:
        return "KWInt";
    case TokenKind::KWPrint:
        return "KWPrint";
    case TokenKind::KWFunc:
        return "KWFunc";
    case TokenKind::KWReturn:
        return "KWReturn";
    case TokenKind::KWIf:
        return "KWIf";
    case TokenKind::KWElse:
        return "KWElse";
    case TokenKind::KWWhile:
        return "KWWhile";

    case TokenKind::LParen:
        return "LParen";
    case TokenKind::RParen:
        return "RParen";
    case TokenKind::LBrace:
        return "LBrace";
    case TokenKind::RBrace:
        return "RBrace";
    case TokenKind::LBracket:
        return "LBracket";
    case TokenKind::RBracket:
        return "RBracket";
    case TokenKind::Comma:
        return "Comma";

    case TokenKind::OpAssign:
        return "OpAssign";
    case TokenKind::OpPlus:
        return "OpPlus";
    case TokenKind::OpMinus:
        return "OpMinus";
    case TokenKind::OpStar:
        return "OpStar";
    case TokenKind::OpSlash:
        return "OpSlash";
    case TokenKind::OpPercent:
        return "OpPercent";
    case TokenKind::OpEqEq:
        return "OpEqEq";
    case TokenKind::OpNeq:
        return "OpNeq";
    case TokenKind::OpLt:
        return "OpLt";
    case TokenKind::OpLe:
        return "OpLe";
    case TokenKind::OpGt:
        return "OpGt";
    case TokenKind::OpGe:
        return "OpGe";
    case TokenKind::OpAndAnd:
        return "OpAndAnd";
    case TokenKind::OpOrOr:
        return "OpOrOr";
    case TokenKind::OpBang:
        return "OpBang";

    case TokenKind::Eos:
        return "Eos";
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
    case TokenKind::String:
        return "A string literal. Must not contain \".";

    case TokenKind::KWInt:
        return "Keyword introducing a variable definition or assignment.";
    case TokenKind::KWPrint:
        return "Keyword for printing a value to standard output.";
    case TokenKind::KWFunc:
        return "Keyword introducing a function definition.";
    case TokenKind::KWReturn:
        return "Keyword returning a value from a function.";
    case TokenKind::KWIf:
        return "Keyword starting a conditional expression or statement.";
    case TokenKind::KWElse:
        return "Keyword introducing the else-branch.";
    case TokenKind::KWWhile:
        return "Keyword starting a while loop.";

    case TokenKind::LParen:
        return "Left parenthesis '('.";
    case TokenKind::RParen:
        return "Right parenthesis ')'.";
    case TokenKind::LBrace:
        return "Left Brace '{'.";
    case TokenKind::RBrace:
        return "Left Brace '}'.";
    case TokenKind::LBracket:
        return "Left parenthesis '['.";
    case TokenKind::RBracket:
        return "Right bracket ']'.";
    case TokenKind::Comma:
        return "Comma ',' separator.";

    case TokenKind::OpAssign:
        return "Assignment operator '='.";
    case TokenKind::OpPlus:
        return "Addition operator '+'.";
    case TokenKind::OpMinus:
        return "Subtraction operator '-'.";
    case TokenKind::OpStar:
        return "Multiplication operator '*'.";
    case TokenKind::OpSlash:
        return "Division operator '/'.";
    case TokenKind::OpPercent:
        return "Modulo operator '%'.";
    case TokenKind::OpEqEq:
        return "Equality comparison operator '=='.";
    case TokenKind::OpNeq:
        return "Inequality comparison operator '!='.";
    case TokenKind::OpLt:
        return "Less-than comparison operator '<'.";
    case TokenKind::OpLe:
        return "Less-than-or-equal comparison operator '<='.";
    case TokenKind::OpGt:
        return "Greater-than comparison operator '>'.";
    case TokenKind::OpGe:
        return "Greater-than-or-equal comparison operator '>='.";
    case TokenKind::OpAndAnd:
        return "Logical AND operator '&&'.";
    case TokenKind::OpOrOr:
        return "Logical OR operator '||'.";
    case TokenKind::OpBang:
        return "Logical NOT operator '!'.";

    case TokenKind::Eos:
        return "';' token that terminates a statement.";
    case TokenKind::Eof:
        return "End-of-file marker indicating no more input.";
    }
    return "Unknown token kind.";
}

} // namespace ds_lang

template <>
struct std::formatter<ds_lang::TokenKind> : std::formatter<std::string_view> {
    auto format(ds_lang::TokenKind k, format_context &ctx) const {
        return std::formatter<std::string_view>::format(ds_lang::to_string(k), ctx);
    }
};

template <>
struct std::formatter<ds_lang::Token> : std::formatter<std::string_view> {
    static std::string escape_for_source(std::string_view s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            switch (c) {
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            case '"':  out += "\\\""; break;
            default:   out += c; break;
            }
        }
        return out;
    }

    auto format(const ds_lang::Token &t, format_context &ctx) const {
        std::string lex;
        if (t.kind == ds_lang::TokenKind::String) {
            lex = std::format("\"{}\"", escape_for_source(t.lexeme));
        } else {
            lex = std::format("{:?}", t.lexeme);
        }

        return std::format_to(
            ctx.out(),
            "Token{{kind={}, lexeme={}, line={}, column={}}}",
            t.kind,
            lex,
            t.line,
            t.column);
    }
};