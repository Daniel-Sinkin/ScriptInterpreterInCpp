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

    std::string file_name = "examples/func.ds";
    std::string code = load_code(file_name);

    std::println("------------------------------------------------");
    std::println("> {}", file_name);
    std::println("------------------------------------------------");
    int line_counter = 0;
    std::print("[{:03}] ", line_counter);
    for(const char c : code) {
        std::print("{}", c);
        if (c == '\n') {
            std::print("[{:03}] ", ++line_counter);
        }
    }
    std::println();
    std::println("------------------------------------------------");
    std::println("");

    Lexer lexer{code};
    std::vector<Token> tokens = lexer.tokenize_all();
    Parser parser{tokens};

    auto statements = parser.parse_scope();

    Interpreter interpreter{true};

    std::println("Running Interpreter on the code");
    interpreter.process_scope(statements);

    std::println("");
}