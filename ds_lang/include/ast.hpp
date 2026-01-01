// ds_lang/include/ast.hpp
#pragma once

#include <string>
#include <variant>
#include <memory>

#include "types.hpp"
#include "token.hpp"

namespace ds_lang {

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
    BinaryOperator op;
    std::unique_ptr<Expression> exp1;
    std::unique_ptr<Expression> exp2;
};
struct Expression
{
    using ExpressionVariant = std::variant<ValueExpression, VariableExpression, BinaryExpression>;
    ExpressionVariant node;
};

} // namespace ds_lang