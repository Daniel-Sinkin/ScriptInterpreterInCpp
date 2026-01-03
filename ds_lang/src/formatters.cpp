// ds_lang/src/formatters.cpp
#include "formatters.hpp"

#include <format>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "parser.hpp"
#include "util.hpp"

namespace ds_lang::Fmt {

static std::string_view to_string(BinaryOp op) noexcept {
    switch (op) {
    case BinaryOp::Add: return "+";
    case BinaryOp::Sub: return "-";
    case BinaryOp::Mul: return "*";
    case BinaryOp::Div: return "/";
    case BinaryOp::Mod: return "%";
    case BinaryOp::Eq:  return "==";
    case BinaryOp::Neq: return "!=";
    case BinaryOp::Lt:  return "<";
    case BinaryOp::Le:  return "<=";
    case BinaryOp::Gt:  return ">";
    case BinaryOp::Ge:  return ">=";
    case BinaryOp::And: return "&&";
    case BinaryOp::Or:  return "||";
    }
    return "<?>";
}

static std::string_view to_string(UnaryOp op) noexcept {
    switch (op) {
    case UnaryOp::Neg: return "-";
    case UnaryOp::Not: return "!";
    }
    return "<?>";
}

static int precedence(BinaryOp op) noexcept {
    switch (op) {
    case BinaryOp::Or:  return 20;
    case BinaryOp::And: return 30;
    case BinaryOp::Eq:
    case BinaryOp::Neq: return 40;
    case BinaryOp::Lt:
    case BinaryOp::Le:
    case BinaryOp::Gt:
    case BinaryOp::Ge:  return 50;
    case BinaryOp::Add:
    case BinaryOp::Sub: return 60;
    case BinaryOp::Mul:
    case BinaryOp::Div:
    case BinaryOp::Mod: return 70;
    }
    return -1;
}


static void append_indent(std::string& out, int indent) {
    out.append(static_cast<std::size_t>(indent), ' ');
}

static void format_expr_into(std::string& out, const Expression& e, int parent_prec, bool is_rhs) {
    std::visit(
        ds_lang::overloaded{
            [&](const IntegerExpression& n) { out += std::format("{}", n.value); },
            [&](const IdentifierExpression& id) { out += id.name; },
            [&](const UnaryExpression& u) {
                const int my_prec = Parser::kUnaryPrec;
                const bool need_parens = my_prec < parent_prec;
                if (need_parens) out.push_back('(');

                out += std::string(to_string(u.op));
                if (u.rhs) format_expr_into(out, *u.rhs, my_prec, true);
                else out += "<null-expr>";

                if (need_parens) out.push_back(')');
            },
            [&](const BinaryExpression& b) {
                const int my_prec = precedence(b.op);
                const bool need_parens =
                    (my_prec < parent_prec) || (is_rhs && my_prec == parent_prec);

                if (need_parens) out.push_back('(');

                if (b.lhs) format_expr_into(out, *b.lhs, my_prec, false);
                else out += "<null-expr>";

                out.push_back(' ');
                out += to_string(b.op);
                out.push_back(' ');

                if (b.rhs) format_expr_into(out, *b.rhs, my_prec, true);
                else out += "<null-expr>";

                if (need_parens) out.push_back(')');
            },
            [&](const CallExpression& c) {
                // Calls bind tighter than unary/infix; treat as postfix.
                const int my_prec = Parser::kCallPrec;
                const bool need_parens = my_prec < parent_prec;
                if (need_parens) out.push_back('(');

                if (c.callee) format_expr_into(out, *c.callee, my_prec, false);
                else out += "<null-expr>";

                out.push_back('(');
                for (std::size_t i = 0; i < c.args.size(); ++i) {
                    if (i) out += ", ";
                    if (c.args[i]) format_expr_into(out, *c.args[i], 0, false);
                    else out += "<null-expr>";
                }
                out.push_back(')');

                if (need_parens) out.push_back(')');
            },
        },
        e.node);
}

std::string format_expression(const Expression& e) {
    std::string out;
    out.reserve(64);
    format_expr_into(out, e, 0, false);
    return out;
}

static void format_statement_into(std::string& out, const Statement& s, int indent);

static void format_scope_into(std::string& out, const std::vector<Statement>& scope, int indent) {
    for (std::size_t i = 0; i < scope.size(); ++i) {
        append_indent(out, indent);
        format_statement_into(out, scope[i], indent);
        if (i + 1 < scope.size()) out.push_back('\n');
    }
}

static void format_statement_into(std::string& out, const Statement& s, int indent) {
    std::visit(
        ds_lang::overloaded{
            [&](const LetStatement& st) {
                out += "LET ";
                out += st.identifier;
                out += " = ";
                out += st.expr ? format_expression(*st.expr) : "<null-expr>";
            },
            [&](const PrintStatement& st) {
                out += "PRINT ";
                out += st.expr ? format_expression(*st.expr) : "<null-expr>";
            },
            [&](const ReturnStatement& st) {
                out += "RETURN ";
                out += st.expr ? format_expression(*st.expr) : "<null-expr>";
            },
            [&](const IfStatement& st) {
                out += "IF ";
                out += st.if_expr ? format_expression(*st.if_expr) : "<null-expr>";
                out += " THEN\n";

                if (!st.then_scope.empty()) {
                    format_scope_into(out, st.then_scope, indent + 4);
                    out.push_back('\n');
                }

                if (!st.else_scope.empty()) {
                    append_indent(out, indent);
                    out += "ELSE\n";
                    format_scope_into(out, st.else_scope, indent + 4);
                    out.push_back('\n');
                }

                append_indent(out, indent);
                out += "END";
            },
            [&](const WhileStatement& st) {
                out += "WHILE ";
                out += st.while_expr ? format_expression(*st.while_expr) : "<null-expr>";
                out += " DO\n";

                if (!st.do_scope.empty()) {
                    format_scope_into(out, st.do_scope, indent + 4);
                    out.push_back('\n');
                }

                append_indent(out, indent);
                out += "END";
            },
            [&](const CallExpression& c) {
                if (c.callee) format_expr_into(out, *c.callee, Parser::kCallPrec, false);
                else out += "<null-expr>";

                out.push_back('(');
                for (std::size_t i = 0; i < c.args.size(); ++i) {
                    if (i) out += ", ";
                    if (c.args[i]) format_expr_into(out, *c.args[i], 0, false);
                    else out += "<null-expr>";
                }
                out.push_back(')');
            },
            [&](const FunctionStatement& st) {
                out += "FUNC ";
                out += st.func_name;
                out += "(";
                for (std::size_t i = 0; i < st.vars.size(); ++i) {
                    if (i) out += ", ";
                    out += st.vars[i];
                }
                out += ")\n";

                if (!st.statements.empty()) {
                    format_scope_into(out, st.statements, indent + 4);
                    out.push_back('\n');
                }

                append_indent(out, indent);
                out += "END";
            },
        },
        s.node);
}

std::string format_statement(const Statement& s) {
    std::string out;
    out.reserve(128);
    format_statement_into(out, s, 0);
    return out;
}

} // namespace ds_lang::Fmt