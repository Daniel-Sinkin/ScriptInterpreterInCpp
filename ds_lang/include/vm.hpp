// ds_lang/include/vm.hpp
#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

#include "parser.hpp"
#include "types.hpp"
#include "util.hpp"

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
    IPtr target_ip{INVALIDIPtr};
};

struct BytecodeJmpFalse {
    IPtr target_ip{INVALIDIPtr};
};

struct BytecodeJmpTrue {
    IPtr target_ip{INVALIDIPtr};
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
    std::vector<BytecodeOperation> code{};
    std::vector<std::string> seen_symbols{};
    u32 num_locals{0};
    u32 num_params{0};
};

struct BytecodeBuilder {
    bool inside_func_{false}; // No nested functions
    std::vector<FunctionBytecode> func_code_{};
    usize active_func_{0zu};
    std::optional<usize> main_func_idx_{std::nullopt};

    IPtr current_ip() const noexcept {
        return func_code().code.size();
    }

    const FunctionBytecode &func_code() const {
        return func_code_[active_func_];
    }

    FunctionBytecode &func_code() {
        return func_code_[active_func_];
    }

    IPtr emit(BytecodeOperation op) {
        const IPtr ip = current_ip();
        func_code().code.push_back(std::move(op));
        return ip;
    }

    BytecodeOperation &at(IPtr ip) {
        return func_code().code.at(ip);
    }

    void set_jump_target(IPtr ip, IPtr target) {
        if (target == INVALIDIPtr) {
            throw std::runtime_error("trying to set jump target to INVALIDPtr!");
        }

        BytecodeOperation &op = at(ip);
        std::visit(
            overloaded{
                [&](BytecodeJmp &j) -> void {
                    if (j.target_ip != INVALIDIPtr) {
                        throw std::runtime_error("Jump targets should only be set once!");
                    }
                    j.target_ip = target;
                },
                [&](BytecodeJmpFalse &j) -> void {
                    if (j.target_ip != INVALIDIPtr) {
                        throw std::runtime_error("Jump targets should only be set once!");
                    }
                    j.target_ip = target;
                },
                [&](BytecodeJmpTrue &j) -> void {
                    if (j.target_ip != INVALIDIPtr) {
                        throw std::runtime_error("Jump targets should only be set once!");
                    }
                    j.target_ip = target;
                },
                [&](auto &) { throw std::runtime_error("patch_jump: not a jump op"); },
            },
            op);
    }

    void build(const std::vector<Statement>& statements) {
        for (const auto& st : statements) {
            build_statement(st);
        }
    }

    void build_expression(const Expression &expr) {
        std::visit(
            overloaded{
                [&](const IntegerExpression &e) -> void {
                    emit(BytecodePushI64{e.value});
                },
                [&](const IdentifierExpression &e) -> void {
                    (void)e;
                    std::unreachable();
                },
                [&](const UnaryExpression &e) -> void {
                    assert(e.rhs);
                    build_expression(*e.rhs);
                    switch (e.op) {
                    case UnaryOp::Neg:
                        emit(BytecodeNEG{});
                        break;
                    case UnaryOp::Not:
                        emit(BytecodeNOT{});
                        break;
                    }
                },
                [&](const BinaryExpression &e) -> void {
                    assert(e.lhs);
                    assert(e.rhs);

                    build_expression(*e.lhs);

                    bool has_short_circuit = false;

                    IPtr jmp_ptr = INVALIDIPtr;
                    if (e.op == BinaryOp::And) {
                        has_short_circuit = true;
                        jmp_ptr = emit(BytecodeJmpFalse{INVALIDIPtr}); // 0 && b == 0
                    } else if (e.op == BinaryOp::Or) {
                        has_short_circuit = true;
                        jmp_ptr = emit(BytecodeJmpTrue{INVALIDIPtr}); // 1 && b == 1
                    }

                    build_expression(*e.rhs);
                    if (has_short_circuit) {
                        assert(jmp_ptr != INVALIDIPtr);
                        IPtr rhs_jmp_ptr = emit(BytecodeJmp{INVALIDIPtr});
                        set_jump_target(jmp_ptr, current_ip());
                        if (e.op == BinaryOp::And) {
                            emit(BytecodePushI64{0}); // 0 && b == 0
                        } else if (e.op == BinaryOp::Or) {
                            emit(BytecodePushI64{1}); // 1 || b == 1
                        } else {
                            std::unreachable();
                        }
                        set_jump_target(rhs_jmp_ptr, current_ip());
                        return;
                    }

                    switch (e.op) {
                    case BinaryOp::Add:
                        emit(BytecodeAdd{});
                        break;
                    case BinaryOp::Sub:
                        emit(BytecodeSub{});
                        break;
                    case BinaryOp::Mul:
                        emit(BytecodeMult{});
                        break;
                    case BinaryOp::Div:
                        emit(BytecodeDiv{});
                        break;
                    case BinaryOp::Mod:
                        emit(BytecodeMod{});
                        break;
                    case BinaryOp::Eq:
                        emit(BytecodeEQ{});
                        break;
                    case BinaryOp::Neq:
                        emit(BytecodeNEQ{});
                        break;
                    case BinaryOp::Lt:
                        emit(BytecodeLT{});
                        break;
                    case BinaryOp::Le:
                        emit(BytecodeLE{});
                        break;
                    case BinaryOp::Gt:
                        emit(BytecodeGT{});
                        break;
                    case BinaryOp::Ge:
                        emit(BytecodeGE{});
                        break;
                    case BinaryOp::And:
                    case BinaryOp::Or:
                        std::unreachable();
                    }
                },
                [&](const CallExpression &e) -> void {
                    (void)e;
                    std::unreachable();
                }},
            expr.node);
    }

    void build_statement(const Statement &statement) {
        std::visit(
            overloaded{
                [&](const IntDeclarationStatement &st) -> void {
                    (void)st;
                    std::unreachable();
                },
                [&](const IntAssignmentStatement &st) -> void {
                    (void)st;
                    std::unreachable();
                },
                [&]([[maybe_unused]] const PrintStatement &st) -> void {
                    build_expression(*st.expr);
                    emit(BytecodePrint{});
                },
                [&]([[maybe_unused]] const ReturnStatement &st) -> void {
                    build_expression(*st.expr);
                    emit(BytecodeReturn{});
                },
                [&](const ScopeStatement &st) -> void {
                    (void)st;
                    std::unreachable();
                },
                [&](const IfStatement &st) -> void {
                    (void)st;
                    std::unreachable();
                },
                [&](const WhileStatement &st) -> void {
                    (void)st;
                    std::unreachable();
                },
                [&](const FunctionStatement &st) -> void {
                    assert(!inside_func_);
                    inside_func_ = true;

                    if (st.func_name == "main") {
                        assert(!main_func_idx_);
                        main_func_idx_ = func_code_.size();
                    }
                    active_func_ = func_code_.size();
                    func_code_.push_back(FunctionBytecode{{}, {}, 0, static_cast<u32>(st.vars.size())});
                    for (const Statement &statement : st.statements) {
                        build_statement(statement);
                    }

                    inside_func_ = false;
                },
            },
            statement.node);
    }
};

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
    [[nodiscard]] const std::vector<i64> &print_buffer() const noexcept { return print_buffer_; }

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
    std::vector<i64> print_buffer_{};
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