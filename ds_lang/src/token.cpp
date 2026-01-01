#include "token.hpp"

#include <print>

namespace ds_lang
{
std::string token_to_string(const Token &tok)
{
    return std::visit([](const auto &t) -> std::string
        {
        using TT = std::decay_t<decltype(t)>;
        if constexpr (std::is_same_v<TT, BinaryOperator>) {
            return std::string{"BinaryOperator("} + std::string{to_string(t)} + ")";
        } else if constexpr (std::is_same_v<TT, TokenOperator>) {
            return std::string{"TokenOperator("} + std::string{to_string(t)} + ")";
        } else if constexpr (std::is_same_v<TT, TokenKeyword>) {
            return std::string{"Keyword("} + std::string{to_string(t)} + ")";
        } else if constexpr (std::is_same_v<TT, TokenInteger>) {
            return std::string{"Integer("} + std::to_string(t.value) + ")";
        } else if constexpr (std::is_same_v<TT, TokenIdentifier>) {
            return std::string{"Identifier(\""} + t.name + "\")";
        } else {
            return "UnknownToken";
        } }, tok);
}

void print_tokens(const std::vector<Token> &toks)
{
    std::print("[");
    for (size_t i = 0; i < toks.size(); ++i)
    {
        std::print("{}", token_to_string(toks[i]));
        if (i + 1 < toks.size()) std::print(", ");
    }
    std::println("]");
}

} // namespace ds_lang