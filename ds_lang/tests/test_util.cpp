// tests/test_util.cpp
#include "common.hpp"

#include <print>
#include <limits>
#include <string_view>

#include "types.hpp"
#include "util.hpp"

namespace ds_lang::Test
{
static void expect_ok(std::string_view s, i64 expected)
{
    auto r = ds_lang::string_to_i64(s);
    EXPECT_TRUE(r.has_value());
    EXPECT_EQ(*r, expected);
}

static void expect_err(std::string_view s, ds_lang::StringToIntError expected)
{
    auto r = ds_lang::string_to_i64(s);
    EXPECT_TRUE(!r.has_value());
    EXPECT_EQ(r.error(), expected);
}

static void test_empty()
{
    expect_err("", ds_lang::StringToIntError::Empty);
}

static void test_single_zero_ok()
{
    // Only fails if you disallow "0" entirely; most implementations accept it.
    expect_ok("0", 0);
}

static void test_starts_with_zero()
{
    // Assumes your implementation forbids leading zeros for multi-digit numbers.
    expect_err("01", ds_lang::StringToIntError::StartsWithZero);
    expect_err("000", ds_lang::StringToIntError::StartsWithZero);
}

static void test_invalid_digit()
{
    expect_err("1a", ds_lang::StringToIntError::InvalidDigit);
    expect_err("a1", ds_lang::StringToIntError::InvalidDigit);
    expect_err("-", ds_lang::StringToIntError::InvalidDigit);
    expect_err("+1", ds_lang::StringToIntError::InvalidDigit);
    expect_err("12 3", ds_lang::StringToIntError::InvalidDigit);
}

static void test_ok_small_values()
{
    expect_ok("7", 7);
    expect_ok("42", 42);
    expect_ok("123456", 123456);
}

static void test_int64_max_ok()
{
    // i64 is std::int64_t per your types.hpp
    constexpr auto max_str = "9223372036854775807";
    expect_ok(max_str, std::numeric_limits<ds_lang::i64>::max());
}

static void test_overflow()
{
    // One above INT64_MAX
    constexpr auto overflow_str = "9223372036854775808";
    expect_err(overflow_str, ds_lang::StringToIntError::Overflow);

    // Obviously too large
    expect_err("999999999999999999999999999999999999", ds_lang::StringToIntError::Overflow);
}

} // namespace ds_lang::Test

int main()
{
    using namespace ds_lang::Test;

    struct TestCase { const char* name; void (*fn)(); };

    const TestCase tests[] = {
        {"util.string_to_i64.empty", test_empty},
        {"util.string_to_i64.single_zero_ok", test_single_zero_ok},
        {"util.string_to_i64.starts_with_zero", test_starts_with_zero},
        {"util.string_to_i64.invalid_digit", test_invalid_digit},
        {"util.string_to_i64.ok_small_values", test_ok_small_values},
        {"util.string_to_i64.int64_max_ok", test_int64_max_ok},
        {"util.string_to_i64.overflow", test_overflow},
    };

    for (const auto& t : tests) {
        try {
            t.fn();
        } catch (const std::exception& e) {
            std::println("FAIL {}: {}", t.name, e.what());
            return 1;
        } catch (...) {
            std::println("FAIL {}: unknown exception", t.name);
            return 1;
        }
    }

    std::println("OK {}", static_cast<int>(std::size(tests)));
    return 0;
}