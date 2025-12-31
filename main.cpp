
#include <algorithm>
#include <cassert>
#include <expected>
#include <functional>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

using i64 = int64_t;
using u64 = uint64_t;

std::string_view get_code()
{
    // String literals have static storage duration so this can return a view without issues
    return "let x = 10;\nif x > 5 then x + 1 else 0;";
}

bool char_is_whitespace(char c)
{
    return std::isspace(static_cast<unsigned char>(c));
}

enum class TokenOperator
{
    Plus,             // +
    Minus,            // -
    Star,             // *
    Slash,            // /
    Equal,            // =
    GreaterThan,      // >
    LessThan,         // <
    GreaterEqualThan, // >=
    LessEqualThan,    // <=
    DoubleEqual,      // ==
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
    Else
};

using Token = std::variant<TokenOperator, TokenIdentifier, TokenInteger, TokenKeyword>;

constexpr bool char_is_digit(char c) noexcept
{
    return c >= '0' && c <= '9';
}

constexpr bool char_is_ascii(char c) noexcept
{
    return (c >= 'A' && c <= 'Z') ||
           (c >= 'a' && c <= 'z');
}

bool word_is_ascii(std::string_view s) noexcept
{
    return std::ranges::all_of(s, char_is_ascii);
}

enum class StringToIntError
{
    Empty,
    InvalidDigit,
    Overflow,
    StartsWithZero // Maybe we drop those later and don't increase power later, but strict is good for now
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

constexpr std::string_view to_string(TokenOperator op) noexcept
{
    switch (op)
    {
    case TokenOperator::Plus:
        return "Plus";
    case TokenOperator::Minus:
        return "Minus";
    case TokenOperator::Star:
        return "Star";
    case TokenOperator::Slash:
        return "Slash";
    case TokenOperator::Equal:
        return "Equal";
    case TokenOperator::GreaterThan:
        return "GreaterThan";
    case TokenOperator::LessThan:
        return "LessThan";
    case TokenOperator::GreaterEqualThan:
        return "GreaterEqualThan";
    case TokenOperator::LessEqualThan:
        return "LessEqualThan";
    case TokenOperator::DoubleEqual:
        return "DoubleEqual";
    }
    return "UnknownOp";
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
    }
    return "UnknownKw";
}

