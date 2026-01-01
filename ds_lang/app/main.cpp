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

namespace ds_lang
{

enum class EvaluateExpressionError
{
    InvalidExpression, // Catch all
    UnsupportedOperator,
    MissingExpression,
    MissingVariable,
    DivisionByZero
};
[[nodiscard]] std::expected<i64, EvaluateExpressionError>
evaluate_expression(const Expression &expr, const Environment &env)
{
    using E = EvaluateExpressionError;
    return std::visit([&env](const auto &e) -> std::expected<i64, EvaluateExpressionError>
        {
        using TT = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<TT, BinaryExpression>) {
            if(!e.exp1) return std::unexpected{E::MissingExpression};
            auto res1 = evaluate_expression(*e.exp1, env);
            if(!res1) {
                return std::unexpected{res1.error()};
            }
            if(!e.exp2) return std::unexpected{E::MissingExpression};
            auto res2 = evaluate_expression(*e.exp2, env);
            if(!res2) {
                return std::unexpected{res2.error()};
            }
            switch(e.op) { // Maybe should have seperate BinaryArithmeic and so on Operators
                case BinaryOperator::Plus:
                    return *res1 + *res2;
                case BinaryOperator::Minus:
                    return *res1 - *res2;
                case BinaryOperator::Star:
                    return (*res1) * (*res2);
                case BinaryOperator::Slash:
                    if (*res2 == 0) return std::unexpected{E::DivisionByZero};
                    return *res1 / *res2;
                case BinaryOperator::GreaterThan:
                    return static_cast<i64>(*res1 > *res2);
                case BinaryOperator::LessThan:
                    return static_cast<i64>(*res1 < *res2);
                case BinaryOperator::GreaterEqualThan:
                    return static_cast<i64>(*res1 >= *res2);
                case BinaryOperator::LessEqualThan:
                    return static_cast<i64>(*res1 <= *res2);
                case BinaryOperator::DoubleEqual:
                    return static_cast<i64>(*res1 == *res2);
                default: 
                    return std::unexpected{E::UnsupportedOperator};
            }
        }
        else if constexpr (std::is_same_v<TT, ValueExpression>)
        {
            return e.value;
        }
        else if constexpr (std::is_same_v<TT, VariableExpression>)
        {
            auto res = env.get(e.name);
            if(!res.has_value()) return std::unexpected{E::MissingVariable};
            return *res;
        } }, expr.node);
}

enum class TokenizeWordError
{
    Empty,
    InvalidIntegerDigit,
    IntegerOverflow,
    StartsWithZero,
    IdentifierIsNotAscii
};
constexpr std::string_view to_string(TokenizeWordError e) noexcept
{
    switch (e)
    {
    case TokenizeWordError::Empty:
        return "Empty";
    case TokenizeWordError::InvalidIntegerDigit:
        return "InvalidIntegerDigit";
    case TokenizeWordError::IntegerOverflow:
        return "IntegerOverflow";
    case TokenizeWordError::StartsWithZero:
        return "StartsWithZero";
    case TokenizeWordError::IdentifierIsNotAscii:
        return "IdentifierIsNotAscii";
    }
    return "UnknownParseTokenError";
}

constexpr std::string_view explain(TokenizeWordError e) noexcept
{
    switch (e)
    {
    case TokenizeWordError::Empty:
        return "Encountered an empty token where input was expected.";
    case TokenizeWordError::InvalidIntegerDigit:
        return "Integer literal contains a non-digit character.";
    case TokenizeWordError::IntegerOverflow:
        return "Integer literal is too large to fit into a 64-bit signed integer.";
    case TokenizeWordError::StartsWithZero:
        return "Integer literal has a leading zero, which is not allowed.";
    case TokenizeWordError::IdentifierIsNotAscii:
        return "Identifier contains non-ASCII alphabetic characters.";
    }
    return "Unknown token parsing error.";
}

[[nodiscard]] std::expected<Token, TokenizeWordError>
tokenize_word(std::string_view word)
{
    using E = TokenizeWordError;
    if (word.empty()) return std::unexpected{E::Empty};
    if ((word[0] == '-' && word.length() > 1) || (char_is_digit(word[0])))
    {
        auto res = string_to_i64(word);
        if (!res)
        {
            using EE = StringToIntError;
            switch (res.error())
            {
            case EE::Overflow:
                return std::unexpected{E::IntegerOverflow};
            case EE::StartsWithZero:
                return std::unexpected{E::StartsWithZero};
            case EE::InvalidDigit:
                return std::unexpected{E::InvalidIntegerDigit};
            case EE::Empty:
                assert(false && "'EmptyError' in inner token parse should not happen.");
                std::unreachable();
            }
        }
        return TokenInteger{*res};
    }
    else
    {
        // clang-format off
        if      (word == "+"   ) return BinaryOperator::Plus;
        else if (word == "-"   ) return BinaryOperator::Minus;
        else if (word == "*"   ) return BinaryOperator::Star;
        else if (word == "/"   ) return BinaryOperator::Slash;
        else if (word == "=="  ) return BinaryOperator::DoubleEqual;
        else if (word == "<"   ) return BinaryOperator::LessThan;
        else if (word == ">"   ) return BinaryOperator::GreaterThan;
        else if (word == ">="  ) return BinaryOperator::GreaterEqualThan;
        else if (word == "<="  ) return BinaryOperator::LessEqualThan;
        else if (word == "="   ) return TokenOperator::Equal;
        else if (word == "int" ) return TokenKeyword::Int;
        else if (word == "if"  ) return TokenKeyword::If;
        else if (word == "else") return TokenKeyword::Else;
        else if (word == "then") return TokenKeyword::Then;
        else if (word == "print") return TokenKeyword::Print;
        else
        { // Identifer
            if (!is_valid_identifier(word)) return std::unexpected{E::IdentifierIsNotAscii};
            return TokenIdentifier{std::string{word}};
        }
        // clang-format on
    }
    std::unreachable();
}

