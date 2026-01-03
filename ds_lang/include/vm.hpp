// ds_lang/include/vm.hpp
#pragma once

#include <cstddef>
#include <optional>
#include <variant>
#include <vector>

#include "types.hpp"

namespace ds_lang {

struct BytecodePushI64 {
    i64 value;
};

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

struct BytecodeLoadLocal {
    u32 slot;
};

struct BytecodeStoreLocal {
    u32 slot;
};

struct BytecodeJmp {
    u32 target_ip;
};

struct BytecodeJmpFalse {
    u32 target_ip;
};

struct BytecodeJmpTrue {
    u32 target_ip;
};

// Call with no args (argc=0)
struct BytecodeCall {
    u32 func_id;
};

// Call with argc args taken from stack, passed into callee locals[0..argc-1].
struct BytecodeCallArgs {
    u32 func_id;
    u32 argc;
};

struct BytecodeReturn {};

struct BytecodePrint {};

using BytecodeOperation = std::variant<
    BytecodePushI64,
    BytecodeAdd,
    BytecodeSub,
    BytecodeMult,
    BytecodeDiv,
    BytecodeMod,
    BytecodeEQ,
    BytecodeNEQ,
    BytecodeLT,
    BytecodeLE,
    BytecodeGT,
    BytecodeGE,
    BytecodeNEG,
    BytecodeNOT,
    BytecodePop,
    BytecodeLoadLocal,
    BytecodeStoreLocal,
    BytecodeJmp,
    BytecodeJmpFalse,
    BytecodeJmpTrue,
    BytecodeCall,
    BytecodeCallArgs,
    BytecodeReturn,
    BytecodePrint>;

struct FunctionBytecode {
    std::vector<BytecodeOperation> code;
    u32 num_locals{0};
    u32 num_params{0};
};

class VirtualMachine {
public:
    explicit VirtualMachine(bool immediate_print = true) noexcept
        : immediate_print_{immediate_print} {}

    void clear();

    // Adds a function and returns its function id.
    [[nodiscard]] u32 add_function(FunctionBytecode fn);

    void set_entry_function(u32 func_id);

    // Resets the VM into a runnable state at entry function.
    void reset();

    [[nodiscard]] bool is_active() const noexcept { return is_active_; }

    // Executes one instruction of the current frame.
    void step();

    // Runs until the VM halts (normal return from entry) or throws.
    void run();

    [[nodiscard]] const std::vector<i64>& stack() const noexcept { return stack_; }
    [[nodiscard]] const std::vector<i64>& print_buffer() const noexcept { return print_buffer_; }

private:
    struct Frame {
        u32 func_id{0};
        usize ip{0};
        std::vector<i64> locals{};
    };

    std::vector<i64> stack_{};
    std::vector<i64> print_buffer_{};
    std::vector<FunctionBytecode> functions_{};

    std::optional<u32> entry_func_{std::nullopt};
    std::vector<Frame> call_stack_{};

    bool immediate_print_{true};
    bool is_active_{false};

    [[nodiscard]] const FunctionBytecode& current_function() const;
    [[nodiscard]] FunctionBytecode& current_function();
    [[nodiscard]] Frame& current_frame();
    [[nodiscard]] const Frame& current_frame() const;

    [[nodiscard]] i64 pop();
    void push(i64 v);

    [[nodiscard]] static bool truthy(i64 v) noexcept { return v != 0; }

    void do_call(u32 func_id, u32 argc);
    void do_return();

    void exec_op(const BytecodeOperation& op);
};

} // namespace ds_lang