// ds_lang/include/bytecode.hpp
#pragma once

#include <string>
#include <variant>
#include <vector>

#include "types.hpp"

namespace ds_lang {

struct BytecodePushI64 { i64 value; };

struct BytecodeAdd {};
struct BytecodeSub {};
struct BytecodeMult {};
struct BytecodeDiv {};
struct BytecodeMod {};

struct BytecodeEQ {};
struct BytecodeNEQ {};
struct BytecodeLT {};
struct BytecodeLE {};
struct BytecodeGT {};
struct BytecodeGE {};

struct BytecodeNEG {};
struct BytecodeNOT {};

struct BytecodePop {};

struct BytecodeLoadLocal { u32 slot; };
struct BytecodeStoreLocal { u32 slot; };

struct BytecodeJmp { IPtr target_ip{INVALIDIPtr}; };
struct BytecodeJmpFalse { IPtr target_ip{INVALIDIPtr}; };
struct BytecodeJmpTrue { IPtr target_ip{INVALIDIPtr}; };

struct BytecodeCall { u32 func_id; };
struct BytecodeCallArgs { u32 func_id; u32 argc; };

struct BytecodeReturn {};
struct BytecodePrint {};
struct BytecodePrintString { std::string content; };

using BytecodeOperation = std::variant<
    BytecodePushI64,
    BytecodeAdd, BytecodeSub, BytecodeMult, BytecodeDiv, BytecodeMod,
    BytecodeEQ, BytecodeNEQ, BytecodeLT, BytecodeLE, BytecodeGT, BytecodeGE,
    BytecodeNEG, BytecodeNOT,
    BytecodePop,
    BytecodeLoadLocal, BytecodeStoreLocal,
    BytecodeJmp, BytecodeJmpFalse, BytecodeJmpTrue,
    BytecodeCall, BytecodeCallArgs,
    BytecodeReturn,
    BytecodePrint, BytecodePrintString>;

struct FunctionBytecode {
    std::vector<BytecodeOperation> code{};
    std::vector<std::string> seen_symbols{};
    u32 num_locals{0};
    u32 num_params{0};
};

} // namespace ds_lang