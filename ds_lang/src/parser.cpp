// ds_lang/src/parser.cpp
#include "parser.hpp"

#include <cassert>
#include <memory>
#include <optional>
#include <variant>

namespace ds_lang
{
struct Parser
{
    std::span<const Token> toks;
    size_t pos = 0;

    struct BpPair
    {
        int left;
        int right;
    };

    [[nodiscard]] bool eof() const noexcept { return pos >= toks.size(); }
    [[nodiscard]] const Token *peek() const noexcept { return eof() ? nullptr : &toks[pos]; }

    [[nodiscard]] const Token &consume()
    {
        assert(!eof());
        return toks[pos++];
    }

    [[nodiscard]] static std::optional<BpPair> infix_bp(const Token &tok) noexcept
    {
        if (auto bop = std::get_if<BinaryOperator>(&tok))
        {
            switch (*bop)
            {
            case BinaryOperator::Star:
            case BinaryOperator::Slash:
                return BpPair{70, 71};

            case BinaryOperator::Plus:
            case BinaryOperator::Minus:
                return BpPair{60, 61};

            case BinaryOperator::GreaterThan:
            case BinaryOperator::LessThan:
            case BinaryOperator::GreaterEqualThan:
            case BinaryOperator::LessEqualThan:
                return BpPair{50, 51};

            case BinaryOperator::DoubleEqual:
                return BpPair{40, 41};
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] std::expected<Expression, ParseExpressionError> parse_expr(int min_bp = 0)
    {
        using E = ParseExpressionError;

        if (eof()) return std::unexpected{E::MalformedExpression};

        Expression lhs;
        {
            const Token &t = consume();

            if (auto ti = std::get_if<TokenInteger>(&t))
            {
                lhs.node = ValueExpression{.value = ti->value};
            }
            else if (auto tv = std::get_if<TokenIdentifier>(&t))
            {
                lhs.node = VariableExpression{.name = tv->name};
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

            const auto bp = infix_bp(*next);
            if (!bp || bp->left < min_bp) break;

            const BinaryOperator op = std::get<BinaryOperator>(consume());

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

std::expected<Expression, ParseExpressionError>
parse_expression(std::span<const Token> tokens)
{
    using E = ParseExpressionError;

    if (tokens.empty()) return std::unexpected{E::EmptyTokens};

    Parser p{.toks = tokens, .pos = 0};
    auto expr = p.parse_expr(0);
    if (!expr) return expr;

    if (p.pos != tokens.size())
        return std::unexpected{E::MalformedExpression};

    return expr;
}

std::expected<Statement, ParseStatementError>
parse_statement(const Tokens &tokens)
{
    using E = ParseStatementError;

    if (tokens.empty()) return std::unexpected{E::EmptyStatement};

    const auto *kw = std::get_if<TokenKeyword>(&tokens[0]);
    if (!kw) return std::unexpected{E::NoKeywordAtStart};

    switch (*kw)
    {
    case TokenKeyword::Int:
    {
        if (tokens.size() < 4) return std::unexpected{E::InvalidLength};

        const auto *var_name = std::get_if<TokenIdentifier>(&tokens[1]);
        const auto *equals_op = std::get_if<TokenOperator>(&tokens[2]);

        if (!var_name || !equals_op || *equals_op != TokenOperator::Equal)
            return std::unexpected{E::MalformedStatement};

        auto expr = parse_expression(std::span{tokens}.subspan(3));
        if (!expr) return std::unexpected{E::MalformedExpression};

        return AssignmentStatement{.identifier = var_name->name, .expr = std::move(*expr)};
    }

    case TokenKeyword::Print:
    {
        if (tokens.size() < 2) return std::unexpected{E::InvalidLength};

        auto expr = parse_expression(std::span{tokens}.subspan(1));
        if (!expr) return std::unexpected{E::MalformedExpression};

        return PrintStatement{.expr = std::move(*expr)};
    }

    case TokenKeyword::If:
    {
        size_t if_expr_idx = 1;
        size_t then_expr_idx = 0;
        size_t else_expr_idx = 0;
        for (size_t i = 2; i < tokens.size(); ++i)
        {
            auto kw_res = std::get_if<TokenKeyword>(&tokens[i]);
            if (!kw_res) continue;
            if (*kw_res == TokenKeyword::Then)
            {
                if (then_expr_idx != 0)
                {
                    return std::unexpected{E::MalformedExpression};
                }
                then_expr_idx = i;
            }
            else if (*kw_res == TokenKeyword::Else)
            {
                if (else_expr_idx != 0)
                {
                    return std::unexpected{E::MalformedExpression};
                }
                else_expr_idx = i;
            }

            if (then_expr_idx != 0 && else_expr_idx != 0) break;
        }

        if (then_expr_idx == 0 && else_expr_idx == 0)
        {
            return std::unexpected{E::MalformedExpression};
        }

        auto if_expr_res = parse_expression(std::span{tokens}.subspan(if_expr_idx, then_expr_idx - if_expr_idx));
        if (!if_expr_res)
        {
            return std::unexpected{E::MalformedExpression};
        }
        auto then_expr_res = parse_expression(std::span{tokens}.subspan(then_expr_idx, else_expr_idx - then_expr_idx));
        if (!then_expr_res)
        {
            return std::unexpected{E::MalformedExpression};
        }
        auto else_expr_res = parse_expression(std::span{tokens}.subspan(else_expr_idx));
        if (!else_expr_res)
        {
            return std::unexpected{E::MalformedExpression};
        }
    }

    case TokenKeyword::Then:
    case TokenKeyword::Else:
        return std::unexpected{E::InvalidKeyword};
    }

    return std::unexpected{E::Generic};
}

} // namespace ds_lang