// app/main.cpp
#include <algorithm>
#include <cassert>
#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "ast.hpp"
#include "lexer.hpp"
#include "token.hpp"
#include "types.hpp"
#include "util.hpp"
#include "env.hpp"
#include "interpreter.hpp"
#include "parser.hpp"

namespace ds_lang
{
void print_env(const Environment& env) {
    std::println("Environment =");
    std::println("\t{{");

    bool first = true;
    for (const auto& [key, value] : env.variables_) {
        if (!first) std::println(",");
        first = false;
        std::print("\t\t\"{}\": {}", key, value);
    }

    if (!first) std::println();
    std::println("\t}};");
}
} // namespace ds_lang


int main(int argc, char **argv)
{
    using namespace ds_lang;
    if (argc < 2)
    {
        std::println("Usage: ds_lang_cli <file.ds>");
        return 1;
    }
    std::string code = load_code(argv[1]);
    std::println("The code:\n{}\n", code);

    std::vector<Tokens> statement_tokens = tokenize_code(code);
    if (statement_tokens.empty())
    {
        return 0;
    }

    std::println("Found {} statements:", statement_tokens.size());
    for (size_t i = 0; i < statement_tokens.size(); ++i)
    {
        std::print("Statement[{:2}] = ", i);
        print_tokens(statement_tokens[i]);
    }
    std::println();

    Environment env;
    std::println("Initialised Runtime");
    print_env(env);
    for (size_t i = 0; i < statement_tokens.size(); ++i)
    {
        std::print("Executing the {}. statement: ", i + 1);
        print_tokens(statement_tokens[i]);

        auto parsed_statement = parse_statement(statement_tokens[i]);
        if (!parsed_statement)
        {
            std::println(
                "Failed to parse statement with error code {}",
                static_cast<int>(parsed_statement.error()));
            return 1;
        }
        auto excuted_statement = execute_statement(env, *parsed_statement);
        if (!excuted_statement)
        {
            std::println(
                "Failed to execute statement with error code {}",
                static_cast<int>(excuted_statement.error()));
            return 1;
        }
        print_env(env);
        std::println();
    }
}