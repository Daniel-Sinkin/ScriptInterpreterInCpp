// ds_lang/app/main.cpp
#include <algorithm>
#include <arm/_endian.h>
#include <optional>
#include <print>
#include <ranges>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "token.hpp"
#include "util.hpp"

void example() {
    using namespace ds_lang;

    std::string file_name = "examples/simple.ds2";
    std::string code = load_code(file_name);

    Lexer lexer{code};
    std::vector<Token> tokens = lexer.tokenize_all();
    std::println("Generated Tokens:");
    std::println();
    for (const Token &token : tokens) {
        std::println("{}", token);
    }

    Parser parser{tokens};

    std::println();
    std::println("Reproduced source code out of parsed expressions:");
    std::println();
    auto statements = parser.parse_program();
    for (const auto &statement : statements) {
        std::println("{}", statement);
    }
}

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
struct BytecodeCall {
    u32 func_id;
};
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
    u32 num_locals;
    u32 num_params;
};

class VirtualMachine {
public:
    void step() {
        if (!is_active_) {
            std::runtime_error("Trying to step but VM has finished running.");
        }
        if (op_ptr_ >= get_code().size()) {
            throw std::runtime_error("Trying to step past the end of the byte code.");
        }
        process_op(get_code()[op_ptr_]);
        ++op_ptr_;
        if(call_stack_.size() == 1 && op_ptr_ == get_code().size()) {
            is_active_ = false;
        }
    }

    void setup() {
        functions_.push_back(FunctionBytecode{
            .code = {BytecodePushI64{10}, BytecodePushI64{20},BytecodeAdd{}, BytecodePrint{}, BytecodePop{}},
            .num_locals = 0,
            .num_params = 0,
        });
    }

    ~VirtualMachine() noexcept {
        try {
            std::println("Virtual Machine result");

            std::println("Stack: {}", stack_);
            std::println("Locals: {}", locals_);
        } catch (...) {
        }
    }

    bool is_active() const {
        return is_active_;
    }

private:
    std::vector<i64> stack_;
    std::vector<i64> locals_;
    usize op_ptr_{0zu};
    std::optional<i64> return_value_{std::nullopt};

    std::vector<FunctionBytecode> functions_;
    usize func_ptr_{0zu};

    bool is_active_ = true;

    const std::vector<BytecodeOperation>& get_code() const {
        return functions_[func_ptr_].code;
    }

    struct CallStackEntry {
        usize func_ptr;
        usize op_ptr;
    };

    std::vector<CallStackEntry> call_stack_{CallStackEntry{0, 0}};

    [[nodiscard]] i64 pop() {
        i64 val = stack_.back();
        stack_.pop_back();
        return val;
    }

    void process_op(const BytecodeOperation op) {
        std::visit(
            overloaded{
                [&](const BytecodePushI64 &op) -> void {
                    stack_.push_back(op.value);
                },
                [&]([[maybe_unused]] const BytecodeAdd &op) -> void {
                    i64 v1 = pop();
                    i64 v2 = pop();
                    stack_.push_back(v1 + v2);
                },
                [&]([[maybe_unused]] const BytecodeSub &op) -> void {
                    i64 v1 = pop();
                    i64 v2 = pop();
                    stack_.push_back(v1 - v2);
                },
                [&]([[maybe_unused]] const BytecodeMult &op) -> void {
                    i64 v1 = pop();
                    i64 v2 = pop();
                    stack_.push_back(v1 * v2);
                },
                [&]([[maybe_unused]] const BytecodeDiv &op) -> void {
                    i64 v1 = pop();
                    i64 v2 = pop();
                    if (v2 == 0) {
                        throw std::runtime_error("Div0");
                    }
                    stack_.push_back(v1 / v2);
                },
                [&]([[maybe_unused]] const BytecodeMod &op) -> void {
                    i64 v1 = pop();
                    i64 v2 = pop();
                    stack_.push_back(v1 + v2);
                },
                [&]([[maybe_unused]] const BytecodeEQ &op) -> void {
                    i64 v1 = pop();
                    i64 v2 = pop();
                    stack_.push_back(v1 == v2);
                },
                [&]([[maybe_unused]] const BytecodeNEQ &op) -> void {
                    i64 v1 = pop();
                    i64 v2 = pop();
                    stack_.push_back(v1 != v2);
                },
                [&]([[maybe_unused]] const BytecodeLT &op) -> void {
                    i64 v1 = pop();
                    i64 v2 = pop();
                    stack_.push_back(v1 < v2);
                },
                [&]([[maybe_unused]] const BytecodeLE &op) -> void {
                    i64 v1 = pop();
                    i64 v2 = pop();
                    stack_.push_back(v1 <= v2);
                },
                [&]([[maybe_unused]] const BytecodeGT &op) -> void {
                    i64 v1 = pop();
                    i64 v2 = pop();
                    stack_.push_back(v1 > v2);
                },
                [&]([[maybe_unused]] const BytecodeGE &op) -> void {
                    i64 v1 = pop();
                    i64 v2 = pop();
                    stack_.push_back(v1 >= v2);
                },
                [&]([[maybe_unused]] const BytecodeNEG &op) -> void {
                    stack_.push_back(-pop());
                },
                [&]([[maybe_unused]] const BytecodeNOT &op) -> void {
                    const i64 v1 = pop();
                    stack_.push_back(v1 <= 0);
                },
                [&]([[maybe_unused]] const BytecodePop &op) -> void {
                    stack_.pop_back();
                },
                [&](const BytecodeLoadLocal &op) -> void {
                    stack_.push_back(locals_[op.slot]);
                },
                [&](const BytecodeStoreLocal &op) -> void {
                    stack_.push_back(locals_[op.slot]);
                },
                [&](const BytecodeJmp &op) -> void {
                    op_ptr_ = static_cast<usize>(op.target_ip);
                },
                [&](const BytecodeJmpFalse &op) -> void {
                    i64 val = pop();
                    if (val <= 0) {
                        op_ptr_ = static_cast<usize>(op.target_ip);
                    }
                },
                [&](const BytecodeJmpTrue &op) -> void {
                    i64 val = pop();
                    if (val > 0) {
                        op_ptr_ = static_cast<usize>(op.target_ip);
                    }
                },
                [&](const BytecodeCall &op) -> void {
                    call_stack_.emplace_back(func_ptr_, op_ptr_);
                    op_ptr_ = 0zu;
                    func_ptr_ = static_cast<usize>(op.func_id);
                    std::unreachable();
                },
                [&](const BytecodeCallArgs &op) -> void {
                    (void)op; // TODO implement this
                    std::unreachable();
                },
                [&]([[maybe_unused]] const BytecodeReturn &op) -> void {
                    if (call_stack_.size() == 1) {
                        is_active_ = false;
                    }
                    const CallStackEntry &prev = call_stack_.back();
                    func_ptr_ = prev.func_ptr;
                    op_ptr_ = prev.op_ptr;
                },
                [&]([[maybe_unused]] const BytecodePrint &op) -> void {
                    if(!stack_.empty()) {
                        std::println("VM Print: [{}]", stack_.back());
                    } else {
                        std::println("VM Print: [<EMPTY>]");
                    }
                }},
            op);
    }
};
} // namespace ds_lang

int main() {
    using namespace ds_lang;

    VirtualMachine vm;
    vm.setup();

    while(vm.is_active()) {
        vm.step();
    }

    std::println();
}