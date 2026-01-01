// include/token.hpp
#pragma once

#include <algorithm>
#include <cassert>
#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace ds_lang
{
enum class BinaryOperator
{
    Plus,             // +
    Minus,            // -
    Star,             // *
    Slash,            // /
    GreaterThan,      // >
    LessThan,         // <
    GreaterEqualThan, // >=
    LessEqualThan,    // <=
    DoubleEqual,      // ==
};
enum class TokenOperator
{
    Equal, // =
};

struct TokenIdentifier
{
    std::string name;
};

struct TokenInteger
{
    int64_t value;
};

enum class TokenKeyword
{
    Let,
    If,
    Then,
    Else,
    Print
};

using Token = std::variant<BinaryOperator, TokenOperator, TokenIdentifier, TokenInteger, TokenKeyword>;
using Tokens = std::vector<Token>;
} // namespace ds_lang