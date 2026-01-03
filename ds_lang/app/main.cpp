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

    std::string code = load_code("examples/simple.ds");

    Lexer lexer{code};
    std::vector<Token> tokens = lexer.tokenize_all();
    Parser parser{tokens};

    for (const auto &statement : parser.parse_scope()) {
        std::println("{}", statement);
    }

    // Interpreter interpreter{false};
    // interpreter.process_scope(parser.parse_scope());
}