// ds_lang/src/util.cpp
#include <format>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "util.hpp"

namespace ds_lang {
std::string load_code(const std::string &path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error(std::format("Failed to open '{}'", path));
    }

    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

bool is_valid_identifier(std::string_view s) noexcept {
    if (s.empty())
        return false;
    if (!char_is_valid_for_identifier(s[0]))
        return false;

    for (usize i = 1; i < s.size(); ++i) {
        const char c = s[i];
        if (!(char_is_valid_for_identifier(c) || char_is_digit(c)))
            return false;
    }
    return true;
}

[[nodiscard]] std::expected<i64, StringToIntError>
string_to_i64(std::string_view word) noexcept {
    using E = StringToIntError;

    if (word.empty())
        return std::unexpected{E::Empty};

    int sign = 1;
    usize i = 0;

    if (word[0] == '-') {
        sign = -1;
        i = 1;
    } else if (word[0] == '+') {
        // tests expect "+1" to be invalid
        return std::unexpected{E::InvalidDigit};
    }

    if (i >= word.size()) {
        // just "-" (no digits)
        return std::unexpected{E::InvalidDigit};
    }

    if (word[i] == '0' && (i + 1) < word.size()) {
        return std::unexpected{E::StartsWithZero};
    }

    const u64 pos_limit = static_cast<u64>(std::numeric_limits<i64>::max());
    const u64 neg_limit = pos_limit + 1ULL; // 9223372036854775808
    const u64 limit = (sign < 0) ? neg_limit : pos_limit;

    u64 acc = 0;
    while(i < word.size()) {
        const char c = word[i];
        if (!char_is_digit(c))
            return std::unexpected{E::InvalidDigit};

        const u64 d = static_cast<u64>(c - '0');

        // overflow check: acc*10 + d <= limit
        if (acc > (limit - d) / 10ULL)
            return std::unexpected{E::Overflow};

        acc = acc * 10ULL + d;
        ++i;
    }

    if (sign > 0) {
        return static_cast<i64>(acc);
    }
    if (acc == neg_limit) {
        return std::numeric_limits<i64>::min();
    }
    return -static_cast<i64>(acc);
}
} // namespace ds_lang