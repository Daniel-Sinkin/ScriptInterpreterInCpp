// ds_lang/src/vm.cpp
#include "vm.hpp"

#include <cassert>
#include <print>
#include <stdexcept>
#include <utility>

#include "util.hpp"

namespace ds_lang {

void VirtualMachine::clear() {
    stack_.clear();
    print_buffer_.clear();
    functions_.clear();
    entry_func_.reset();
    call_stack_.clear();
    is_active_ = false;
}

u32 VirtualMachine::add_function(FunctionBytecode fn) {
    const u32 id = static_cast<u32>(functions_.size());
    functions_.push_back(std::move(fn));
    return id;
}

void VirtualMachine::set_entry_function(u32 func_id) {
    if (static_cast<usize>(func_id) >= functions_.size()) {
        throw std::runtime_error("set_entry_function: invalid func_id");
    }
    entry_func_ = func_id;
}

void VirtualMachine::reset() {
    if (!entry_func_) {
        throw std::runtime_error("reset: entry function not set");
    }
    const u32 fid = *entry_func_;
    const FunctionBytecode& f = functions_[static_cast<usize>(fid)];

    stack_.clear();
    print_buffer_.clear();
    call_stack_.clear();

    Frame entry;
    entry.func_id = fid;
    entry.ip = 0;
    entry.locals.assign(static_cast<usize>(f.num_locals), 0);

    call_stack_.push_back(std::move(entry));
    is_active_ = true;

    // If entry has no code, halt immediately.
    if (f.code.empty()) {
        is_active_ = false;
    }
}

const FunctionBytecode& VirtualMachine::current_function() const {
    if (call_stack_.empty()) {
        throw std::runtime_error("VM: no current frame");
    }
    const u32 fid = call_stack_.back().func_id;
    return functions_.at(static_cast<usize>(fid));
}

FunctionBytecode& VirtualMachine::current_function() {
    if (call_stack_.empty()) {
        throw std::runtime_error("VM: no current frame");
    }
    const u32 fid = call_stack_.back().func_id;
    return functions_.at(static_cast<usize>(fid));
}

VirtualMachine::Frame& VirtualMachine::current_frame() {
    if (call_stack_.empty()) {
        throw std::runtime_error("VM: no current frame");
    }
    return call_stack_.back();
}

const VirtualMachine::Frame& VirtualMachine::current_frame() const {
    if (call_stack_.empty()) {
        throw std::runtime_error("VM: no current frame");
    }
    return call_stack_.back();
}

i64 VirtualMachine::pop() {
    if (stack_.empty()) {
        throw std::runtime_error("VM stack underflow");
    }
    const i64 v = stack_.back();
    stack_.pop_back();
    return v;
}

void VirtualMachine::push(i64 v) {
    stack_.push_back(v);
}

void VirtualMachine::do_call(u32 func_id, u32 argc) {
    if (static_cast<usize>(func_id) >= functions_.size()) {
        throw std::runtime_error("VM call: invalid function id");
    }

    const FunctionBytecode& callee = functions_[static_cast<usize>(func_id)];
    if (argc != callee.num_params) {
        throw std::runtime_error("VM call: argc does not match callee.num_params");
    }
    if (callee.num_locals < callee.num_params) {
        throw std::runtime_error("VM call: num_locals must be >= num_params");
    }
    if (stack_.size() < static_cast<usize>(argc)) {
        throw std::runtime_error("VM call: not enough values on stack for args");
    }

    std::vector<i64> args(static_cast<usize>(argc));
    for (usize i = 0; i < static_cast<usize>(argc); ++i) {
        // Pop in reverse; last pushed argument is on top
        args[static_cast<usize>(argc) - 1 - i] = pop();
    }

    Frame fr;
    fr.func_id = func_id;
    fr.ip = 0;
    fr.locals.assign(static_cast<usize>(callee.num_locals), 0);

    for (usize i = 0; i < static_cast<usize>(argc); ++i) {
        fr.locals[i] = args[i];
    }

    call_stack_.push_back(std::move(fr));

    // If callee is empty, just treat as returning immediately.
    if (callee.code.empty()) {
        do_return();
    }
}

void VirtualMachine::do_return() {
    // Return value convention: must have value on stack to return.
    // (Keeps the VM simple and tests deterministic.)
    if (stack_.empty()) {
        throw std::runtime_error("VM return: missing return value on stack");
    }
    i64 ret = pop();
    return_value_ = std::make_unique<i64>(ret);

    if (call_stack_.empty()) {
        throw std::runtime_error("VM return: call stack empty");
    }

    call_stack_.pop_back();

    if (call_stack_.empty()) {
        // Returned from entry => halt. Ret is discarded (or could be stored later).
        is_active_ = false;
        return;
    }

    // Push return value to caller stack.
    push(ret);
}

void VirtualMachine::exec_op(const BytecodeOperation& op) {
    auto& fr = current_frame();

    std::visit(
        ds_lang::overloaded{
            [&](const BytecodePushI64& o) {
                push(o.value);
            },
            [&](const BytecodeAdd&) {
                const i64 rhs = pop();
                const i64 lhs = pop();
                push(lhs + rhs);
            },
            [&](const BytecodeSub&) {
                const i64 rhs = pop();
                const i64 lhs = pop();
                push(lhs - rhs);
            },
            [&](const BytecodeMult&) {
                const i64 rhs = pop();
                const i64 lhs = pop();
                push(lhs * rhs);
            },
            [&](const BytecodeDiv&) {
                const i64 rhs = pop();
                const i64 lhs = pop();
                if (rhs == 0) {
                    throw std::runtime_error("VM: division by zero");
                }
                push(lhs / rhs);
            },
            [&](const BytecodeMod&) {
                const i64 rhs = pop();
                const i64 lhs = pop();
                if (rhs == 0) {
                    throw std::runtime_error("VM: modulo by zero");
                }
                push(lhs % rhs);
            },

            [&](const BytecodeEQ&) {
                const i64 rhs = pop();
                const i64 lhs = pop();
                push((lhs == rhs) ? 1 : 0);
            },
            [&](const BytecodeNEQ&) {
                const i64 rhs = pop();
                const i64 lhs = pop();
                push((lhs != rhs) ? 1 : 0);
            },
            [&](const BytecodeLT&) {
                const i64 rhs = pop();
                const i64 lhs = pop();
                push((lhs < rhs) ? 1 : 0);
            },
            [&](const BytecodeLE&) {
                const i64 rhs = pop();
                const i64 lhs = pop();
                push((lhs <= rhs) ? 1 : 0);
            },
            [&](const BytecodeGT&) {
                const i64 rhs = pop();
                const i64 lhs = pop();
                push((lhs > rhs) ? 1 : 0);
            },
            [&](const BytecodeGE&) {
                const i64 rhs = pop();
                const i64 lhs = pop();
                push((lhs >= rhs) ? 1 : 0);
            },

            [&](const BytecodeNEG&) { push(-pop()); },
            [&](const BytecodeNOT&) { push(truthy(pop()) ? 0 : 1); },

            [&](const BytecodePop&) { (void)pop(); },

            [&](const BytecodeLoadLocal& o) {
                if (static_cast<usize>(o.slot) >= fr.locals.size()) {
                    throw std::runtime_error("VM: LOAD_LOCAL out of range");
                }
                push(fr.locals[static_cast<usize>(o.slot)]);
            },
            [&](const BytecodeStoreLocal& o) {
                if (static_cast<usize>(o.slot) >= fr.locals.size()) {
                    throw std::runtime_error("VM: STORE_LOCAL out of range");
                }
                fr.locals[static_cast<usize>(o.slot)] = pop();
            },

            [&](const BytecodeJmp& o) { fr.ip = static_cast<usize>(o.target_ip); },
            [&](const BytecodeJmpFalse& o) {
                const i64 cond = pop();
                if (!truthy(cond)) {
                    fr.ip = static_cast<usize>(o.target_ip);
                }
            },
            [&](const BytecodeJmpTrue& o) {
                const i64 cond = pop();
                if (truthy(cond)) {
                    fr.ip = static_cast<usize>(o.target_ip);
                }
            },

            [&](const BytecodeCall& o) { do_call(o.func_id, 0); },
            [&](const BytecodeCallArgs& o) { do_call(o.func_id, o.argc); },

            [&](const BytecodeReturn&) {
                do_return();
            },

            [&](const BytecodePrint&) {
                if (stack_.empty()) {
                    throw std::runtime_error("VM: PRINT requires a value on stack");
                }
                const i64 v = stack_.back();
                print_buffer_.push_back(v);
                if (immediate_print_) {
                    std::println("VM Print: [{}]", v);
                }
            },
        },
        op);
}

void VirtualMachine::step() {
    if (!is_active_) {
        throw std::runtime_error("VM: step() called while inactive");
    }
    if (call_stack_.empty()) {
        throw std::runtime_error("VM: internal error (active but no frames)");
    }

    const FunctionBytecode& f = current_function();
    auto& fr = current_frame();

    if (fr.ip >= f.code.size()) {
        // If we hit end of function without explicit return => error.
        throw std::runtime_error("VM: fell off end of function without return");
    }

    const BytecodeOperation op = f.code[fr.ip];
    fr.ip += 1;

    exec_op(op);

    // Halt condition: entry frame still exists and finished via RETURN.
    // (do_return() sets is_active_=false when popping last frame.)
}

void VirtualMachine::run() {
    while (is_active_) {
        step();
    }
}

} // namespace ds_lang