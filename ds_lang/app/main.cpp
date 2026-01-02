// ds_lang/app/main.cpp
#include <print>
#include <string>

#include "lexer.hpp"
#include "util.hpp"

int main() {
    using namespace ds_lang;

    std::string code = load_code("examples/simple.ds");
    Lexer lexer{code};
    for (const auto& token : lexer.tokenize_all()) {
        std::println("{}", token);
    }
}