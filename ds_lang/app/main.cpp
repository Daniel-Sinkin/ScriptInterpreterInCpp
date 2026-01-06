// ds_lang/app/main.cpp
#include <print>
#include <string>
#include <vector>

#include "formatters.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "util.hpp"
#include "bytecode_builder.hpp"
#include "vm.hpp"

int main() {
    using namespace ds_lang;

    // Load source
    std::string code = load_code("examples/simple.ds");

    // Lex + parse
    std::vector<Token> tokens = Lexer{code}.tokenize_all();
    std::vector<Statement> statements = Parser{tokens}.parse_program();

    // Dump tokens
    std::println("Tokens:");
    for (const Token& token : tokens) {
        std::println("{}", token);
    }
    std::println();

    // Dump parsed functions (AST)
    std::println("Functions:");
    for (usize i = 0; i < statements.size(); ++i) {
        std::println("[{:03}]\n{}", i, statements[i]);
    }
    std::println();

    // Build bytecode
    BytecodeBuilder builder{};
    builder.build(statements);

    // Dump bytecode
    std::println("Bytecode:");
    for (usize i = 0; i < builder.functions().size(); ++i) {
        const FunctionBytecode& fn = builder.functions()[i];
        std::println("Function {}:", static_cast<u32>(i));
        std::println("  num_params = {}", fn.num_params);
        std::println("  num_locals = {}", fn.num_locals);
        for (usize ip = 0; ip < fn.code.size(); ++ip) {
            std::println("  {:>4}: {}", ip, fn.code[ip]);
        }
    }
    std::println();

    // Run VM
    VirtualMachine vm{true};
    for (const auto& fn : builder.functions()) {
        (void)vm.add_function(fn);
    }

    vm.set_entry_function(*builder.entry_function());
    vm.reset();
    vm.run();

    std::println("Return Value = {}", vm.get_return_value());
}