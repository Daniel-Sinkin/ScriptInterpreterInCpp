
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

using i64 = int64_t;
using u64 = uint64_t;

std::string_view get_code()
{
    // String literals have static storage duration so this can return a view without issues
    return "let x = 1;\nprint x;\nlet x = 2;\nprint x;\nlet my_var = 123;\nprint my_var;\nlet my_var = x;\nlet y = if x > 5 then x + 1 else 0;\nlet z = x + y + 1;";
}

bool char_is_whitespace(char c)
{
    return std::isspace(static_cast<unsigned char>(c));
}

enum class BinaryArithmeticOperator
{
    Plus,  // +
    Minus, // -
    Star,  // *
    Slash, // /
};
enum class ComparisonOperator
{
    GreaterThan,      // >
    LessThan,         // <
    GreaterEqualThan, // >=
    LessEqualThan,    // <=
    DoubleEqual,      // ==
};
enum class TokenOperator
{
    Equal, // =
};

struct TokenIdentifier
{
    std::string name;
};

struct TokenInteger
{
    int64_t value;
};

enum class TokenKeyword
{
    Let,
    If,
    Then,
    Else,
    Print
};

using Token = std::variant<BinaryArithmeticOperator, ComparisonOperator, TokenOperator, TokenIdentifier, TokenInteger, TokenKeyword>;
using Tokens = std::vector<Token>;

constexpr bool char_is_digit(char c) noexcept
{
    return c >= '0' && c <= '9';
}

