// include/lexer.hpp
#include <expected>
#include <string_view>
#include <print>

#include "lexer.hpp"
#include "token.hpp"
#include "util.hpp"

namespace ds_lang
{
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

} // namespace ds_lang