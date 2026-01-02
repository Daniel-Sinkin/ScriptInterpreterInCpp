#pragma once

#include <cassert>
#include <string_view>
#include <vector>

#include "token.hpp"
#include "types.hpp"

namespace ds_lang {

class Lexer {
  public:
    explicit Lexer(std::string_view code) noexcept : code_(code) {}
    Lexer() = delete;

    [[nodiscard]] std::vector<Token> tokenize_all() const;
    [[nodiscard]] std::vector<Token> tokenize_range(usize left, usize right) const; // [left,right)

  private:
    std::string_view code_;

    TokenKind determine_token_kind(std::string_view lexeme, int line, int column) const;

    static void compute_line_col_at(std::string_view code, usize pos, int &line, int &col);
};

} // namespace ds_lang