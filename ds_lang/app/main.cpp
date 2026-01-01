// app/main.cpp
#include <algorithm>
#include <cassert>
#include <expected>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <print>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "ast.hpp"
#include "env.hpp"
#include "interpreter.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "token.hpp"
#include "types.hpp"
#include "util.hpp"

namespace ds_lang
{
void print_env(const Environment &env)
{
    std::println("Environment =");
    std::println("\t{{");

    bool first = true;
    for (const auto &[key, value] : env.variables_)
    {
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

    Environment env;

    // File mode (optional)
    if (argc == 2)
    {
        const std::string code = load_code(argv[1]);
        const auto stmts = tokenize_code(code);
        for (const auto &toks : stmts)
        {
            auto st = parse_statement(toks);
            if (!st)
            {
                std::println("Parse error.");
                return 1;
            }
            auto ex = execute_statement(env, *st);
            if (!ex)
            {
                std::println("Execution error.");
                return 1;
            }
        }
        return 0;
    }

    // Interactive mode
    std::println("Interactive mode (Ctrl-D to exit).");

    std::string line;
    while (true)
    {
        std::print("> ");
        if (!std::getline(std::cin, line)) break;

        if (line.empty()) continue;

        // Convenience: allow "exit" / "quit"
        if (line == "exit" || line == "quit") break;

        // Your lexer expects ';' to terminate statements; add it if missing.
        if (!line.ends_with(';')) line.push_back(';');

        const auto stmts = tokenize_code(line);
        if (stmts.empty()) continue;

        // One line should usually yield exactly one statement.
        for (const auto &toks : stmts)
        {
            auto st = parse_statement(toks);
            if (!st)
            {
                std::println("Parse error.");
                continue;
            }

            auto ex = execute_statement(env, *st);
            if (!ex)
            {
                auto err = ex.error().expression_error;
                std::println("Failed to execute statement: {} [{}]", explain(err), to_string(err));
                continue;
            }
        }
    }

    std::println("Bye.");
    return 0;
}