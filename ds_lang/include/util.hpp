// ds_lang/include/util.hpp
#pragma once

#include <expected>
#include <functional>
#include <string_view>

#include "types.hpp"

namespace ds_lang {
// std::visit overload pattern for multiple lambdas.
// Reference: https://en.cppreference.com/w/cpp/utility/variant/visit2
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

struct ScopeExit {
    std::function<void()> f;
    ~ScopeExit() { f(); }
};

std::string load_code(const std::string &path);

constexpr bool is_whitespace(char c) noexcept
{
    return c == ' ' || c == '\t' || c == '\r' ||
           c == '\f' || c == '\v';
}
constexpr bool is_eos(char c) noexcept {
    return c == ';';
}

constexpr bool char_is_digit(char c) noexcept { return c >= '0' && c <= '9'; }

constexpr bool char_is_valid_for_identifier(char c) noexcept {
    return (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') ||
           c == '_';
}

bool is_valid_identifier(std::string_view s) noexcept;

enum class StringToIntError {
    Empty,
    InvalidDigit,
    Overflow,
    StartsWithZero
};
constexpr std::string_view to_string(StringToIntError e) noexcept {
    switch (e) {
    case StringToIntError::Empty:
        return "Empty";
    case StringToIntError::InvalidDigit:
        return "InvalidDigit";
    case StringToIntError::Overflow:
        return "Overflow";
    case StringToIntError::StartsWithZero:
        return "StartsWithZero";
    }
    return "UnknownStringToIntError";
}

constexpr std::string_view explain(StringToIntError e) noexcept {
    switch (e) {
    case StringToIntError::Empty:
        return "The input string is empty and cannot be converted to an integer.";
    case StringToIntError::InvalidDigit:
        return "The input contains a character that is not a valid decimal digit.";
    case StringToIntError::Overflow:
        return "The numeric value exceeds the representable range of the target integer type.";
    case StringToIntError::StartsWithZero:
        return "The input starts with a leading zero, which is not allowed in this context.";
    }
    return "Unknown string-to-integer conversion error.";
}

[[nodiscard]] std::expected<i64, StringToIntError>
string_to_i64(std::string_view word) noexcept;
} // namespace ds_lang
