// ds_lang/app/main.cpp
#include <format>
#include <print>
#include <string>
#include <vector>

#include "lexer.hpp"
#include "token.hpp"
#include "util.hpp"

namespace ds_lang {

} // namespace ds_lang

int main() {
    using namespace ds_lang;

    load_code("examples/new.ds2");

    std::string code = "LET x = 1\nPRINT x";

    std::vector<Token> tokens = Lexer{code}.take_tokens();
    for(const Token& token : tokens) {
        std::println("{}", token);
    }
}