
#include <cassert>
#include <expected>
#include <print>
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>
#include <string_view> 
#include <functional> 
#include <optional>

using i64 = int64_t;
using u64 = uint64_t;

std::string get_code()
{
    return "let x = 10;\nif x > 5 then x + 1 else 0;";
}

bool is_whitespace(char c)
{
    return std::isspace(static_cast<unsigned char>(c));
}

enum class TokenOperator
{
    Plus,  // +
    Minus, // -
    Star,  // *
    Slash  // /
};

struct TokenIdentifier
{
    std::string name;
};

struct TokenInteger
{
    int64_t value;
};

enum class TokenKeyword {
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

enum class StringToIntError
{
    Empty,
    InvalidDigit,
    Overflow,
    StartsWithZero // Maybe we drop those later and don't increase power later, but strict is good for now
};
std::expected<i64, StringToIntError> string_to_i64(const std::string &word) noexcept
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
        case TokenOperator::Plus:  return "Plus";
        case TokenOperator::Minus: return "Minus";
        case TokenOperator::Star:  return "Star";
        case TokenOperator::Slash: return "Slash";
    }
    return "UnknownOp";
}

constexpr std::string_view to_string(TokenKeyword kw) noexcept
{
    switch (kw)
    {
        case TokenKeyword::Let:  return "Let";
        case TokenKeyword::If:   return "If";
        case TokenKeyword::Then: return "Then";
        case TokenKeyword::Else: return "Else";
    }
    return "UnknownKw";
}

static std::string token_to_string(const Token& tok)
{
    return std::visit([](const auto& t) -> std::string {
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
        }
    }, tok);
}

static void print_tokens(const std::vector<Token>& toks)
{
    std::print("[");
    for (std::size_t i = 0; i < toks.size(); ++i)
    {
        std::print("{}", token_to_string(toks[i]));
        if (i + 1 < toks.size()) std::print(", ");
    }
    std::println("]");
}

static void print_expressions(const std::vector<std::vector<Token>>& exprs)
{
    for (std::size_t i = 0; i < exprs.size(); ++i)
    {
        std::print("expr[{}] = ", i);
        print_tokens(exprs[i]);
    }
}

class RuntimeContext {
public:
    [[nodiscard]]
    std::optional<i64>
    variable_by_name(const std::string& name) const noexcept {
        auto it = m_variables.find(name);
        if(it == m_variables.end()) return std::nullopt;
        return it->second;
    }
    void set_variable(std::string name, i64 value) {
        m_variables.insert_or_assign(name, value);
    }
private:
    std::unordered_map<std::string, i64> m_variables; // Later this should be a map into value types
};

int main()
{
    using ExpressionTokens = std::vector<Token>;

    std::string code = get_code();
    std::println("The code:\n{}\n", code);

    std::vector<ExpressionTokens> expressions;
    std::vector<std::vector<std::string>> expression_words;
    // TODO: Prolly can just have views / ranges instead of copying?
    {
        std::vector<std::string> words;
        std::vector<Token> tokens;
        std::println("Starting to parse words:");
        size_t start_idx = 0;
        while (start_idx < code.length()) // Each iteration is one word, whitespace trimmed
        {
            while (start_idx < code.length() && is_whitespace(code[start_idx]))
            {
                ++start_idx;
            }
            if (start_idx >= code.length()) break;
            size_t curr_idx = start_idx;

            [[maybe_unused]] bool found_semicolon = false;
            while (curr_idx < code.length())
            {
                char c = code[curr_idx];
                if (is_whitespace(c))
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
            assert(curr_idx > start_idx);
            auto word_diff = curr_idx - start_idx;
            auto word = code.substr(start_idx, word_diff);

            if (word.empty())
            {
                std::println("Can't parse empty words!");
                return 1;
            }
            Token token;
            if (word.length() == 1) // This must be an operator or a single positive digit
            {
                char c = word[0];
                if(char_is_digit(c)) {
                    token = TokenInteger{c - '0'};
                } else {
                    switch(c) {
                        case '+':
                            token = TokenOperator::Plus;
                            break;
                        case '-':
                            token = TokenOperator::Minus;
                            break;
                        case '*':
                            token = TokenOperator::Star;
                            break;
                        case '/':
                            token = TokenOperator::Slash;
                            break;
                        default:
                            token = TokenIdentifier{word};
                            break;
                    }
                }
            }
            else if (word[0] == '-' || (char_is_digit(word[0])))
            {
                auto res = string_to_i64(word);
                if (!res)
                {
                    std::println("Failed to parse integer {}!", word);
                    return 1;
                }
                token = TokenInteger{res.value()};
            }
            else // Keyword or Identifier
            {
                if(word == "let") {
                    token = TokenKeyword::Let;
                } else if (word == "if") {
                    token = TokenKeyword::If;
                } else if (word == "else") {
                    token = TokenKeyword::Else;
                } else if (word == "then") {
                    token = TokenKeyword::Then;
                } else { // Identifer
                    token = TokenIdentifier{word};
                }
            }

            tokens.push_back(token);
            words.push_back(word);

            if (found_semicolon)
            {
                ++curr_idx;
                expressions.push_back(std::move(tokens));
                expression_words.push_back(std::move(words));
                tokens = {};
                words = {};
            }
            start_idx = curr_idx;
        }
    }
    std::println("Finished parsing words!\n");

    std::println("Found {} expressions:", expressions.size());
    print_expressions(expressions);

    std::println("\nParsing results:");
    for (size_t expr_id = 0; expr_id < expressions.size(); ++expr_id)
    {
        const auto& words = expression_words[expr_id];
        const auto& toks  = expressions[expr_id];

        for (size_t tid = 0; tid < words.size(); ++tid)
        {
            std::println("  {:10} -> {}", words[tid], token_to_string(toks[tid]));
        }
    }
}