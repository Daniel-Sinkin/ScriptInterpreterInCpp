// ds_alng/include/ast.hpp
#pragma once

#include <memory>
#include <string>
#include <variant>

#include "types.hpp"

namespace ds_lang {
enum class BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Eq,
    Neq,
    Lt,
    Le,
    Gt,
    Ge,
    And,
    Or,
};

struct Expression;

struct IntegerExpression {
    i64 value;
};

struct IdentifierExpression {
    std::string name;
};

struct BinaryExpression {
    BinaryOp op;
    std::unique_ptr<Expression> lhs;
    std::unique_ptr<Expression> rhs;
};

struct Expression {
    using Variant = std::variant<
        IntegerExpression,
        IdentifierExpression,
        BinaryExpression>;
    Variant node;
};

} // namespace ds_lang