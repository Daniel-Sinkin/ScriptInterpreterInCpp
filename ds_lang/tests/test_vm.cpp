// tests/test_vm.cpp
#include "common.hpp"

#include <print>
#include <string_view>
#include <vector>

#include "vm.hpp"

namespace ds_lang::Test {

static ds_lang::VirtualMachine build_single_function_vm(
    std::vector<ds_lang::BytecodeOperation> code,
    ds_lang::u32 num_locals = 0,
    ds_lang::u32 num_params = 0) {
    using namespace ds_lang;

    VirtualMachine vm{false};

    FunctionBytecode f0;
    f0.num_locals = num_locals;
    f0.num_params = num_params;
    f0.code = std::move(code);

    (void)vm.add_function(std::move(f0));
    vm.set_entry_function(0);
    vm.reset();
    return vm;
}

static void test_push_and_pop() {
    using namespace ds_lang;

    auto vm = build_single_function_vm({
        BytecodePushI64{1},
        BytecodePushI64{2},
        BytecodePop{},      // drops 2
        BytecodePushI64{3},
        BytecodePop{},      // drops 3
        BytecodePushI64{0}, // return value
        BytecodeReturn{},
    });

    vm.run();

    EXPECT_TRUE(!vm.is_active());
    EXPECT_EQ(vm.print_buffer().size(), static_cast<std::size_t>(0));

    EXPECT_EQ(vm.stack().size(), static_cast<std::size_t>(1));
}

static void test_arithmetic_ops() {
    using namespace ds_lang;

    // Compute 85, print it, pop it, push return value, return.
    auto vm = build_single_function_vm({
        BytecodePushI64{10},
        BytecodePushI64{20},
        BytecodeAdd{},      // 30
        BytecodePushI64{3},
        BytecodeMult{},     // 90
        BytecodePushI64{5},
        BytecodeSub{},      // 85
        BytecodePrint{},    // prints 85
        BytecodePop{},      // drop 85
        BytecodePushI64{0}, // return value
        BytecodeReturn{},
    });

    vm.run();

    EXPECT_EQ(vm.print_buffer().size(), static_cast<std::size_t>(1));
    EXPECT_EQ(vm.print_buffer()[0], static_cast<i64>(85));
}

static void test_div0_throws() {
    using namespace ds_lang;

    auto vm = build_single_function_vm({
        BytecodePushI64{1},
        BytecodePushI64{0},
        BytecodeDiv{},
        BytecodePushI64{0},
        BytecodeReturn{},
    });

    EXPECT_THROW(vm.run());
}

static void test_mod0_throws() {
    using namespace ds_lang;

    auto vm = build_single_function_vm({
        BytecodePushI64{1},
        BytecodePushI64{0},
        BytecodeMod{},
        BytecodePushI64{0},
        BytecodeReturn{},
    });

    EXPECT_THROW(vm.run());
}

static void test_comparison_ops() {
    using namespace ds_lang;

    // Prints:
    //  (3 < 4) => 1
    //  (4 == 4) => 1
    //  (5 != 6) => 1
    auto vm = build_single_function_vm({
        BytecodePushI64{3},
        BytecodePushI64{4},
        BytecodeLT{},
        BytecodePrint{},
        BytecodePop{},

        BytecodePushI64{4},
        BytecodePushI64{4},
        BytecodeEQ{},
        BytecodePrint{},
        BytecodePop{},

        BytecodePushI64{5},
        BytecodePushI64{6},
        BytecodeNEQ{},
        BytecodePrint{},
        BytecodePop{},

        BytecodePushI64{0}, // return value
        BytecodeReturn{},
    });

    vm.run();

    EXPECT_EQ(vm.print_buffer().size(), static_cast<std::size_t>(3));
    EXPECT_EQ(vm.print_buffer()[0], static_cast<i64>(1));
    EXPECT_EQ(vm.print_buffer()[1], static_cast<i64>(1));
    EXPECT_EQ(vm.print_buffer()[2], static_cast<i64>(1));
}

static void test_unary_ops() {
    using namespace ds_lang;

    // NEG(5) => -5
    // NOT(0) => 1
    // NOT(7) => 0
    auto vm = build_single_function_vm({
        BytecodePushI64{5},
        BytecodeNEG{},
        BytecodePrint{},
        BytecodePop{},

        BytecodePushI64{0},
        BytecodeNOT{},
        BytecodePrint{},
        BytecodePop{},

        BytecodePushI64{7},
        BytecodeNOT{},
        BytecodePrint{},
        BytecodePop{},

        BytecodePushI64{0}, // return value
        BytecodeReturn{},
    });

    vm.run();

    EXPECT_EQ(vm.print_buffer().size(), static_cast<std::size_t>(3));
    EXPECT_EQ(vm.print_buffer()[0], static_cast<i64>(-5));
    EXPECT_EQ(vm.print_buffer()[1], static_cast<i64>(1));
    EXPECT_EQ(vm.print_buffer()[2], static_cast<i64>(0));
}

static void test_locals_load_store() {
    using namespace ds_lang;

    // locals[0]=7; locals[1]=5; print locals[0]+locals[1] => 12
    auto vm = build_single_function_vm(
        {
            BytecodePushI64{7},
            BytecodeStoreLocal{0},

            BytecodePushI64{5},
            BytecodeStoreLocal{1},

            BytecodeLoadLocal{0},
            BytecodeLoadLocal{1},
            BytecodeAdd{},
            BytecodePrint{},
            BytecodePop{},

            BytecodePushI64{0}, // return value
            BytecodeReturn{},
        },
        /*num_locals=*/2,
        /*num_params=*/0);

    vm.run();

    EXPECT_EQ(vm.print_buffer().size(), static_cast<std::size_t>(1));
    EXPECT_EQ(vm.print_buffer()[0], static_cast<i64>(12));
}

static void test_jmp_skips_code() {
    using namespace ds_lang;

    // Jump over "print 111", execute "print 222"
    auto vm = build_single_function_vm({
        BytecodeJmp{4},         // -> ip 4
        BytecodePushI64{111},   // skipped
        BytecodePrint{},
        BytecodePop{},

        BytecodePushI64{222},
        BytecodePrint{},
        BytecodePop{},

        BytecodePushI64{0}, // return value
        BytecodeReturn{},
    });

    vm.run();

    EXPECT_EQ(vm.print_buffer().size(), static_cast<std::size_t>(1));
    EXPECT_EQ(vm.print_buffer()[0], static_cast<i64>(222));
}

static void test_jmpfalse_jmptrue() {
    using namespace ds_lang;

    // if (0) print 111 else print 222
    auto vm = build_single_function_vm({
        BytecodePushI64{0},
        BytecodeJmpTrue{6},

        BytecodePushI64{222},
        BytecodePrint{},
        BytecodePop{},
        BytecodeJmp{9},

        BytecodePushI64{111},
        BytecodePrint{},
        BytecodePop{},

        BytecodePushI64{0}, // return value
        BytecodeReturn{},
    });

    vm.run();

    EXPECT_EQ(vm.print_buffer().size(), static_cast<std::size_t>(1));
    EXPECT_EQ(vm.print_buffer()[0], static_cast<i64>(222));

    // if (0) print 111 else print 222 (using JMP_FALSE)
    auto vm2 = build_single_function_vm({
        BytecodePushI64{0},
        BytecodeJmpFalse{6},

        BytecodePushI64{111},
        BytecodePrint{},
        BytecodePop{},
        BytecodeJmp{9},

        BytecodePushI64{222},
        BytecodePrint{},
        BytecodePop{},

        BytecodePushI64{0}, // return value
        BytecodeReturn{},
    });

    vm2.run();

    EXPECT_EQ(vm2.print_buffer().size(), static_cast<std::size_t>(1));
    EXPECT_EQ(vm2.print_buffer()[0], static_cast<i64>(222));
}

static void test_call_and_return_with_args() {
    using namespace ds_lang;

    VirtualMachine vm{false};

    // f1(a,b): print a+b; return 0
    FunctionBytecode f1;
    f1.num_locals = 2;
    f1.num_params = 2;
    f1.code = {
        BytecodeLoadLocal{0},
        BytecodeLoadLocal{1},
        BytecodeAdd{},
        BytecodePrint{},
        BytecodePop{},
        BytecodePushI64{0},
        BytecodeReturn{},
    };

    // f0: call f1(7,5); return 0
    FunctionBytecode f0;
    f0.num_locals = 0;
    f0.num_params = 0;
    f0.code = {
        BytecodePushI64{7},
        BytecodePushI64{5},
        BytecodeCallArgs{1, 2},
        BytecodePop{},      // drop f1 return value
        BytecodePushI64{0},
        BytecodeReturn{},
    };

    (void)vm.add_function(std::move(f0)); // id 0
    (void)vm.add_function(std::move(f1)); // id 1
    vm.set_entry_function(0);
    vm.reset();
    vm.run();

    EXPECT_EQ(vm.print_buffer().size(), static_cast<std::size_t>(1));
    EXPECT_EQ(vm.print_buffer()[0], static_cast<i64>(12));
}

static void test_nested_calls() {
    using namespace ds_lang;

    VirtualMachine vm{false};

    // f2(x): print x*2; return 0
    FunctionBytecode f2;
    f2.num_locals = 1;
    f2.num_params = 1;
    f2.code = {
        BytecodeLoadLocal{0},
        BytecodePushI64{2},
        BytecodeMult{},
        BytecodePrint{},
        BytecodePop{},
        BytecodePushI64{0},
        BytecodeReturn{},
    };

    // f1(a,b): call f2(a+b); return 0
    FunctionBytecode f1;
    f1.num_locals = 2;
    f1.num_params = 2;
    f1.code = {
        BytecodeLoadLocal{0},
        BytecodeLoadLocal{1},
        BytecodeAdd{},
        BytecodeCallArgs{2, 1},
        BytecodePop{},      // drop f2 return value
        BytecodePushI64{0},
        BytecodeReturn{},
    };

    // f0: call f1(7,5); return 0
    FunctionBytecode f0;
    f0.num_locals = 0;
    f0.num_params = 0;
    f0.code = {
        BytecodePushI64{7},
        BytecodePushI64{5},
        BytecodeCallArgs{1, 2},
        BytecodePop{},      // drop f1 return value
        BytecodePushI64{0},
        BytecodeReturn{},
    };

    (void)vm.add_function(std::move(f0)); // 0
    (void)vm.add_function(std::move(f1)); // 1
    (void)vm.add_function(std::move(f2)); // 2

    vm.set_entry_function(0);
    vm.reset();
    vm.run();

    EXPECT_EQ(vm.print_buffer().size(), static_cast<std::size_t>(1));
    EXPECT_EQ(vm.print_buffer()[0], static_cast<i64>(24));
}

static void test_print_records_value() {
    using namespace ds_lang;

    auto vm = build_single_function_vm({
        BytecodePushI64{123},
        BytecodePrint{},
        BytecodePop{},
        BytecodePushI64{0},
        BytecodeReturn{},
    });

    vm.run();

    EXPECT_EQ(vm.print_buffer().size(), static_cast<std::size_t>(1));
    EXPECT_EQ(vm.print_buffer()[0], static_cast<i64>(123));
}

static void test_step_past_end_throws() {
    using namespace ds_lang;

    auto vm = build_single_function_vm({
        BytecodePushI64{0},
        BytecodeReturn{},
    });

    EXPECT_NO_THROW(vm.step()); // push
    EXPECT_NO_THROW(vm.step()); // return => halts
    EXPECT_TRUE(!vm.is_active());
    EXPECT_THROW(vm.step()); // inactive
}

} // namespace ds_lang::Test

