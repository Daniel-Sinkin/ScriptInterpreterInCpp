// ds_lang/include/parser.hpp
#pragma once

#include <expected>
#include <span>

#include "ast.hpp"
#include "token.hpp"
#include "interpreter.hpp"

namespace ds_lang {

enum class ParseExpressionError
{
    MalformedExpression,
    EmptyTokens,
    NotImplemented,
};

enum class ParseStatementError
{
    NoKeywordAtStart,
    MalformedStatement,
    MalformedExpression,
    InvalidKeyword,
    EmptyStatement,
    InvalidLength,
    NotImplementedKeyword,
    Generic,
};

[[nodiscard]]
std::expected<Expression, ParseExpressionError>
parse_expression(std::span<const Token> tokens);

[[nodiscard]]
std::expected<Statement, ParseStatementError>
parse_statement(const Tokens& tokens);
} // namespace ds_lang