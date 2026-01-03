// ds_lang/app/main.cpp
#include <print>
#include <string>
#include <vector>

#include "formatters.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "token.hpp"
#include "util.hpp"
#include "vm.hpp"

static ds_lang::FunctionBytecode build_function_square() {
    using namespace ds_lang;

    FunctionBytecode f;
    f.num_locals = 1;
    f.num_params = 1;
    f.code = {
        BytecodeLoadLocal{0},
        BytecodeLoadLocal{0},
        BytecodeMult{},
        BytecodePushI64{0},
        BytecodeAdd{},
        BytecodeReturn{},
    };
    return f;
}

static ds_lang::FunctionBytecode build_function_main() {
    using namespace ds_lang;

    FunctionBytecode f;
    f.num_locals = 3;
    f.num_params = 0;

    f.code = {
        BytecodePushI64{5},
        BytecodeStoreLocal{0},

        BytecodePushI64{0},
        BytecodeStoreLocal{1},

        BytecodeLoadLocal{0},
        BytecodePushI64{0},
        BytecodeGT{},
        BytecodeJmpFalse{18},

        BytecodeLoadLocal{0},
        BytecodeCallArgs{1, 1},
        BytecodeLoadLocal{1},
        BytecodeAdd{},
        BytecodeStoreLocal{1},

        BytecodeLoadLocal{0},
        BytecodePushI64{1},
        BytecodeSub{},
        BytecodeStoreLocal{0},

        BytecodeJmp{4},

        BytecodeLoadLocal{1},
        BytecodePrint{},
        BytecodePop{},

        BytecodePushI64{0},
        BytecodeReturn{},
    };

    return f;
}

static void print_function_code(const ds_lang::FunctionBytecode &fn, ds_lang::u32 func_id) {
    using namespace ds_lang;

    std::println("Function {}:", func_id);
    std::println("  num_locals = {}", fn.num_locals);
    std::println("  num_params = {}", fn.num_params);
    for (usize ip = 0; ip < fn.code.size(); ++ip) {
        std::println("  {:>4}: {}", ip, fn.code[ip]);
    }
}

int main() {
    using namespace ds_lang;

    const FunctionBytecode f_main = build_function_main();
    const FunctionBytecode f_square = build_function_square();

    std::println("Bytecode:");
    print_function_code(f_main, 0);
    print_function_code(f_square, 1);
    std::println();

    VirtualMachine vm{true};

    (void)vm.add_function(f_main);
    (void)vm.add_function(f_square);

    vm.set_entry_function(0);
    vm.reset();
    vm.run();

    std::println("VM stack: {}", vm.stack());
    std::println("VM prints: {}", vm.print_buffer());
}