constexpr bool char_is_valid_for_identifier(char c) noexcept
{
    return (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z') ||
           c == '_';
}

bool is_valid_identifier(std::string_view s) noexcept
{
    return std::ranges::all_of(s, char_is_valid_for_identifier);
}

enum class StringToIntError
{
    Empty,
    InvalidDigit,
    Overflow,
    StartsWithZero
};
[[nodiscard]]
std::expected<i64, StringToIntError>
string_to_i64(std::string_view word) noexcept
{
    using E = StringToIntError;
    if (word.empty()) return std::unexpected{E::Empty};
    if (word.length() > 1 && word[0] == '0') return std::unexpected{E::StartsWithZero};
    i64 retval = 0;
    bool is_negative = false;
    for (size_t i = 0; i < word.size(); ++i)
    {
        char c = word[i];
        if (i == 0 && c == '-')
        {
            is_negative = true;
            continue;
        }

        if (!char_is_digit(c))
        {
            return std::unexpected{E::InvalidDigit};
        }
        retval = retval * 10 + static_cast<i64>(c - '0');
        if (i > 18) // if this is false then we are guaranteed to fit in i64, avoid having to do bounds check for now
        {
            return std::unexpected{E::Overflow};
        }
    }
    return {is_negative ? -retval : retval};
}

constexpr std::string_view to_string(BinaryArithmeticOperator op) noexcept
{
    switch (op)
    {
    case BinaryArithmeticOperator::Plus:
        return "Plus";
    case BinaryArithmeticOperator::Minus:
        return "Minus";
    case BinaryArithmeticOperator::Star:
        return "Star";
    case BinaryArithmeticOperator::Slash:
        return "Slash";
    }
    return "UnknownBinaryArithmeticOperator";
}

constexpr std::string_view to_string(TokenOperator op) noexcept
{
    switch (op)
    {
    case TokenOperator::Equal:
        return "Equal";
    }
    return "UnknownTokenOperator";
}

constexpr std::string_view to_string(ComparisonOperator op) noexcept
{
    switch (op)
    {
    case ComparisonOperator::GreaterThan:
        return "GreaterThan";
    case ComparisonOperator::LessThan:
        return "LessThan";
    case ComparisonOperator::GreaterEqualThan:
        return "GreaterEqualThan";
    case ComparisonOperator::LessEqualThan:
        return "LessEqualThan";
    case ComparisonOperator::DoubleEqual:
        return "DoubleEqual";
    }
    return "UnknownComparisonOperator";
}

constexpr std::string_view to_string(TokenKeyword kw) noexcept
{
    switch (kw)
    {
    case TokenKeyword::Let:
        return "Let";
    case TokenKeyword::If:
        return "If";
    case TokenKeyword::Then:
        return "Then";
    case TokenKeyword::Else:
        return "Else";
    case TokenKeyword::Print:
        return "Print";
    }
    return "UnknownKw";
}

static std::string token_to_string(const Token &tok)
{
    return std::visit([](const auto &t) -> std::string
        {
        using TT = std::decay_t<decltype(t)>;

        if constexpr (std::is_same_v<TT, BinaryArithmeticOperator>)
        {
            return std::string{"BinaryArithmeticOperator("} + std::string{to_string(t)} + ")";
        }
        else if constexpr (std::is_same_v<TT, TokenOperator>)
        {
            return std::string{"TokenOperator("} + std::string{to_string(t)} + ")";
        }
        else if constexpr (std::is_same_v<TT, ComparisonOperator>)
        {
            return std::string{"ComparisonOperator("} + std::string{to_string(t)} + ")";
        }
        else if constexpr (std::is_same_v<TT, TokenKeyword>)
        {
            return std::string{"Keyword("} + std::string{to_string(t)} + ")";
        }
        else if constexpr (std::is_same_v<TT, TokenInteger>)
        {
            return std::string{"Integer("} + std::to_string(t.value) + ")";
        }
        else if constexpr (std::is_same_v<TT, TokenIdentifier>)
        {
            return std::string{"Identifier(\""} + t.name + "\")";
        }
        else
        {
            return "UnknownToken";
        } }, tok);
}

static void print_tokens(const std::vector<Token> &toks)
{
    std::print("[");
    for (std::size_t i = 0; i < toks.size(); ++i)
    {
        std::print("{}", token_to_string(toks[i]));
        if (i + 1 < toks.size()) std::print(", ");
    }
    std::println("]");
}

struct Expression; // forward declaration
struct ValueExpression
{
    i64 value;
};

struct VariableExpression
{
    std::string name;
};

struct BinaryExpression
{
    BinaryArithmeticOperator op;
    std::unique_ptr<Expression> exp1;
    std::unique_ptr<Expression> exp2;
};
struct ComparisonExpression
{
    ComparisonOperator op;
    std::unique_ptr<Expression> exp1;
    std::unique_ptr<Expression> exp2;
};

struct Expression
{
    using ExpressionVariant = std::variant<ValueExpression, VariableExpression, BinaryExpression>;
    ExpressionVariant node;
};

class RuntimeContext
{
public:
    [[nodiscard]]
    std::optional<i64>
    variable_by_name(const std::string &name) const noexcept
    {
        auto it = m_variables.find(name);
        if (it == m_variables.end()) return std::nullopt;
        return it->second;
    }
    void set_variable(std::string name, i64 value)
    {
        m_variables.insert_or_assign(name, value);
    }

    void print()
    {
        std::println("RuntimeContext =");
        std::println("\t{{");

        bool first = true;
        for (const auto& [key, value] : m_variables)
        {
            if (!first)
            {
                std::println(",");
            }
            first = false;

            std::print("\t\t\"{}\": {}", key, value);
        }

        if (!first)
        {
            std::println();
        }

        std::println("\t}};");
    }
    private:
        std::unordered_map<std::string, i64> m_variables;
    };

enum class EvaluateExpressionError
{
    InvalidExpression, // Catch all
    UnsupportedOperator,
    MissingExpression,
    MissingVariable,
    DivisionByZero
};
[[nodiscard]]
std::expected<i64, EvaluateExpressionError>
evaluate_expression(const Expression &expr, const RuntimeContext &ctx)
{
    using E = EvaluateExpressionError;
    return std::visit([&ctx](const auto &e) -> std::expected<i64, EvaluateExpressionError>
        {
        using TT = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<TT, BinaryExpression>) {
            if(!e.exp1) return std::unexpected{E::MissingExpression};
            auto res1 = evaluate_expression(*e.exp1, ctx);
            if(!res1) {
                return std::unexpected{res1.error()};
            }
            if(!e.exp2) return std::unexpected{E::MissingExpression};
            auto res2 = evaluate_expression(*e.exp2, ctx);
            if(!res2) {
                return std::unexpected{res2.error()};
            }
            switch(e.op) { // Maybe should have seperate BinaryArithmeic and so on Operators
                case BinaryArithmeticOperator::Plus:
                    return *res1 + *res2;
                case BinaryArithmeticOperator::Minus:
                    return *res1 - *res2;
                case BinaryArithmeticOperator::Star:
                    return (*res1) * (*res2);
                case BinaryArithmeticOperator::Slash:
                    if (*res2 == 0) return std::unexpected{E::DivisionByZero};
                    return *res1 / *res2;
                default: 
                    return std::unexpected{E::UnsupportedOperator};
            }
        }
        else if constexpr (std::is_same_v<TT, ComparisonExpression>) {
            if(!e.exp1) return std::unexpected{E::MissingExpression};
            auto res1 = evaluate_expression(*e.exp1, ctx);
            if(!res1) {
                return std::unexpected{res1.error()};
            }
            if(!e.exp2) return std::unexpected{E::MissingExpression};
            auto res2 = evaluate_expression(*e.exp2, ctx);
            if(!res2) {
                return std::unexpected{res2.error()};
            }
            switch(e.op) { // Maybe should have seperate BinaryArithmeic and so on Operators
                case ComparisonOperator::GreaterThan:
                    return *res1 > *res2;
                case ComparisonOperator::LessThan:
                    return *res1 < *res2;
                case ComparisonOperator::GreaterEqualThan:
                    return *res1 >= *res2;
                case ComparisonOperator::LessEqualThan:
                    return *res1 <= *res2;
                case ComparisonOperator::DoubleEqual:
                    return *res1 == *res2;
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
            auto res = ctx.variable_by_name(e.name);
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
    FallThrough,
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
    case TokenizeWordError::FallThrough:
        return "FallThrough";
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
    case TokenizeWordError::FallThrough:
        return "Token could not be classified into any known category.";
    case TokenizeWordError::IdentifierIsNotAscii:
        return "Identifier contains non-ASCII alphabetic characters.";
    }
    return "Unknown token parsing error.";
}

[[nodiscard]]
std::expected<Token, TokenizeWordError>
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
            }
        }
        return TokenInteger{*res};
    }
    else
    {
        // clang-format off
        if      (word == "+"   ) return BinaryArithmeticOperator::Plus;
        else if (word == "-"   ) return BinaryArithmeticOperator::Minus;
        else if (word == "*"   ) return BinaryArithmeticOperator::Star;
        else if (word == "/"   ) return BinaryArithmeticOperator::Slash;
        else if (word == "="   ) return TokenOperator::Equal;
        else if (word == "=="  ) return ComparisonOperator::DoubleEqual;
        else if (word == "<"   ) return ComparisonOperator::LessThan;
        else if (word == ">"   ) return ComparisonOperator::GreaterThan;
        else if (word == ">="  ) return ComparisonOperator::GreaterEqualThan;
        else if (word == "<="  ) return ComparisonOperator::LessEqualThan;
        else if (word == "let" ) return TokenKeyword::Let;
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
    assert(false && "Fallthrough in parse_token");
}

