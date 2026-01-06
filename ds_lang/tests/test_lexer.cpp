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
    const std::string code = "int x = 1;print x";
    const auto tokens = lex(code);

    EXPECT_TRUE(!tokens.empty());
    EXPECT_EQ(tokens.back().kind, TokenKind::Eof);

    // int, x, =, 1, ;, print, x, EOF
    EXPECT_EQ(tokens.size(), static_cast<std::size_t>(8));

    expect_token(tokens[0], TokenKind::KWInt, "int", 0, 0);
    expect_token(tokens[1], TokenKind::Identifier, "x", 0, 4);
    expect_token(tokens[2], TokenKind::OpAssign, "=", 0, 6);
    expect_token(tokens[3], TokenKind::Integer, "1", 0, 8);
    expect_token(tokens[4], TokenKind::Eos, ";", 0, 9);
    expect_token(tokens[5], TokenKind::KWPrint, "print", 0, 10);
    expect_token(tokens[6], TokenKind::Identifier, "x", 0, 16);

    EXPECT_EQ(tokens[7].kind, TokenKind::Eof);
    EXPECT_EQ(tokens[7].line, 0);
    EXPECT_EQ(tokens[7].column, 17);
}

static void test_hspace_skipping() {
    const std::string code = "int\t  x\t=\t  42;print\t\tx";
    const auto tokens = lex(code);

    EXPECT_TRUE(!tokens.empty());
    EXPECT_EQ(tokens.back().kind, TokenKind::Eof);

    // int, x, =, 42, ;, print, x, EOF
    EXPECT_EQ(tokens.size(), static_cast<std::size_t>(8));

    EXPECT_EQ(tokens[0].kind, TokenKind::KWInt);
    EXPECT_EQ(tokens[1].kind, TokenKind::Identifier);
    EXPECT_EQ(tokens[2].kind, TokenKind::OpAssign);
    EXPECT_EQ(tokens[3].kind, TokenKind::Integer);
    EXPECT_EQ(tokens[4].kind, TokenKind::Eos);
    EXPECT_EQ(tokens[5].kind, TokenKind::KWPrint);
    EXPECT_EQ(tokens[6].kind, TokenKind::Identifier);
    EXPECT_EQ(tokens[7].kind, TokenKind::Eof);

    EXPECT_EQ(tokens[0].lexeme, "int");
    EXPECT_EQ(tokens[1].lexeme, "x");
    EXPECT_EQ(tokens[2].lexeme, "=");
    EXPECT_EQ(tokens[3].lexeme, "42");
    EXPECT_EQ(tokens[4].lexeme, ";");
    EXPECT_EQ(tokens[5].lexeme, "print");
    EXPECT_EQ(tokens[6].lexeme, "x");
}

static void test_multiple_blank_lines() {
    const std::string code = "int x = 1;;;print x;";
    const auto tokens = lex(code);

    EXPECT_TRUE(!tokens.empty());
    EXPECT_EQ(tokens.back().kind, TokenKind::Eof);

    std::vector<TokenKind> kinds;
    kinds.reserve(tokens.size());
    for (const auto &t : tokens) {
        kinds.push_back(t.kind);
    }

    const std::vector<TokenKind> expected = {
        TokenKind::KWInt,
        TokenKind::Identifier,
        TokenKind::OpAssign,
        TokenKind::Integer,
        TokenKind::Eos,
        TokenKind::Eos,
        TokenKind::Eos,
        TokenKind::KWPrint,
        TokenKind::Identifier,
        TokenKind::Eos,
        TokenKind::Eof,
    };

    EXPECT_EQ(kinds.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(kinds[i], expected[i]);
    }
}

static void test_invalid_digit_throws() {
    // With the current lexer rules, a numeric token is a run of digits.
    // If an unexpected character follows (e.g. '$'), lexing must fail there.
    EXPECT_THROW(lex("int x = 12$;"));
}

static void test_starts_with_zero_throws() {
    EXPECT_THROW(lex("int x = 01;"));
}

static void test_overflow_throws() {
    EXPECT_THROW(lex("int x = 999999999999999999999999999999999999;"));
}

static void test_eof_only_once_and_at_end() {
    const auto tokens = lex("print x");
    int eof_count = 0;
    for (const auto &t : tokens) {
        if (t.kind == TokenKind::Eof) {
            ++eof_count;
        }
    }
    EXPECT_EQ(eof_count, 1);
    EXPECT_EQ(tokens.back().kind, TokenKind::Eof);
}

// ----------------------------------------------------------------------------
// Extra lexer coverage (not referenced by your current CMake LEXER_TEST_CASES)
// ----------------------------------------------------------------------------

