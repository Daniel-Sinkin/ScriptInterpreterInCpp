// ds_lang/include/util.hpp
#pragma once

#include <expected>
#include <string_view>

#include "types.hpp"

namespace ds_lang
{
std::string load_code(const std::string& path);

constexpr bool char_is_whitespace(char c) noexcept
{
    return c == ' ' || c == '\t' ||
           c == '\n' || c == '\r' ||
           c == '\f' || c == '\v';
}
constexpr bool char_is_digit(char c) noexcept { return c >= '0' && c <= '9'; }

constexpr bool char_is_valid_for_identifier(char c) noexcept
{
    return (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') ||
           c == '_';
}

bool is_valid_identifier(std::string_view s) noexcept;

enum class StringToIntError
{
    Empty,
    InvalidDigit,
    Overflow,
    StartsWithZero
};
[[nodiscard]] std::expected<i64, StringToIntError>
string_to_i64(std::string_view word) noexcept;
} // namespace ds_lang