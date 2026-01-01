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

constexpr std::string_view to_string(ParseExpressionError e) noexcept
{
    switch (e)
    {
    case ParseExpressionError::MalformedExpression:
        return "MalformedExpression";
    case ParseExpressionError::EmptyTokens:
        return "EmptyTokens";
    case ParseExpressionError::NotImplemented:
        return "NotImplemented";
    }
    return "UnknownParseExpressionError";
}

constexpr std::string_view explain(ParseExpressionError e) noexcept
{
    switch (e)
    {
    case ParseExpressionError::MalformedExpression:
        return "The token sequence does not form a valid expression.";
    case ParseExpressionError::EmptyTokens:
        return "An expression was expected but no tokens were provided.";
    case ParseExpressionError::NotImplemented:
        return "This expression form is recognized but not yet implemented.";
    }
    return "Unknown expression parsing error.";
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
    Generic,
};

constexpr std::string_view to_string(ParseStatementError e) noexcept
{
    switch (e)
    {
    case ParseStatementError::NoKeywordAtStart:
        return "NoKeywordAtStart";
    case ParseStatementError::MalformedStatement:
        return "MalformedStatement";
    case ParseStatementError::MalformedExpression:
        return "MalformedExpression";
    case ParseStatementError::InvalidKeyword:
        return "InvalidKeyword";
    case ParseStatementError::EmptyStatement:
        return "EmptyStatement";
    case ParseStatementError::InvalidLength:
        return "InvalidLength";
    case ParseStatementError::NotImplementedKeyword:
        return "NotImplementedKeyword";
    case ParseStatementError::Generic:
        return "Generic";
    }
    return "UnknownParseStatementError";
}

constexpr std::string_view explain(ParseStatementError e) noexcept
{
    switch (e)
    {
    case ParseStatementError::NoKeywordAtStart:
        return "A statement must begin with a valid keyword.";
    case ParseStatementError::MalformedStatement:
        return "The token sequence does not form a valid statement.";
    case ParseStatementError::MalformedExpression:
        return "The statement contains an invalid or malformed expression.";
    case ParseStatementError::InvalidKeyword:
        return "The keyword is not valid in this context.";
    case ParseStatementError::EmptyStatement:
        return "An empty statement was encountered.";
    case ParseStatementError::InvalidLength:
        return "The statement has an invalid number of tokens.";
    case ParseStatementError::NotImplementedKeyword:
        return "The keyword is recognized but not yet implemented.";
    case ParseStatementError::Generic:
        return "A generic parsing error occurred.";
    }
    return "Unknown statement parsing error.";
}


[[nodiscard]]
std::expected<Expression, ParseExpressionError>
parse_expression(std::span<const Token> tokens);

[[nodiscard]]
std::expected<Statement, ParseStatementError>
parse_statement(const Tokens& tokens);
} // namespace ds_lang