static void test_tokenize_range_absolute_line_col() {
    const std::string code = "int x = 1;print x";

    // indices: ';' at 9, 'p' at 10
    const auto tokens = lex_range(code, 10, code.size());

    EXPECT_TRUE(!tokens.empty());
    EXPECT_EQ(tokens.back().kind, TokenKind::Eof);

    // print, x, EOF
    EXPECT_EQ(tokens.size(), static_cast<std::size_t>(3));
    expect_token(tokens[0], TokenKind::KWPrint, "print", 1, 0);
    expect_token(tokens[1], TokenKind::Identifier, "x", 1, 6);

    EXPECT_EQ(tokens[2].kind, TokenKind::Eof);
    EXPECT_EQ(tokens[2].line, 1);
    EXPECT_EQ(tokens[2].column, 7);
}

static void test_two_char_operators() {
    const std::string code =
        "int x = 1;"
        "int y = x == 1 && x != 2 || x <= 3 && x >= 4;";

    const auto tokens = lex(code);

    bool saw_eqeq = false;
    bool saw_neq = false;
    bool saw_andand = false;
    bool saw_oror = false;
    bool saw_le = false;
    bool saw_ge = false;

    for (const auto &t : tokens) {
        saw_eqeq |= (t.kind == TokenKind::OpEqEq);
        saw_neq |= (t.kind == TokenKind::OpNeq);
        saw_andand |= (t.kind == TokenKind::OpAnd);
        saw_oror |= (t.kind == TokenKind::OpOr);
        saw_le |= (t.kind == TokenKind::OpLe);
        saw_ge |= (t.kind == TokenKind::OpGe);
    }

    EXPECT_TRUE(saw_eqeq);
    EXPECT_TRUE(saw_neq);
    EXPECT_TRUE(saw_andand);
    EXPECT_TRUE(saw_oror);
    EXPECT_TRUE(saw_le);
    EXPECT_TRUE(saw_ge);
}

static void test_keyword_prefix_is_identifier() {
    const auto tokens = lex("intx");
    EXPECT_EQ(tokens.size(), static_cast<std::size_t>(2));
    EXPECT_EQ(tokens[0].kind, TokenKind::Identifier);
    EXPECT_EQ(tokens[0].lexeme, "intx");
    EXPECT_EQ(tokens[1].kind, TokenKind::Eof);
}

static void test_delimiters_and_bang() {
    const std::string code = "{ ( ) { } [ ] , ! }";
    const auto tokens = lex(code);

    const std::vector<TokenKind> expected = {
        TokenKind::LBrace,
        TokenKind::LParen,
        TokenKind::RParen,
        TokenKind::LBrace,
        TokenKind::RBrace,
        TokenKind::LBracket,
        TokenKind::RBracket,
        TokenKind::Comma,
        TokenKind::OpBang,
        TokenKind::RBrace,
        TokenKind::Eof,
    };

    EXPECT_EQ(tokens.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(tokens[i].kind, expected[i]);
    }
}

static void test_single_ampersand_throws() {
    // Only && is valid; a lone '&' is an unexpected character.
    EXPECT_THROW(lex("print x & y;"));
}

static void test_single_pipe_throws() {
    // Only || is valid; a lone '|' is an unexpected character.
    EXPECT_THROW(lex("print x | y;"));
}

static void test_tokenize_range_invalid_throws() {
    const std::string code = "print x";
    EXPECT_THROW(lex_range(code, 4, 3));
    EXPECT_THROW(lex_range(code, 0, code.size() + 1));
}

} // namespace ds_lang::Test

int main(int argc, char **argv) {
    using namespace ds_lang::Test;

    struct TestCase {
        const char *name;
        void (*fn)();
    };

    const TestCase tests[] = {
        // Cases referenced from CMake (keep names stable)
        {"basic_two_lines", test_basic_two_lines},
        {"hspace_skipping", test_hspace_skipping},
        {"multiple_blank_lines", test_multiple_blank_lines},
        {"invalid_digit_throws", test_invalid_digit_throws},
        {"starts_with_zero_throws", test_starts_with_zero_throws},
        {"overflow_throws", test_overflow_throws},
        {"eof_only_once_and_at_end", test_eof_only_once_and_at_end},

        // Extra cases (not in CMake's LEXER_TEST_CASES by default)
        {"tokenize_range_absolute_line_col", test_tokenize_range_absolute_line_col},
        {"two_char_operators", test_two_char_operators},
        {"keyword_prefix_is_identifier", test_keyword_prefix_is_identifier},
        {"delimiters_and_bang", test_delimiters_and_bang},
        {"single_ampersand_throws", test_single_ampersand_throws},
        {"single_pipe_throws", test_single_pipe_throws},
        {"tokenize_range_invalid_throws", test_tokenize_range_invalid_throws},
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
        if (!only.empty() && only != t.name) {
            continue;
        }
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