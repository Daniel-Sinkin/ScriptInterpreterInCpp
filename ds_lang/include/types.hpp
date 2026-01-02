// ds_lang/include/types.hpp
#pragma once

#include <cstdint>
#include <cassert>
#include <cstddef>

namespace ds_lang
{
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using usize = std::size_t;
using isize = std::ptrdiff_t;

#if defined(__cpp_lib_stdfloat)
using f32 = std::float32_t;
using f64 = std::float64_t;
#else
using f32 = float;
using f64 = double;
#endif
} // namespace ds_lang