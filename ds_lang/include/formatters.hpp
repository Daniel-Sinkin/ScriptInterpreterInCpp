// ds_lang/include/formatters.hpp
#pragma once

#include <format>
#include <string>
#include <string_view>

// Forward declarations: the full definitions must be visible
// before you use ds_lang::Fmt::format_* implementations.
namespace ds_lang {
struct Expression;
struct Statement;
} // namespace ds_lang

namespace ds_lang::Fmt {
// Declarations only in the header:
std::string format_expression(const Expression& e);
std::string format_statement(const Statement& s);
} // namespace ds_lang::Fmt

template <>
struct std::formatter<ds_lang::Expression> : std::formatter<std::string_view> {
    auto format(const ds_lang::Expression& e, format_context& ctx) const {
        const std::string s = ds_lang::Fmt::format_expression(e);
        return std::formatter<std::string_view>::format(s, ctx);
    }
};

template <>
struct std::formatter<ds_lang::Statement> : std::formatter<std::string_view> {
    auto format(const ds_lang::Statement& s, format_context& ctx) const {
        const std::string str = ds_lang::Fmt::format_statement(s);
        return std::formatter<std::string_view>::format(str, ctx);
    }
};