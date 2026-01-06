#include <format>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "lexer.hpp"
#include "token.hpp"
#include "types.hpp"
#include "util.hpp"

namespace ds_lang {

void Lexer::compute_line_col_at(std::string_view code, usize pos, int &line, int &col) {
    line = 0;
    col = 0;
    for (usize i = 0; i < pos; ++i) {
        if (code[i] == '\n') {
            ++line;
            col = 0;
        } else {
            ++col;
        }
    }
}

static TokenKind keyword_or_identifier(std::string_view s) {
    // clang-format off
    if (s == "int")    return TokenKind::KWInt;
    if (s == "print")  return TokenKind::KWPrint;
    if (s == "func")   return TokenKind::KWFunc;
    if (s == "struct") return TokenKind::KWStruct;
    if (s == "return") return TokenKind::KWReturn;
    if (s == "if")     return TokenKind::KWIf;
    if (s == "else")   return TokenKind::KWElse;
    if (s == "while")  return TokenKind::KWWhile;
    if (s == "true")   return TokenKind::KWTrue;
    if (s == "false")  return TokenKind::KWFalse;
    // clang-format on
    return TokenKind::Identifier;
}

TokenKind Lexer::determine_token_kind(std::string_view lexeme, int line, int column) const {
    if (lexeme.empty()) {
        throw std::runtime_error("Cannot build token out of empty lexeme");
    }

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
    return keyword_or_identifier(lexeme);
}

std::vector<Token> Lexer::tokenize_all() const {
    return tokenize_range(0, code_.size());
}

std::vector<Token> Lexer::tokenize_range(usize left, usize right) const {
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

    auto new_char = [&] -> void {
        ++pos;
        ++col;
    };
    auto new_line = [&] -> void {
        ++pos;
        ++line;
        col = 0;
    };

    auto emit = [&](TokenKind kind, usize start, usize len, int tok_line, int tok_col) {
        out.emplace_back(kind, code_.substr(start, len), tok_line, tok_col);
    };

    while (true) {
        if (pos >= right) {
            out.emplace_back(TokenKind::Eof, std::string_view{}, line, col);
            break;
        }

        while (pos < right) {
            const char c = code_[pos];
            if (c == '\n') {
                new_line();
                continue;
            }
            if (c == '\r') { // \r\n is windows specific newline shenanigans
                if (pos + 1 < right && code_[pos + 1] == '\n') {
                    ++pos;
                    new_line();
                } else {
                    new_char();
                }
                continue;
            }
            if (is_whitespace(c)) {
                new_char();
                continue;
            }
            break;
        }
        if (pos >= right) {
            out.emplace_back(TokenKind::Eof, std::string_view{}, line, col);
            break;
        }

        if (is_eos(code_[pos])) {
            out.emplace_back(TokenKind::Eos, code_.substr(pos, 1), line, col);
            new_char();
            continue;
        }

        const usize start = pos;
        const int tok_line = line;
        const int tok_col = col;

        if(code_[pos] == '"') {
            new_char();
            const usize content_start = pos;
            while(true) {
                if(pos >= right) {
                    throw std::runtime_error(std::format(
                        "Unterminated string literal (line={},column={})",
                        tok_line, tok_col));
                }
                const char c = code_[pos];
                if(c == '\n' || c == '\r') {
                    throw std::runtime_error(std::format(
                        "Unterminated string literal before newline (line={},column={})",
                        tok_line, tok_col));
                }
                if (c == '"') {
                    break;
                }
                if (is_eos(c)) { // TODO: Maybe allow ; in strings, not sure yet
                    throw std::runtime_error(std::format(
                        "Unterminated string literal before ';' (line={},column={})",
                        tok_line, tok_col));
                }
                new_char();
            }
            const usize content_len = pos - content_start;
            emit(TokenKind::String, content_start, content_len, tok_line, tok_col);
            new_char();
            continue;
        }

        if(pos + 2 < right) {
            const char a = code_[pos];
            const char b = code_[pos + 1];
            const char c = code_[pos + 2];

            if(a == 'a' && b == 'n' && c == 'd') {
                emit(TokenKind::OpAnd, start, 3, tok_line, tok_col);
                new_char();
                new_char();
                new_char();
                continue;
            }
        }

        if (pos + 1 < right) {
            const char a = code_[pos];
            const char b = code_[pos + 1];

            if (a == '=' && b == '=') {
                emit(TokenKind::OpEqEq, start, 2, tok_line, tok_col);
                new_char();
                new_char();
                continue;
            }
            if (a == '!' && b == '=') {
                emit(TokenKind::OpNeq, start, 2, tok_line, tok_col);
                new_char();
                new_char();
                continue;
            }
            if (a == '<' && b == '=') {
                emit(TokenKind::OpLe, start, 2, tok_line, tok_col);
                new_char();
                new_char();
                continue;
            }
            if (a == '>' && b == '=') {
                emit(TokenKind::OpGe, start, 2, tok_line, tok_col);
                new_char();
                new_char();
                continue;
            }
            if (a == 'o' && b == 'r') {
                emit(TokenKind::OpOr, start, 2, tok_line, tok_col);
                new_char();
                new_char();
                continue;
            }
        }

        switch (code_[pos]) {
        case '(':
            emit(TokenKind::LParen, start, 1, tok_line, tok_col);
            new_char();
            continue;
        case ')':
            emit(TokenKind::RParen, start, 1, tok_line, tok_col);
            new_char();
            continue;
        case '{':
            emit(TokenKind::LBrace, start, 1, tok_line, tok_col);
            new_char();
            continue;
        case '}':
            emit(TokenKind::RBrace, start, 1, tok_line, tok_col);
            new_char();
            continue;
        case '[':
            emit(TokenKind::LBracket, start, 1, tok_line, tok_col);
            new_char();
            continue;
        case ']':
            emit(TokenKind::RBracket, start, 1, tok_line, tok_col);
            new_char();
            continue;
        case ',':
            emit(TokenKind::Comma, start, 1, tok_line, tok_col);
            new_char();
            continue;

        case '=':
            emit(TokenKind::OpAssign, start, 1, tok_line, tok_col);
            new_char();
            continue;
        case '+':
            emit(TokenKind::OpPlus, start, 1, tok_line, tok_col);
            new_char();
            continue;
        case '-':
            emit(TokenKind::OpMinus, start, 1, tok_line, tok_col);
            new_char();
            continue;
        case '*':
            emit(TokenKind::OpStar, start, 1, tok_line, tok_col);
            new_char();
            continue;
        case '/':
            emit(TokenKind::OpSlash, start, 1, tok_line, tok_col);
            new_char();
            continue;
        case '%':
            emit(TokenKind::OpPercent, start, 1, tok_line, tok_col);
            new_char();
            continue;

        case '!':
            emit(TokenKind::OpBang, start, 1, tok_line, tok_col);
            new_char();
            continue;
        case '.':
            emit(TokenKind::OpPeriod, start, 1, tok_line, tok_col);
            new_char();
            continue;
        case '<':
            emit(TokenKind::OpLt, start, 1, tok_line, tok_col);
            new_char();
            continue;
        case '>':
            emit(TokenKind::OpGt, start, 1, tok_line, tok_col);
            new_char();
            continue;
        default:
            break;
        }

        if (char_is_digit(code_[pos])) {
            while (pos < right && char_is_digit(code_[pos])) {
                new_char();
            }
            const std::string_view lex = code_.substr(start, pos - start);
            emit(TokenKind::Integer, start, pos - start, tok_line, tok_col);
            (void)determine_token_kind(lex, tok_line, tok_col);
            continue;
        }

        if (char_is_valid_for_identifier(code_[pos])) {
            new_char();
            while (pos < right && (char_is_valid_for_identifier(code_[pos]) || char_is_digit(code_[pos]))) {
                new_char();
            }
            const std::string_view lex = code_.substr(start, pos - start);

            if (!is_valid_identifier(lex)) {
                throw std::runtime_error(std::format(
                    "Invalid identifier {} (line={},column={})",
                    lex, tok_line, tok_col));
            }

            emit(keyword_or_identifier(lex), start, pos - start, tok_line, tok_col);
            continue;
        }

        throw std::runtime_error(std::format(
            "Unexpected character {:?} (line={},column={})",
            code_.substr(pos, 1), tok_line, tok_col));
    }

    return out;
}

} // namespace ds_lang