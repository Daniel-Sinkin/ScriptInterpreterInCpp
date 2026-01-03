// ds_lang/app/main.cpp
#include <print>
#include <string>
#include <vector>

#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "token.hpp"
#include "util.hpp"

int main() {
    using namespace ds_lang;

    std::string file_name = "examples/simple.ds2";
    std::string code = load_code(file_name);

    Lexer lexer{code};
    std::vector<Token> tokens = lexer.tokenize_all();
    std::println("Generated Tokens:");
    std::println();
    for(const Token& token : tokens) {
        std::println("{}", token);
    }

    Parser parser{tokens};    

    std::println();
    std::println("Reproduced source code out of parsed expressions:");
    std::println();
    auto statements = parser.parse_scope();
    for(const auto& statement : statements) {
        std::println("{}", statement);
    }

    // Interpreter interpreter{true};
    // std::println("Running Interpreter on the code");
    // interpreter.process_scope(statements);
}