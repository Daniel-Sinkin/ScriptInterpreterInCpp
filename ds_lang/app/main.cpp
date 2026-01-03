// ds_lang/app/main.cpp
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
    auto scope = parser.parse_scope();
    Interpreter interpreter{false};
    for (const auto &statement : scope) {
        if (interpreter.process_statement(statement) != Interpreter::ExecResult::Continue) {
            break;
        }
    }
}