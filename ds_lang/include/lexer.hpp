// include/lexer.hpp
#pragma once

#include <string_view>

#include "token.hpp"

namespace ds_lang
{

enum class TokenizeWordError
{
    Empty,
    InvalidIntegerDigit,
    IntegerOverflow,
    StartsWithZero,
    IdentifierIsNotAscii
};

constexpr std::string_view to_string(TokenizeWordError e) noexcept
{
    switch (e)
    {
    case TokenizeWordError::Empty:
        return "Empty";
    case TokenizeWordError::InvalidIntegerDigit:
        return "InvalidIntegerDigit";
    case TokenizeWordError::IntegerOverflow:
        return "IntegerOverflow";
    case TokenizeWordError::StartsWithZero:
        return "StartsWithZero";
    case TokenizeWordError::IdentifierIsNotAscii:
        return "IdentifierIsNotAscii";
    }
    return "UnknownParseTokenError";
}

constexpr std::string_view explain(TokenizeWordError e) noexcept
{
    switch (e)
    {
    case TokenizeWordError::Empty:
        return "Encountered an empty token where input was expected.";
    case TokenizeWordError::InvalidIntegerDigit:
        return "Integer literal contains a non-digit character.";
    case TokenizeWordError::IntegerOverflow:
        return "Integer literal is too large to fit into a 64-bit signed integer.";
    case TokenizeWordError::StartsWithZero:
        return "Integer literal has a leading zero, which is not allowed.";
    case TokenizeWordError::IdentifierIsNotAscii:
        return "Identifier contains non-ASCII alphabetic characters.";
    }
    return "Unknown token parsing error.";
}

[[nodiscard]] std::expected<Token, TokenizeWordError>
tokenize_word(std::string_view word);

struct TokenizeNextStatementError
{
    TokenizeWordError tokenize_error;
    size_t word_idx;
    size_t word_length;
};
[[nodiscard]] std::expected<Tokens, TokenizeNextStatementError>
tokenize_next_statement(std::string_view code, size_t &idx);

std::vector<Tokens> tokenize_code(std::string_view code);
} // namespace ds_lang