struct TokenizeNextStatementError
{
    TokenizeWordError tokenize_error;
    size_t word_idx;
    size_t word_length;
};
[[nodiscard]] std::expected<Tokens, TokenizeNextStatementError>
tokenize_next_statement(std::string_view code, size_t &idx)
{
    Tokens statement;
    while (idx < code.length() && char_is_whitespace(code[idx]))
    {
        ++idx;
    }
    if (idx >= code.length())
    {
        // Empty Statement list is to be understood as "to skip"
        // Could also treat that as an error case to seperate EOF with ";;"
        return statement;
    }

    while (idx < code.length())
    {
        while (idx < code.length() && char_is_whitespace(code[idx]))
        { // Skips whitespace
            ++idx;
        }
        if (idx >= code.length()) break;

        size_t curr_idx = idx;
        bool found_semicolon = false;

        while (curr_idx < code.length())
        { // Consume chars until hitting whitespace or ';'
            char c = code[curr_idx];
            if (char_is_whitespace(c))
            {
                break;
            }
            else if (c == ';')
            {
                found_semicolon = true;
                break;
            }
            ++curr_idx;
        }

        assert(curr_idx > idx);
        const size_t word_diff = curr_idx - idx;
        const auto word = code.substr(idx, word_diff);

        auto res = tokenize_word(word);
        if (!res)
        {
            return std::unexpected(TokenizeNextStatementError{
                .tokenize_error = res.error(),
                .word_idx = idx,
                .word_length = word_diff});
        }
        statement.push_back(*res);

        idx = curr_idx;
        if (found_semicolon)
        {
            ++idx; // Skips the ';'
            return statement;
        }
    }

    return statement;
}

struct AssignmentStatement
{
    std::string identifier;
    Expression expr;
};

struct PrintStatement
{
    Expression expr;
};

// Statements either don't do anything, have side effects on the context (set a variable)
// or side effects outside of the context (how function calling is handled idk, but things like printing to console)
using Statement = std::variant<AssignmentStatement, PrintStatement>;

std::vector<Tokens> tokenize_code(std::string_view code)
{
    std::vector<Tokens> statement_tokens;
    size_t start_idx = 0;
    while (start_idx < code.length()) // Each iteration is one statement, seperated by ';', whitespace trimmed
    {
        size_t initial_start_idx = start_idx;
        auto res = tokenize_next_statement(code, start_idx);
        if (!res)
        {
            TokenizeNextStatementError err = res.error();
            TokenizeWordError err_code = err.tokenize_error;
            std::println("Lexer failed, the following statement is misformed:");
            size_t print_start = initial_start_idx;
            size_t print_width = 1;
            while (print_start + print_width < code.size())
            {
                char current_char = code[print_start + print_width];
                if (current_char == '\n' || current_char == ';')
                {
                    break;
                }
                ++print_width;
            }

            std::println("{}", code.substr(print_start, print_width));
            for (size_t i = 0; i < err.word_idx - print_start - 1; ++i)
            {
                std::print(" ");
            }
            std::println("^");
            for (size_t i = 0; i < err.word_idx - print_start - 1; ++i)
            {
                std::print(" ");
            }
            std::println("{} [{}]", explain(err_code), to_string(err_code));
            assert(false && "Failed to tokenize statement");
        }
        else
        {
            statement_tokens.push_back(*res);
        }
    }
    return statement_tokens;
}

enum class ExecuteStatementError
{
    Generic,
    ExpressionError
};
[[nodiscard]] std::expected<void, ExecuteStatementError>
execute_statement(Environment &env, const Statement &statement)
{
    using E = ExecuteStatementError;
    return std::visit([&env](const auto &s) -> std::expected<void, ExecuteStatementError>
        {
        using TT = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<TT, AssignmentStatement>) {
            auto res = evaluate_expression(s.expr, env);
            if(!res) {
                return std::unexpected{E::ExpressionError};
            }
            env.set(s.identifier, *res);
        }
        else if constexpr (std::is_same_v<TT, PrintStatement>) {
            auto res = evaluate_expression(s.expr, env);
            if(!res) {
                return std::unexpected{E::ExpressionError};
            }
            std::println("Interpreter Printout: '{}'", *res);
        }
        return {}; }, statement);
}

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