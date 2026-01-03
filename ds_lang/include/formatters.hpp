// ds_lang/include/formatters.hpp
#pragma once

#include <format>
#include <string>
#include <string_view>
#include <variant>

namespace ds_lang {
struct Expression;
struct Statement;

struct FunctionBytecode;
using BytecodeOperation = std::variant<
    struct BytecodePushI64,
    struct BytecodeAdd,
    struct BytecodeSub,
    struct BytecodeMult,
    struct BytecodeDiv,
    struct BytecodeMod,
    struct BytecodeEQ,
    struct BytecodeNEQ,
    struct BytecodeLT,
    struct BytecodeLE,
    struct BytecodeGT,
    struct BytecodeGE,
    struct BytecodeNEG,
    struct BytecodeNOT,
    struct BytecodePop,
    struct BytecodeLoadLocal,
    struct BytecodeStoreLocal,
    struct BytecodeJmp,
    struct BytecodeJmpFalse,
    struct BytecodeJmpTrue,
    struct BytecodeCall,
    struct BytecodeCallArgs,
    struct BytecodeReturn,
    struct BytecodePrint>;
} // namespace ds_lang

namespace ds_lang::Fmt {
// Existing:
std::string format_expression(const Expression& e);
std::string format_statement(const Statement& s);

// New:
std::string format_bytecode_operation(const BytecodeOperation& op);
std::string format_function_bytecode(const FunctionBytecode& fn);
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

template <>
struct std::formatter<ds_lang::BytecodeOperation> : std::formatter<std::string_view> {
    auto format(const ds_lang::BytecodeOperation& op, format_context& ctx) const {
        const std::string s = ds_lang::Fmt::format_bytecode_operation(op);
        return std::formatter<std::string_view>::format(s, ctx);
    }
};

template <>
struct std::formatter<ds_lang::FunctionBytecode> : std::formatter<std::string_view> {
    auto format(const ds_lang::FunctionBytecode& fn, format_context& ctx) const {
        const std::string s = ds_lang::Fmt::format_function_bytecode(fn);
        return std::formatter<std::string_view>::format(s, ctx);
    }
};