// ds_lang/app/main.cpp
#include <format>
#include <print>
#include <string>
#include <vector>

#include "lexer.hpp"
#include "token.hpp"
#include "util.hpp"

int main() {
    using namespace ds_lang;

    std::string code = load_code("examples/simple.ds");

    std::vector<Token> tokens = Lexer{code}.take_tokens();
    for(const Token& token : tokens) {
        std::println("{}", token);
    }
}