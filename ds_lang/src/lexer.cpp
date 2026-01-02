// src/lexer.cpp
#include <format>
#include <stdexcept>
#include <string_view>

#include "lexer.hpp"
#include "token.hpp"
#include "types.hpp"
#include "util.hpp"

namespace ds_lang {
TokenKind Lexer::determine_token_kind(std::string_view lexeme) const {
    if (lexeme.empty()) {
        throw std::runtime_error("Kind build token out of empty lexeme");
    }
    if (lexeme == "LET") {
        return TokenKind::KWLet;
    } else if (lexeme == "PRINT") {
        return TokenKind::KWPrint;
    } else if (lexeme == "=") {
        return TokenKind::OpEqual;
    } else if (lexeme == "\n") {
        return TokenKind::Newline;
    } else if (char_is_digit(lexeme[0])) {
        auto res = string_to_i64(lexeme);
        if (!res) {
            StringToIntError err = res.error();
            throw std::runtime_error(
                std::format("Failed to convert lexeme {} into an integer: {} [{}]",
                            lexeme, explain(err), to_string(err)));
        }
        return TokenKind::Integer;
    } else {
        if (!is_valid_identifier(lexeme)) {
            throw std::runtime_error(
                std::format("The lexeme {} is not a valid identifier! (line={},column={})",
                            lexeme, line_, column_));
        }
        return TokenKind::Identifier;
    }
}

[[nodiscard]]
std::vector<Token> Lexer::take_tokens() && {
    assert(is_active_);

    while (true) {
        Token t = process_next_token();
        tokens_.push_back(t);
        if (t.kind == TokenKind::Eof) {
            break;
        }
    }
    assert(eof() && !tokens_.empty() && tokens_.back().kind == TokenKind::Eof);
    is_active_ = false;
    return std::move(tokens_);
}

Token Lexer::process_next_token() {
    /// <Contracts
    assert(is_active_);

    const usize old_pos = pos_;
    ScopeExit ensure{[&] {
        assert(pos_ > old_pos || eof());
    }};
    /// Contracts>

    while (!eof() && is_hspace(code_[pos_])) {
        new_char();
    }
    if (eof())
        return Token{TokenKind::Eof, std::string_view{}, line_, column_};
    if (is_newline(code_[pos_])) {
        Token token{TokenKind::Newline, code_.substr(pos_, 1), line_,
                    column_};
        new_line();
        return token;
    }
    const usize word_start = pos_;
    const int tok_line = line_;
    const int tok_col = column_;
    while (!eof() && !is_hspace(code_[pos_]) && !is_newline(code_[pos_])) {
        new_char();
    }
    assert(pos_ > word_start);
    std::string_view lexeme = code_.substr(word_start, pos_ - word_start);
    return {determine_token_kind(lexeme), lexeme, tok_line, tok_col};
}

} // namespace ds_lang