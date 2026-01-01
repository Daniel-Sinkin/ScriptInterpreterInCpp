#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "types.hpp"

namespace ds_lang
{

enum class BinaryOperator
{
    Plus,
    Minus,
    Star,
    Slash,
    GreaterThan,
    LessThan,
    GreaterEqualThan,
    LessEqualThan,
    DoubleEqual
};
enum class TokenOperator
{
    Equal
};

struct TokenIdentifier
{
    std::string name;
};
struct TokenInteger
{
    i64 value;
};

enum class TokenKeyword
{
    Int,
    If,
    Then,
    Else,
    Print
};

using Token = std::variant<BinaryOperator, TokenOperator, TokenIdentifier, TokenInteger, TokenKeyword>;
using Tokens = std::vector<Token>;

constexpr std::string_view to_string(BinaryOperator op) noexcept
{
    switch (op)
    {
    case BinaryOperator::Plus:             return "Plus";
    case BinaryOperator::Minus:            return "Minus";
    case BinaryOperator::Star:             return "Star";
    case BinaryOperator::Slash:            return "Slash";
    case BinaryOperator::GreaterThan:      return "GreaterThan";
    case BinaryOperator::LessThan:         return "LessThan";
    case BinaryOperator::GreaterEqualThan: return "GreaterEqualThan";
    case BinaryOperator::LessEqualThan:    return "LessEqualThan";
    case BinaryOperator::DoubleEqual:      return "DoubleEqual";
    }
    return "UnknownBinaryOperator";
}

constexpr std::string_view to_string(TokenOperator op) noexcept
{
    switch (op)
    {
    case TokenOperator::Equal: return "Equal";
    }
    return "UnknownTokenOperator";
}

constexpr std::string_view to_string(TokenKeyword kw) noexcept
{
    switch (kw)
    {
    case TokenKeyword::Int:   return "Int";
    case TokenKeyword::If:    return "If";
    case TokenKeyword::Then:  return "Then";
    case TokenKeyword::Else:  return "Else";
    case TokenKeyword::Print: return "Print";
    }
    return "UnknownTokenKeyword";
}

std::string token_to_string(const Token &tok);
void print_tokens(const std::vector<Token> &toks);

} // namespace ds_lang