int main(int argc, char** argv) {
    using namespace ds_lang::Test;

    struct TestCase {
        const char* name;
        void (*fn)();
    };

    const TestCase tests[] = {
        {"push_and_pop", test_push_and_pop},
        {"arithmetic_ops", test_arithmetic_ops},
        {"div0_throws", test_div0_throws},
        {"mod0_throws", test_mod0_throws},
        {"comparison_ops", test_comparison_ops},
        {"unary_ops", test_unary_ops},
        {"locals_load_store", test_locals_load_store},
        {"jmp_skips_code", test_jmp_skips_code},
        {"jmpfalse_jmptrue", test_jmpfalse_jmptrue},
        {"call_and_return_with_args", test_call_and_return_with_args},
        {"nested_calls", test_nested_calls},
        {"print_records_value", test_print_records_value},
        {"step_past_end_throws", test_step_past_end_throws},
    };

    std::string_view only;
    if (argc == 3 && std::string_view(argv[1]) == "--case") {
        only = argv[2];
    } else if (argc != 1) {
        std::println("Usage: {} [--case <name>]", argv[0]);
        return 2;
    }

    int passed = 0;
    int ran = 0;

    for (const auto& t : tests) {
        if (!only.empty() && only != t.name) {
            continue;
        }
        ++ran;
        try {
            t.fn();
            ++passed;
        } catch (const std::exception& e) {
            std::println("FAIL {}: {}", t.name, e.what());
            return 1;
        } catch (...) {
            std::println("FAIL {}: unknown exception", t.name);
            return 1;
        }
    }

    if (!only.empty() && ran == 0) {
        std::println("Unknown test case: {}", only);
        return 2;
    }

    std::println("OK {}/{}", passed, ran);
    return 0;
}