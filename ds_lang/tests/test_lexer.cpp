// tests/test_lexer.cpp
#include "common.hpp"

#include <print>
#include <string>
#include <string_view>
#include <vector>

#include "lexer.hpp"
#include "token.hpp"

namespace ds_lang::Test {
static std::vector<Token> lex(std::string_view code) {
    return ds_lang::Lexer{code}.tokenize_all();
}

static std::vector<Token> lex_range(std::string_view code, usize left, usize right) {
    return ds_lang::Lexer{code}.tokenize_range(left, right);
}

static void expect_token(
    const Token &t,
    TokenKind kind,
    std::string_view lexeme,
    int line,
    int column) {
    EXPECT_EQ(t.kind, kind);
    EXPECT_EQ(t.lexeme, lexeme);
    EXPECT_EQ(t.line, line);
    EXPECT_EQ(t.column, column);
}

static void test_basic_two_lines() {
    const std::string code = "LET x = 1\nPRINT x";
    const auto tokens = lex(code);

    EXPECT_TRUE(!tokens.empty());
    EXPECT_EQ(tokens.back().kind, TokenKind::Eof);

    // LET, x, =, 1, \n, PRINT, x, EOF
    EXPECT_EQ(tokens.size(), static_cast<std::size_t>(8));

    expect_token(tokens[0], TokenKind::KWLet, "LET", 0, 0);
    expect_token(tokens[1], TokenKind::Identifier, "x", 0, 4);
    expect_token(tokens[2], TokenKind::OpAssign, "=", 0, 6);
    expect_token(tokens[3], TokenKind::Integer, "1", 0, 8);
    expect_token(tokens[4], TokenKind::Newline, "\n", 0, 9);
    expect_token(tokens[5], TokenKind::KWPrint, "PRINT", 1, 0);
    expect_token(tokens[6], TokenKind::Identifier, "x", 1, 6);

    EXPECT_EQ(tokens[7].kind, TokenKind::Eof);
    EXPECT_EQ(tokens[7].line, 1);
    EXPECT_EQ(tokens[7].column, 7);
}

static void test_hspace_skipping() {
    const std::string code = "LET\t  x\t=\t  42\nPRINT\t\tx";
    const auto tokens = lex(code);

    EXPECT_TRUE(!tokens.empty());
    EXPECT_EQ(tokens.back().kind, TokenKind::Eof);

    // LET, x, =, 42, \n, PRINT, x, EOF
    EXPECT_EQ(tokens.size(), static_cast<std::size_t>(8));
    EXPECT_EQ(tokens[0].kind, TokenKind::KWLet);
    EXPECT_EQ(tokens[1].kind, TokenKind::Identifier);
    EXPECT_EQ(tokens[2].kind, TokenKind::OpAssign);
    EXPECT_EQ(tokens[3].kind, TokenKind::Integer);
    EXPECT_EQ(tokens[4].kind, TokenKind::Newline);
    EXPECT_EQ(tokens[5].kind, TokenKind::KWPrint);
    EXPECT_EQ(tokens[6].kind, TokenKind::Identifier);
    EXPECT_EQ(tokens[7].kind, TokenKind::Eof);

    EXPECT_EQ(tokens[0].lexeme, "LET");
    EXPECT_EQ(tokens[1].lexeme, "x");
    EXPECT_EQ(tokens[2].lexeme, "=");
    EXPECT_EQ(tokens[3].lexeme, "42");
    EXPECT_EQ(tokens[4].lexeme, "\n");
    EXPECT_EQ(tokens[5].lexeme, "PRINT");
    EXPECT_EQ(tokens[6].lexeme, "x");
}

static void test_multiple_blank_lines() {
    const std::string code = "LET x = 1\n\n\nPRINT x\n";
    const auto tokens = lex(code);

    EXPECT_TRUE(!tokens.empty());
    EXPECT_EQ(tokens.back().kind, TokenKind::Eof);

    std::vector<TokenKind> kinds;
    kinds.reserve(tokens.size());
    for (const auto &t : tokens)
        kinds.push_back(t.kind);

    const std::vector<TokenKind> expected = {
        TokenKind::KWLet,
        TokenKind::Identifier,
        TokenKind::OpAssign,
        TokenKind::Integer,
        TokenKind::Newline,
        TokenKind::Newline,
        TokenKind::Newline,
        TokenKind::KWPrint,
        TokenKind::Identifier,
        TokenKind::Newline,
        TokenKind::Eof};
    EXPECT_EQ(kinds.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(kinds[i], expected[i]);
    }
}

static void test_invalid_digit_throws() {
    // With the new lexer, "12a" is tokenized as Integer("12") then Identifier("a"),
    // so lexing itself should not throw.
    EXPECT_NO_THROW(lex("LET x = 12a\n"));
}

static void test_starts_with_zero_throws() {
    EXPECT_THROW(lex("LET x = 01\n"));
}

static void test_overflow_throws() {
    EXPECT_THROW(lex("LET x = 999999999999999999999999999999999999\n"));
}

static void test_eof_only_once_and_at_end() {
    const auto tokens = lex("PRINT x");
    int eof_count = 0;
    for (const auto &t : tokens) {
        if (t.kind == TokenKind::Eof)
            ++eof_count;
    }
    EXPECT_EQ(eof_count, 1);
    EXPECT_EQ(tokens.back().kind, TokenKind::Eof);
}

static void test_tokenize_range_absolute_line_col() {
    const std::string code = "LET x = 1\nPRINT x";

    // indices: newline at 9, 'P' at 10
    const auto tokens = lex_range(code, 10, code.size());

    EXPECT_TRUE(!tokens.empty());
    EXPECT_EQ(tokens.back().kind, TokenKind::Eof);

    // PRINT, x, EOF
    EXPECT_EQ(tokens.size(), static_cast<std::size_t>(3));
    expect_token(tokens[0], TokenKind::KWPrint, "PRINT", 1, 0);
    expect_token(tokens[1], TokenKind::Identifier, "x", 1, 6);

    EXPECT_EQ(tokens[2].kind, TokenKind::Eof);
    EXPECT_EQ(tokens[2].line, 1);
    EXPECT_EQ(tokens[2].column, 7);
}

static void test_two_char_operators() {
    const std::string code = "LET x = 1\nLET y = x == 1 && x != 2 || x <= 3 && x >= 4\n";
    const auto tokens = lex(code);

    // Just sanity-check presence/order of a few key operator tokens.
    bool saw_eqeq = false;
    bool saw_neq = false;
    bool saw_andand = false;
    bool saw_oror = false;
    bool saw_le = false;
    bool saw_ge = false;

    for (const auto &t : tokens) {
        if (t.kind == TokenKind::OpEqEq)
            saw_eqeq = true;
        if (t.kind == TokenKind::OpNeq)
            saw_neq = true;
        if (t.kind == TokenKind::OpAndAnd)
            saw_andand = true;
        if (t.kind == TokenKind::OpOrOr)
            saw_oror = true;
        if (t.kind == TokenKind::OpLe)
            saw_le = true;
        if (t.kind == TokenKind::OpGe)
            saw_ge = true;
    }

    EXPECT_TRUE(saw_eqeq);
    EXPECT_TRUE(saw_neq);
    EXPECT_TRUE(saw_andand);
    EXPECT_TRUE(saw_oror);
    EXPECT_TRUE(saw_le);
    EXPECT_TRUE(saw_ge);
}

} // namespace ds_lang::Test

