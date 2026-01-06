// ds_lang/include/vm.hpp
#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "bytecode.hpp"
#include "types.hpp"

namespace ds_lang {


class VirtualMachine {
public:
    explicit VirtualMachine(bool immediate_print = true) noexcept
        : immediate_print_{immediate_print} {}

    void clear();

    [[nodiscard]] u32 add_function(FunctionBytecode fn);

    void set_entry_function(u32 func_id);

    void reset();

    [[nodiscard]] bool is_active() const noexcept { return is_active_; }

    void step();

    void run();

    [[nodiscard]] const std::vector<i64> &stack() const noexcept { return stack_; }
    [[nodiscard]] const std::vector<std::string> &print_buffer() const noexcept { return print_buffer_; }

    i64 get_return_value() {
        return *return_value_;
    }

private:
    struct Frame {
        u32 func_id{0};
        IPtr ip{0};
        std::vector<i64> locals{};
    };

    std::vector<i64> stack_{};
    std::vector<std::string> print_buffer_{};
    std::vector<FunctionBytecode> functions_{};

    std::optional<usize> entry_func_{std::nullopt};
    std::vector<Frame> call_stack_{};

    bool immediate_print_{true};
    bool is_active_{true};

    std::unique_ptr<i64> return_value_{nullptr};

    [[nodiscard]] const FunctionBytecode &current_function() const;
    [[nodiscard]] FunctionBytecode &current_function();
    [[nodiscard]] Frame &current_frame();
    [[nodiscard]] const Frame &current_frame() const;

    [[nodiscard]] i64 pop();
    void push(i64 v);

    [[nodiscard]] static bool truthy(i64 v) noexcept { return v != 0; }

    void do_call(u32 func_id, u32 argc);
    void do_return();

    void exec_op(const BytecodeOperation &op);
};

} // namespace ds_lang