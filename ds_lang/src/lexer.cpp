#include <format>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "lexer.hpp"
#include "token.hpp"
#include "types.hpp"
#include "util.hpp"

namespace ds_lang {

void Lexer::compute_line_col_at(std::string_view code, usize pos, int& line, int& col)
{
    // absolute 0-based (line,col)
    line = 0;
    col = 0;
    for (usize i = 0; i < pos; ++i) {
        if (is_newline(code[i])) {
            ++line;
            col = 0;
        } else {
            ++col;
        }
    }
}

TokenKind Lexer::determine_token_kind(std::string_view lexeme, int line, int column) const
{
    if (lexeme.empty()) {
        throw std::runtime_error("Cannot build token out of empty lexeme");
    }

    if (lexeme == "LET")   return TokenKind::KWLet;
    if (lexeme == "PRINT") return TokenKind::KWPrint;
    if (lexeme == "=")     return TokenKind::OpEqual;

    if (char_is_digit(lexeme[0])) {
        auto res = string_to_i64(lexeme);
        if (!res) {
            const StringToIntError err = res.error();
            throw std::runtime_error(std::format(
                "Failed to convert lexeme {} into an integer: {} [{}] (line={},column={})",
                lexeme, explain(err), to_string(err), line, column));
        }
        return TokenKind::Integer;
    }

    if (!is_valid_identifier(lexeme)) {
        throw std::runtime_error(std::format(
            "The lexeme {} is not a valid identifier! (line={},column={})",
            lexeme, line, column));
    }

    return TokenKind::Identifier;
}

std::vector<Token> Lexer::tokenize_all() const
{
    return tokenize_range(0, code_.size());
}

std::vector<Token> Lexer::tokenize_range(usize left, usize right) const
{
    if (left > right || right > code_.size()) {
        throw std::runtime_error(std::format(
            "tokenize_range: invalid range [{}, {}) for code size {}",
            left, right, code_.size()));
    }

    int line = 0;
    int col = 0;
    compute_line_col_at(code_, left, line, col);

    usize pos = left;
    std::vector<Token> out;

    auto new_char = [&] {
        ++pos;
        ++col;
    };
    auto new_line = [&] {
        ++pos;
        ++line;
        col = 0;
    };

    while (true) {
        if (pos >= right) {
            out.emplace_back(TokenKind::Eof, std::string_view{}, line, col);
            break;
        }

        while (pos < right && is_hspace(code_[pos])) {
            new_char();
        }
        if (pos >= right) {
            out.emplace_back(TokenKind::Eof, std::string_view{}, line, col);
            break;
        }

        if (is_newline(code_[pos])) {
            out.emplace_back(TokenKind::Newline, code_.substr(pos, 1), line, col);
            new_line();
            continue;
        }

        const usize start = pos;
        const int tok_line = line;
        const int tok_col = col;

        while (pos < right && !is_hspace(code_[pos]) && !is_newline(code_[pos])) {
            new_char();
        }

        const std::string_view lex = code_.substr(start, pos - start);
        out.emplace_back(
            determine_token_kind(lex, tok_line, tok_col),
            lex,
            tok_line,
            tok_col
        );
    }
    assert(pos >= right);

    return out;
}

} // namespace ds_lang