static std::string token_to_string(const Token &tok)
{
    return std::visit([](const auto &t) -> std::string
        {
        using TT = std::decay_t<decltype(t)>;

        if constexpr (std::is_same_v<TT, TokenOperator>)
        {
            return std::string{"Operator("} + std::string{to_string(t)} + ")";
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

struct AssignmentExpression
{
    std::string identifier;
    i64 value;
};
using Expression = std::variant<AssignmentExpression>;

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
    void process_expression_assignment(AssignmentExpression expr)
    { // TODO: Use visitor pattern
        set_variable(expr.identifier, expr.value);
    }

private:
    std::unordered_map<std::string, i64> m_variables; // Later this should be a map into value types
    void set_variable(std::string name, i64 value)
    {
        m_variables.insert_or_assign(name, value);
    }
};

enum class ParseTokenError
{
    Empty,
    InvalidIntegerDigit,
    IntegerOverflow,
    StartsWithZero,
    FallThrough,
    IdentifierIsNotAscii
};
constexpr std::string_view to_string(ParseTokenError e) noexcept
{
    switch (e)
    {
    case ParseTokenError::Empty:
        return "Empty";
    case ParseTokenError::InvalidIntegerDigit:
        return "InvalidIntegerDigit";
    case ParseTokenError::IntegerOverflow:
        return "IntegerOverflow";
    case ParseTokenError::StartsWithZero:
        return "StartsWithZero";
    case ParseTokenError::FallThrough:
        return "FallThrough";
    case ParseTokenError::IdentifierIsNotAscii:
        return "IdentifierIsNotAscii";
    }
    return "UnknownParseTokenError";
}

constexpr std::string_view explain(ParseTokenError e) noexcept
{
    switch (e)
    {
    case ParseTokenError::Empty:
        return "Encountered an empty token where input was expected.";
    case ParseTokenError::InvalidIntegerDigit:
        return "Integer literal contains a non-digit character.";
    case ParseTokenError::IntegerOverflow:
        return "Integer literal is too large to fit into a 64-bit signed integer.";
    case ParseTokenError::StartsWithZero:
        return "Integer literal has a leading zero, which is not allowed.";
    case ParseTokenError::FallThrough:
        return "Token could not be classified into any known category.";
    case ParseTokenError::IdentifierIsNotAscii:
        return "Identifier contains non-ASCII alphabetic characters.";
    }
    return "Unknown token parsing error.";
}

[[nodiscard]]
std::expected<Token, ParseTokenError>
parse_token(std::string_view word)
{
    using E = ParseTokenError;
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
                assert(false && "Empty error in inner token parse should not happen.");
            }
        }
        return TokenInteger{res.value()};
    }
    else
    {
        // clang-format off
        if      (word == "+"   ) return TokenOperator::Plus;
        else if (word == "-"   ) return TokenOperator::Minus;
        else if (word == "*"   ) return TokenOperator::Star;
        else if (word == "/"   ) return TokenOperator::Slash;
        else if (word == "="   ) return TokenOperator::Equal;
        else if (word == "=="  ) return TokenOperator::DoubleEqual;
        else if (word == "<"   ) return TokenOperator::LessThan;
        else if (word == ">"   ) return TokenOperator::GreaterThan;
        else if (word == ">="  ) return TokenOperator::GreaterEqualThan;
        else if (word == "<="  ) return TokenOperator::LessEqualThan;
        else if (word == "let" ) return TokenKeyword::Let;
        else if (word == "if"  ) return TokenKeyword::If;
        else if (word == "else") return TokenKeyword::Else;
        else if (word == "then") return TokenKeyword::Then;
        else
        { // Identifer
            if (!word_is_ascii(word)) return std::unexpected{E::IdentifierIsNotAscii};
            return TokenIdentifier{std::string{word}};
        }
        // clang-format on
    }
    assert(false && "Fallthrough in parse_token");
}

struct ParseNextExpressionError
{
    ParseTokenError parsing_error;
    size_t word_idx;
    size_t word_length;
};
[[nodiscard]]
std::expected<std::vector<Token>, ParseNextExpressionError>
parse_next_expression(std::string_view code, size_t &idx)
{
    std::vector<Token> tokens; // <-- fixed

    while (idx < code.length() && char_is_whitespace(code[idx]))
    {
        ++idx;
    }
    if (idx >= code.length())
    {
        // Empty Expression list is to be understood as "to skip"
        // Could also treat that as an error case to seperate EOF with ";;"
        return tokens;
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

        auto res = parse_token(word);
        if (!res)
        {
            return std::unexpected(ParseNextExpressionError{
                .parsing_error = res.error(),
                .word_idx = idx,
                .word_length = word_diff});
        }

        tokens.push_back(res.value());

        idx = curr_idx;
        if (found_semicolon)
        {
            ++idx; // Skips the ';'
            return tokens;
        }
    }

    return tokens;
}

int main()
{
    using ExpressionTokens = std::vector<Token>;

    std::string_view code = get_code();
    std::println("The code:\n{}\n", code);

    std::vector<ExpressionTokens> expressions;
    size_t start_idx = 0;
    while (start_idx < code.length()) // Each iteration is one word, whitespace trimmed
    {
        auto res = parse_next_expression(code, start_idx);
        if (!res)
        {
            std::println("Failed to parse expression, get parse error code {}",
                static_cast<int>(res.error().parsing_error));
            return 1;
        }
        expressions.push_back(res.value());
    }
    std::println("Finished parsing words!\n");

    std::println("Found {} expressions:", expressions.size());
    for (size_t i = 0; i < expressions.size(); ++i)
    {
        std::print("Expr[{:2}] = ", i);
        print_tokens(expressions[i]);
    }
}