struct TokenizeNextStatementError
{
    TokenizeWordError tokenize_error;
    size_t word_idx;
    size_t word_length;
};
[[nodiscard]]
std::expected<Tokens, TokenizeNextStatementError>
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

enum class ParseExpressionError
{
    Generic,
    MalformedExpression,
    EmptyStatement,
    NotImplemented
};
[[nodiscard]]
std::expected<Expression, ParseExpressionError>
parse_expression(std::span<const Token> tokens)
{
    using E = ParseExpressionError;
    if (tokens.empty()) return std::unexpected{E::EmptyStatement};
    if (tokens.size() > 1) return std::unexpected{E::NotImplemented};

    auto tvar = std::get_if<TokenIdentifier>(&tokens[0]);
    if(tvar)
    {
        return Expression{.node = VariableExpression{.name = tvar->name}};
    }

    auto tint = std::get_if<TokenInteger>(&tokens[0]);
    if (tint)
    {
        return Expression{.node = ValueExpression{.value = tint->value}};
    }
    return std::unexpected{E::MalformedExpression};
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
        case TokenKeyword::Let:
        {
            if (tokens.size() < 4) return std::unexpected{E::InvalidLength};
            auto var_name = std::get_if<TokenIdentifier>(&tokens[1]);
            auto equals_op = std::get_if<TokenOperator>(&tokens[2]);
            if (!var_name || !equals_op || *equals_op != TokenOperator::Equal)
            {
                return std::unexpected{E::MalformedStatement};
            }
            if (tokens.size() > 4) return std::unexpected{E::NotImplemented};
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

std::vector<Tokens> tokenize_code(std::string_view code)
{
    std::vector<Tokens> statement_tokens;
    size_t start_idx = 0;
    while (start_idx < code.length()) // Each iteration is one statement, seperated by ';', whitespace trimmed
    {
        auto res = tokenize_next_statement(code, start_idx);
        if (!res)
        {
            std::println("Skipping Statement as we failed to parse statement, get parse error code {}",
                static_cast<int>(res.error().tokenize_error));
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
[[nodiscard]]
std::expected<void, ExecuteStatementError>
execute_statement(RuntimeContext &ctx, const Statement &statement)
{
    using E = ExecuteStatementError;
    return std::visit([&ctx](const auto &s) -> std::expected<void, ExecuteStatementError>
        {
        using TT = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<TT, AssignmentStatement>) {
            auto res = evaluate_expression(s.expr, ctx);
            if(!res) {
                return std::unexpected{E::ExpressionError};
            }
            ctx.set_variable(s.identifier, *res);
        }
        else if constexpr (std::is_same_v<TT, PrintStatement>) {
            auto res = evaluate_expression(s.expr, ctx);
            if(!res) {
                return std::unexpected{E::ExpressionError};
            }
            std::println("Interpreter Printout: '{}'", *res);
        }
        return {}; }, statement);
}

int main()
{
    std::string_view code = get_code();
    std::println("The code:\n{}\n", code);

    std::vector<Tokens> statement_tokens = tokenize_code(code);

    std::println("Found {} statements:", statement_tokens.size());
    for (size_t i = 0; i < statement_tokens.size(); ++i)
    {
        std::print("Statement[{:2}] = ", i);
        print_tokens(statement_tokens[i]);
    }
    std::println();

    RuntimeContext ctx;
    std::println("Initialised Runtime");
    ctx.print();
    for (size_t i = 0; i < 7; ++i) {
        std::print("Executing the {}. statement: ", i + 1);
        print_tokens(statement_tokens[i]);
        if (!execute_statement(ctx, *parse_statement(statement_tokens[i]))) return 1;
        ctx.print();
        std::println();
    }
}