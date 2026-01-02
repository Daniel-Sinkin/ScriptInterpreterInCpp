// tests/common.hpp
#pragma once

#include <cmath>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <algorithm>

namespace ds_lang::Test
{
inline void expect(bool ok, const char *msg)
{
    if (!ok) throw std::runtime_error(msg);
}

template <class F>
inline void expect_exception(F &&fn, const char *msg)
{
    static_assert(std::is_invocable_v<F>);
    bool thrown = false;
    try
    {
        std::forward<F>(fn)();
    }
    catch (...)
    {
        thrown = true;
    }
    if (!thrown) throw std::runtime_error(msg);
}

template <typename T>
constexpr bool nearly_equal(T a, T b, T rel_eps, T abs_eps) noexcept
{
    static_assert(std::is_floating_point_v<T>);
    const T diff = std::abs(a - b);
    if (diff <= abs_eps) return true;
    return diff <= rel_eps * std::max(std::abs(a), std::abs(b));
}

#define EXPECT_TRUE(expr)                                             \
    do                                                                \
    {                                                                 \
        if (!(expr)) throw std::runtime_error("EXPECT_TRUE: " #expr); \
    } while (0)

#define EXPECT_EQ(a, b)                                                          \
    do                                                                           \
    {                                                                            \
        if (!((a) == (b))) throw std::runtime_error("EXPECT_EQ: " #a " == " #b); \
    } while (0)

#define EXPECT_THROW(expr)                                         \
    do                                                             \
    {                                                              \
        bool _t = false;                                           \
        try                                                        \
        {                                                          \
            (void)(expr);                                          \
        }                                                          \
        catch (...)                                                \
        {                                                          \
            _t = true;                                             \
        }                                                          \
        if (!_t) throw std::runtime_error("EXPECT_THROW: " #expr); \
    } while (0)

#define EXPECT_NO_THROW(expr)                                    \
    do                                                           \
    {                                                            \
        try                                                      \
        {                                                        \
            (void)(expr);                                        \
        }                                                        \
        catch (...)                                              \
        {                                                        \
            throw std::runtime_error("EXPECT_NO_THROW: " #expr); \
        }                                                        \
    } while (0)

} // namespace ds_lang::Test