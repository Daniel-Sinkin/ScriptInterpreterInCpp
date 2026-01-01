// ds_lang/src/util.cpp
#include <algorithm>
#include <format>
#include <fstream>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>

#include "util.hpp"

namespace ds_lang
{
std::string load_code(const std::string &path)
{
    std::ifstream in(path);
    if (!in)
    {
        throw std::runtime_error(std::format("Failed to open '{}'", path));
    }

    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

bool is_valid_identifier(std::string_view s) noexcept
{
    return std::ranges::all_of(s, char_is_valid_for_identifier);
}

[[nodiscard]] std::expected<i64, StringToIntError>
string_to_i64(std::string_view word) noexcept
{
    using E = StringToIntError;
    if (word.empty()) return std::unexpected{E::Empty};
    if (word.length() > 1 && word[0] == '0') return std::unexpected{E::StartsWithZero};
    i64 retval = 0;
    bool is_negative = false;
    for (size_t i = 0; i < word.size(); ++i)
    {
        char c = word[i];
        if (i == 0 && c == '-')
        {
            is_negative = true;
            continue;
        }

        if (!char_is_digit(c))
        {
            return std::unexpected{E::InvalidDigit};
        }
        retval = retval * 10 + static_cast<i64>(c - '0');
        if (i > 18) // if this is false then we are guaranteed to fit in i64, avoid having to do bounds check for now
        {
            return std::unexpected{E::Overflow};
        }
    }
    return {is_negative ? -retval : retval};
}
} // namespace ds_lang