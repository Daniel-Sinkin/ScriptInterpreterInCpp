// tests/test_bytecode_builder.cpp
#include "common.hpp"

#include <print>
#include <string_view>
#include <vector>

#include "bytecode_builder.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "vm.hpp"
#include "util.hpp"

namespace ds_lang::Test {

static std::vector<Statement> parse_program(std::string_view code) {
    auto tokens = Lexer{code}.tokenize_all();
    Parser p{tokens};
    return p.parse_program();
}

static BytecodeBuilder build(std::string_view code) {
    auto program = parse_program(code);
    BytecodeBuilder b;
    b.build(program);
    return b;
}

static VirtualMachine run(const BytecodeBuilder& b) {
    VirtualMachine vm{false};
    for (const auto& fn : b.functions())
        (void)vm.add_function(fn);

    EXPECT_TRUE(b.entry_function().has_value());
    vm.set_entry_function(*b.entry_function());
    vm.reset();
    vm.run();
    return vm;
}

static void expect_jumps_patched(const FunctionBytecode& fn) {
    for (const auto& op : fn.code) {
        std::visit(overloaded{
            [&](const BytecodeJmp& j)       { EXPECT_TRUE(j.target_ip != INVALIDIPtr); },
            [&](const BytecodeJmpFalse& j)  { EXPECT_TRUE(j.target_ip != INVALIDIPtr); },
            [&](const BytecodeJmpTrue& j)   { EXPECT_TRUE(j.target_ip != INVALIDIPtr); },
            [&](const auto&) {}
        }, op);
    }
}

static void test_entry_function_is_main_even_if_not_first() {
    auto b = build(
        "func foo() { return 0; }"
        "func main() { return 0; }"
    );
    EXPECT_EQ(*b.entry_function(), 1u);
}

static void test_implicit_return_added() {
    auto b = build("func main() { int x = 1; }");
    const auto& fn = b.functions().at(*b.entry_function());
    EXPECT_TRUE(std::holds_alternative<BytecodeReturn>(fn.code.back()));
}

static void test_print_string_emits_printstring() {
    auto b = build("func main() { print \"hello\"; return 0; }");
    const auto& fn = b.functions().at(*b.entry_function());
    auto* ps = std::get_if<BytecodePrintString>(&fn.code[0]);
    EXPECT_TRUE(ps && ps->content == "hello");
}

static void test_locals_scoping_and_shadowing() {
    auto b = build(
        "func main() {"
        " int x = 1;"
        " { int x = 2; print x; }"
        " print x;"
        " return 0;"
        "}"
    );
    auto vm = run(b);
    EXPECT_EQ(vm.print_buffer()[0], "2");
    EXPECT_EQ(vm.print_buffer()[1], "1");
}

static void test_if_else_patches_jumps() {
    auto b = build(
        "func main() { if (1) { print 1; } else { print 2; } return 0; }"
    );
    expect_jumps_patched(b.functions().at(*b.entry_function()));
}

static void test_while_loop_runs() {
    auto b = build(
        "func main() { int x=0; while(x<3){ print x; x=x+1; } return 0; }"
    );
    auto vm = run(b);
    EXPECT_EQ(vm.print_buffer().size(), 3u);
}

static void test_short_circuit_avoids_div0() {
    auto b = build(
        "func main() {"
        " if (0 && 1/0) { print 1; } else { print 2; }"
        " if (1 || 1/0) { print 3; }"
        " return 0;"
        "}"
    );
    auto vm = run(b);
    EXPECT_EQ(vm.print_buffer()[0], "2");
    EXPECT_EQ(vm.print_buffer()[1], "3");
}

static void test_function_call_and_args() {
    auto b = build(
        "func add(a,b){ return a+b; }"
        "func main(){ print add(7,5); return 0; }"
    );
    auto vm = run(b);
    EXPECT_EQ(vm.print_buffer()[0], "12");
}

static void test_missing_main_throws() {
    EXPECT_THROW(build("func foo(){return 0;}"));
}

static void test_top_level_non_function_throws() {
    EXPECT_THROW(build("int x = 1;"));
}

static void test_undefined_variable_throws() {
    EXPECT_THROW(build("func main(){ x=1; return 0; }"));
}

static void test_undefined_function_throws() {
    EXPECT_THROW(build("func main(){ print foo(1); return 0; }"));
}

static void test_duplicate_function_throws() {
    EXPECT_THROW(build(
        "func main(){return 0;}"
        "func main(){return 0;}"
    ));
}

} // namespace ds_lang::Test

int main(int argc, char** argv) {
    using namespace ds_lang::Test;

    struct Test { const char* name; void(*fn)(); };
    const Test tests[] = {
        {"entry_function_is_main_even_if_not_first", test_entry_function_is_main_even_if_not_first},
        {"implicit_return_added", test_implicit_return_added},
        {"print_string_emits_printstring", test_print_string_emits_printstring},
        {"locals_scoping_and_shadowing", test_locals_scoping_and_shadowing},
        {"if_else_patches_jumps", test_if_else_patches_jumps},
        {"while_loop_runs", test_while_loop_runs},
        {"short_circuit_avoids_div0", test_short_circuit_avoids_div0},
        {"function_call_and_args", test_function_call_and_args},
        {"missing_main_throws", test_missing_main_throws},
        {"top_level_non_function_throws", test_top_level_non_function_throws},
        {"undefined_variable_throws", test_undefined_variable_throws},
        {"undefined_function_throws", test_undefined_function_throws},
        {"duplicate_function_throws", test_duplicate_function_throws},
    };

    std::string_view only;
    if (argc == 3 && argv[1] == std::string("--case")) only = argv[2];

    int ran = 0;
    for (auto& t : tests) {
        if (!only.empty() && only != t.name) continue;
        ++ran;
        t.fn();
    }

    EXPECT_TRUE(ran > 0);
    std::println("OK {}", ran);
}