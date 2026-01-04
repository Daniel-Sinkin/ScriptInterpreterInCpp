// ds_lang/include/vm.hpp
#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>
#include <limits>

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

struct BytecodeBuilder {
    std::vector<BytecodeOperation> code_;

    using IP = usize; // IP = Instruction Pointer
    usize INVALID_IP = std::numeric_limits<IP>::max();

    IP current_ip() const noexcept {
        return code_.size();
    }

    IP emit(BytecodeOperation op) {
        const IP ip = current_ip();
        code_.push_back(std::move(op));
        return ip;
    }

    void patch_jump(IP at, IP target) {
        auto& op = code_.at(at);
        std::visit(
            overloaded{
                [&](BytecodeJmp& j) { j.target_ip = target; },
                [&](BytecodeJmpFalse& j) { j.target_ip = target; },
                [&](BytecodeJmpTrue& j) { j.target_ip = target; },
                [&](auto&) { throw std::runtime_error("patch_jump: not a jump op"); },
            },
            op
        );
    }


    void build_expression(const Expression& expr) {
        std::visit(
            overloaded{
                [&](const IntegerExpression &e) -> void {
                    code_.push_back(BytecodePushI64{e.value});
                },
                [&](const IdentifierExpression &e) -> void {
                    (void)e;
                    std::unreachable();
                },
                [&](const UnaryExpression &e) -> void {
                    assert(e.rhs);
                    build_expression(*e.rhs);
                    switch(e.op) {
                        case UnaryOp::Neg:
                            code_.push_back(BytecodeNEG{});
                            break;
                        case UnaryOp::Not:
                            code_.push_back(BytecodeNOT{});
                            break;
                    }
                },
                [&](const BinaryExpression &e) -> void {
                    assert(e.lhs);
                    assert(e.rhs);
                    std::vector<BytecodeOperation> out;

                    build_expression(*e.lhs);

                    IP jmp_ptr = INVALID_IP;
                    if(e.op == BinaryOp::And) {
                        jmp_ptr = emit(BytecodeJmpFalse{});
                    }
                    else if(e.op == BinaryOp::Or) {
                        jmp_ptr = emit(BytecodeJmpTrue{});
                    }

                    IP rhs_ptr = current_ip();
                    build_expression(*e.rhs);
                    if(jmp_ptr != INVALID_IP) {
                        patch_jump(jmp_ptr, rhs_ptr);
                        return;
                    }

                    switch(e.op) {
                        case BinaryOp::Add:
                            out.push_back(BytecodeAdd{});
                            break;
                        case BinaryOp::Sub:
                            out.push_back(BytecodeSub{});
                            break;
                        case BinaryOp::Mul:
                            out.push_back(BytecodeMult{});
                            break;
                        case BinaryOp::Div:
                            out.push_back(BytecodeDiv{});
                            break;
                        case BinaryOp::Mod:
                            out.push_back(BytecodeMod{});
                            break;
                        case BinaryOp::Eq:
                            out.push_back(BytecodeEQ{});
                            break;
                        case BinaryOp::Neq:
                            out.push_back(BytecodeNEQ{});
                            break;
                        case BinaryOp::Lt:
                            out.push_back(BytecodeLT{});
                            break;
                        case BinaryOp::Le:
                            out.push_back(BytecodeLE{});
                            break;
                        case BinaryOp::Gt:
                            out.push_back(BytecodeGT{});
                            break;
                        case BinaryOp::Ge:
                            out.push_back(BytecodeGE{});
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

    void build_statement(const Statement& statement) {
        std::visit(
            overloaded{
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
                    (void)st;
                    std::unreachable();
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
        usize ip{0};
        std::vector<i64> locals{};
    };

    std::vector<i64> stack_{};
    std::vector<i64> print_buffer_{};
    std::vector<FunctionBytecode> functions_{};

    std::optional<u32> entry_func_{std::nullopt};
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