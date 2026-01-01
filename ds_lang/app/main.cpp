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

namespace ds_lang
{

enum class ParseExpressionError
{
    Generic,
    MalformedExpression,
    EmptyStatement,
    NotImplemented
};
struct Parser
{
public:
    std::span<const Token> toks_;
    size_t pos_ = 0;

    [[nodiscard]] bool end_of_file() const noexcept { return pos_ >= toks_.size(); }
    [[nodiscard]] const Token *peek() const noexcept { return end_of_file() ? nullptr : &toks_[pos_]; }

    [[nodiscard]] const Token &consume()
    {
        assert(!end_of_file());
        return toks_[pos_++];
    }

    struct BpPair
    {
        int left;
        int right;
    };

    [[nodiscard]] static std::optional<BpPair>
    infix_bp(const Token &tok) noexcept
    {
        // Return nullopt if tok is not an infix operator in expressions.
        if (auto bop = std::get_if<BinaryOperator>(&tok))
        {
            switch (*bop)
            {
            case BinaryOperator::Star:
            case BinaryOperator::Slash:
                return BpPair{70, 71}; // left-assoc

            case BinaryOperator::Plus:
            case BinaryOperator::Minus:
                return BpPair{60, 61}; // left-assoc

            case BinaryOperator::GreaterThan:
            case BinaryOperator::LessThan:
            case BinaryOperator::GreaterEqualThan:
            case BinaryOperator::LessEqualThan:
                return BpPair{50, 51}; // left-assoc (non-chain in many langs; Pratt still parses)

            case BinaryOperator::DoubleEqual:
                return BpPair{40, 41}; // left-assoc

            default:
                return std::nullopt;
            }
        }
        return std::nullopt;
    }

    [[nodiscard]]
    std::expected<Expression, ParseExpressionError>
    parse_expr(int min_bp = 0)
    {
        using E = ParseExpressionError;

        if (end_of_file()) return std::unexpected{E::MalformedExpression};
        ;

        Expression lhs;
        {
            const Token &t = consume();

            if (auto tid = std::get_if<TokenInteger>(&t))
            {
                lhs.node = ValueExpression{.value = tid->value};
            }
            else if (auto tvar = std::get_if<TokenIdentifier>(&t))
            {
                lhs.node = VariableExpression{.name = tvar->name};
            }
            else
            {
                return std::unexpected{E::NotImplemented};
            }
        }

        while (true)
        {
            const Token *next = peek();
            if (!next) break;

            auto bp = infix_bp(*next);
            if (!bp || bp->left < min_bp) break;

            BinaryOperator op = std::get<BinaryOperator>(consume());

            auto rhs = parse_expr(bp->right);
            if (!rhs) return std::unexpected{rhs.error()};

            BinaryExpression b{
                .op = op,
                .exp1 = std::make_unique<Expression>(std::move(lhs)),
                .exp2 = std::make_unique<Expression>(std::move(*rhs)),
            };
            lhs.node = std::move(b);
        }

        return lhs;
    }
};

[[nodiscard]]
std::expected<Expression, ParseExpressionError>
parse_expression(std::span<const Token> tokens)
{
    Parser p{.toks_ = tokens, .pos_ = 0};
    auto expr = p.parse_expr(0);
    if (!expr) return expr;
    if (p.pos_ != tokens.size()) return std::unexpected{ParseExpressionError::MalformedExpression};
    return expr;
}
enum class ParseStatementError
{
    NoKeywordAtStart,
    MalformedStatement,
    MalformedExpression,
    InvalidKeyword,
    EmptyStatement,
    InvalidLength,
    NotImplementedKeyword,
    NotImplemented,
    Generic
};
[[nodiscard]]
std::expected<Statement, ParseStatementError>
parse_statement(const Tokens &tokens)
{
    using E = ParseStatementError;
    if (tokens.empty()) return std::unexpected{E::EmptyStatement};
    if (auto kw = std::get_if<TokenKeyword>(&tokens[0]))
    {
        switch (*kw)
        {
        case TokenKeyword::Int:
        {
            if (tokens.size() < 4) return std::unexpected{E::InvalidLength};
            auto var_name = std::get_if<TokenIdentifier>(&tokens[1]);
            auto equals_op = std::get_if<TokenOperator>(&tokens[2]);
            if (!var_name || !equals_op || *equals_op != TokenOperator::Equal)
            {
                return std::unexpected{E::MalformedStatement};
            }
            auto res = parse_expression(std::span{tokens}.subspan(3));
            if (!res)
            {
                return std::unexpected{E::MalformedExpression};
            }
            return AssignmentStatement{
                .identifier = var_name->name,
                .expr = std::move(*res)};
        }
        case TokenKeyword::Print:
        {
            if (tokens.size() < 2) return std::unexpected{E::InvalidLength};
            auto res = parse_expression(std::span{tokens}.subspan(1));
            if (!res)
            {
                return std::unexpected{E::MalformedExpression};
            }
            return PrintStatement{.expr = std::move(*res)};
        }
        case TokenKeyword::If:
        {
            return std::unexpected{E::NotImplementedKeyword};
        }
        case TokenKeyword::Else:
        case TokenKeyword::Then:
        {
            return std::unexpected{E::InvalidKeyword};
        }
        }
    }
    else
    {
        return std::unexpected{E::NoKeywordAtStart};
    }

    return std::unexpected{E::Generic};
}

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