int main(int argc, char **argv) {
    using namespace ds_lang::Test;

    struct TestCase {
        const char *name;
        void (*fn)();
    };

    const TestCase tests[] = {
        {"basic_two_lines", test_basic_two_lines},
        {"hspace_skipping", test_hspace_skipping},
        {"multiple_blank_lines", test_multiple_blank_lines},
        {"invalid_digit_throws", test_invalid_digit_throws},
        {"starts_with_zero_throws", test_starts_with_zero_throws},
        {"overflow_throws", test_overflow_throws},
        {"eof_only_once_and_at_end", test_eof_only_once_and_at_end},
        {"tokenize_range_absolute_line_col", test_tokenize_range_absolute_line_col},
        {"two_char_operators", test_two_char_operators},
    };

    std::string_view only;
    if (argc == 3 && std::string_view(argv[1]) == "--case") {
        only = argv[2];
    } else if (argc != 1) {
        std::println("Usage: {} [--case <name>]", argv[0]);
        return 2;
    }

    int passed = 0;
    int ran = 0;

    for (const auto &t : tests) {
        if (!only.empty() && only != t.name)
            continue;
        ++ran;
        try {
            t.fn();
            ++passed;
        } catch (const std::exception &e) {
            std::println("FAIL {}: {}", t.name, e.what());
            return 1;
        } catch (...) {
            std::println("FAIL {}: unknown exception", t.name);
            return 1;
        }
    }

    if (!only.empty() && ran == 0) {
        std::println("Unknown test case: {}", only);
        return 2;
    }

    std::println("OK {}/{}", passed, ran);
    return 0;
}