// include/lexer.hpp
#pragma once

#include <string_view>
#include <vector>

#include "token.hpp"
#include "types.hpp"

namespace ds_lang {
class Lexer {
  public:
    explicit Lexer(std::string_view code) noexcept // TODO: Need to think of a good way to handle code lifetime
        : code_(code), pos_(0), line_(0), column_(0) {}
    Lexer() = delete;
    Lexer(const Lexer &) = default;
    Lexer &operator=(const Lexer &) = default;

    bool eof() const { 
        assert(is_active_);
        return pos_ >= code_.size();
    }

    TokenKind determine_token_kind(std::string_view lexeme) const;

    [[nodiscard]] std::vector<Token> take_tokens() &&;

  private:
    std::string_view code_;
    usize pos_ = 0zu;
    int line_ = 0;
    int column_ = 0;
    std::vector<Token> tokens_;
    bool is_active_ = true;

    Token process_next_token();

    void new_char() {
        assert(is_active_);
        ++pos_;
        ++column_;
    }

    void new_line() {
        assert(is_active_);
        ++pos_;
        column_ = 0;
        ++line_;
    }

};
} // namespace ds_lang