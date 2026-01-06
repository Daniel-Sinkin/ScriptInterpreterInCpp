// ds_lang/src/ast_dot.cpp
#include "ast_dot.hpp"

#include <format>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "parser.hpp"
#include "types.hpp"
#include "util.hpp"

namespace ds_lang::AstDot {
namespace {

constexpr std::string_view kFillDefault = "#ffffff";
constexpr std::string_view kFillControl = "#f2f2f2";
constexpr std::string_view kFillStruct = "#e6dcf5";
constexpr std::string_view kFillIdentifier = "#cfe8ff";
constexpr std::string_view kFillInteger = "#d6f5d6";
constexpr std::string_view kFillOperator = "#ffe8c7";

[[nodiscard]] std::string dot_escape(std::string_view s) {
    std::string out;
    out.reserve(s.size());

    for (char c : s) {
        switch (c) {
        case '\\':
            out += "\\\\";
            break;
        case '"':
            out += "\\\"";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            break; // ignore
        case '\t':
            out += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(c) < 0x20)
                out += ' ';
            else
                out += c;
            break;
        }
    }
    return out;
}

[[nodiscard]] std::string_view to_string(BinaryOp op) noexcept {
    switch (op) {
    case BinaryOp::Add:
        return "+";
    case BinaryOp::Sub:
        return "-";
    case BinaryOp::Mul:
        return "*";
    case BinaryOp::Div:
        return "/";
    case BinaryOp::Mod:
        return "%";
    case BinaryOp::Eq:
        return "==";
    case BinaryOp::Neq:
        return "!=";
    case BinaryOp::Lt:
        return "<";
    case BinaryOp::Le:
        return "<=";
    case BinaryOp::Gt:
        return ">";
    case BinaryOp::Ge:
        return ">=";
    case BinaryOp::And:
        return "&&";
    case BinaryOp::Or:
        return "||";
    }
    return "<?>";
}

[[nodiscard]] std::string_view to_string(UnaryOp op) noexcept {
    switch (op) {
    case UnaryOp::Neg:
        return "-";
    case UnaryOp::Not:
        return "!";
    }
    return "<?>";
}

struct DotBuilder {
    std::string out{};
    u32 next_id{0};

    DotBuilder() { out.reserve(16 * 1024); }

    [[nodiscard]] u32 add_node(
        std::string_view label,
        std::string_view fillcolor = kFillDefault,
        std::string_view shape = "box") {
        const u32 id = next_id++;
        out += std::format(
            "  n{} [label=\"{}\", shape={}, style=\"rounded,filled\", fillcolor=\"{}\"];\n",
            id,
            dot_escape(label),
            shape,
            fillcolor);
        return id;
    }

    void add_edge(u32 from, u32 to, std::string_view label = {}) {
        if (label.empty()) {
            out += std::format("  n{} -> n{};\n", from, to);
        } else {
            out += std::format(
                "  n{} -> n{} [label=\"{}\"];\n",
                from,
                to,
                dot_escape(label));
        }
    }
};

[[nodiscard]] u32 emit_expression(DotBuilder &b, const Expression *e);

[[nodiscard]] u32 emit_identifier_node(
    DotBuilder &b,
    std::string_view name,
    std::string_view edge_label,
    u32 parent) {
    const u32 id = b.add_node(std::format("Id\n{}", name), kFillIdentifier, "box");
    b.add_edge(parent, id, edge_label);
    return id;
}

[[nodiscard]] u32 emit_scope(DotBuilder &b, std::string_view label, const std::vector<Statement> &scope);

[[nodiscard]] u32 emit_statement(DotBuilder &b, const Statement &st) {
    return std::visit(
        overloaded{
            [&](const IntDeclarationAssignmentStatement &s) -> u32 {
                const u32 n = b.add_node("IntDecl", kFillControl, "box");
                (void)emit_identifier_node(b, s.identifier, "name", n);
                const u32 expr = emit_expression(b, s.expr.get());
                b.add_edge(n, expr, "init");
                return n;
            },
            [&](const IntDeclarationStatement&s) -> u32 {
                const u32 n = b.add_node("Assign", kFillControl, "box");
                (void)emit_identifier_node(b, s.identifier, "name", n);
                return n;
            },
            [&](const IntAssignmentStatement &s) -> u32 {
                const u32 n = b.add_node("Assign", kFillControl, "box");
                (void)emit_identifier_node(b, s.identifier, "name", n);
                const u32 expr = emit_expression(b, s.expr.get());
                b.add_edge(n, expr, "value");
                return n;
            },
            [&](const PrintStatement &s) -> u32 {
                const u32 n = b.add_node("Print", kFillControl, "box");
                const u32 expr = emit_expression(b, s.expr.get());
                b.add_edge(n, expr, "value");
                return n;
            },
            [&](const PrintStringStatement &s) -> u32 {
                // Show it as a literal-ish node.
                // Lexer stores content without quotes; we add quotes for display.
                const u32 n = b.add_node(std::format("PrintString\n\"{}\"", s.content), kFillControl, "box");
                return n;
            },
            [&](const ReturnStatement &s) -> u32 {
                const u32 n = b.add_node("Return", kFillControl, "box");
                const u32 expr = emit_expression(b, s.expr.get());
                b.add_edge(n, expr, "value");
                return n;
            },
            [&](const ScopeStatement &s) -> u32 {
                const u32 n = b.add_node("Scope", kFillControl, "box");
                const u32 inner = emit_scope(b, "ScopeBody", s.scope);
                b.add_edge(n, inner, "body");
                return n;
            },
            [&](const IfStatement &s) -> u32 {
                const u32 n = b.add_node("If", kFillControl, "box");
                const u32 cond = emit_expression(b, s.if_expr.get());
                b.add_edge(n, cond, "cond");
                const u32 then_n = emit_scope(b, "Then", s.then_scope);
                b.add_edge(n, then_n, "then");
                if (!s.else_scope.empty()) {
                    const u32 else_n = emit_scope(b, "Else", s.else_scope);
                    b.add_edge(n, else_n, "else");
                }
                return n;
            },
            [&](const WhileStatement &s) -> u32 {
                const u32 n = b.add_node("While", kFillControl, "box");
                const u32 cond = emit_expression(b, s.while_expr.get());
                b.add_edge(n, cond, "cond");
                const u32 body = emit_scope(b, "Body", s.do_scope);
                b.add_edge(n, body, "body");
                return n;
            },
            [&](const FunctionStatement &s) -> u32 {
                const u32 n = b.add_node("Function", kFillControl, "box");
                (void)emit_identifier_node(b, s.func_name, "name", n);

                const u32 params = b.add_node("Params", kFillControl, "box");
                b.add_edge(n, params, "params");
                for (usize p = 0; p < s.vars.size(); ++p) {
                    const u32 pid = b.add_node(std::format("Param\n{}", s.vars[p]), kFillIdentifier, "box");
                    b.add_edge(params, pid, std::format("{}", p));
                }

                const u32 body = emit_scope(b, "Body", s.statements);
                b.add_edge(n, body, "body");
                return n;
            },
            [&](const StructStatement &s) -> u32 {
                const u32 n = b.add_node("Struct", kFillStruct, "box");
                (void)emit_identifier_node(b, s.struct_name, "name", n);

                const u32 params = b.add_node("Params", kFillControl, "box");
                b.add_edge(n, params, "params");
                for (usize var_idx = 0; var_idx < s.vars.size(); ++var_idx) {
                    const u32 var_identifier = b.add_node(
                        std::format(
                            "Var\n{}",
                            s.vars[var_idx]),
                        kFillIdentifier,
                        "box");
                    b.add_edge(params, var_identifier, std::format("{}", var_idx));
                }
                return n;
            },
        },
        st.node);
}

[[nodiscard]] u32 emit_scope(DotBuilder &b, std::string_view label, const std::vector<Statement> &scope) {
    const u32 sid = b.add_node(label, kFillControl, "box");
    for (usize i = 0; i < scope.size(); ++i) {
        const u32 child = emit_statement(b, scope[i]);
        b.add_edge(sid, child, std::format("{}", i));
    }
    return sid;
}

[[nodiscard]] u32 emit_expression(DotBuilder &b, const Expression *e) {
    if (!e) {
        return b.add_node("<null-expr>", kFillDefault, "box");
    }

    return std::visit(
        overloaded{
            [&](const IntegerExpression &n) -> u32 {
                return b.add_node(std::format("Int\n{}", n.value), kFillInteger, "box");
            },
            [&](const IdentifierExpression &id) -> u32 {
                return b.add_node(std::format("Id\n{}", id.name), kFillIdentifier, "box");
            },
            [&](const UnaryExpression &u) -> u32 {
                const u32 n = b.add_node(std::format("Unary\n{}", to_string(u.op)), kFillOperator, "ellipse");
                const u32 rhs = emit_expression(b, u.rhs.get());
                b.add_edge(n, rhs, "rhs");
                return n;
            },
            [&](const BinaryExpression &bin) -> u32 {
                const u32 n = b.add_node(std::format("Binary\n{}", to_string(bin.op)), kFillOperator, "ellipse");
                const u32 lhs = emit_expression(b, bin.lhs.get());
                const u32 rhs = emit_expression(b, bin.rhs.get());
                b.add_edge(n, lhs, "lhs");
                b.add_edge(n, rhs, "rhs");
                return n;
            },
            [&](const CallExpression &c) -> u32 {
                const u32 n = b.add_node("Call", kFillDefault, "box");
                const u32 callee = emit_expression(b, c.callee.get());
                b.add_edge(n, callee, "callee");
                for (usize i = 0; i < c.args.size(); ++i) {
                    const u32 arg = emit_expression(b, c.args[i].get());
                    b.add_edge(n, arg, std::format("arg{}", i));
                }
                return n;
            },
        },
        e->node);
}

} // namespace

std::string to_dot(const std::vector<Statement> &program) {
    DotBuilder b;

    std::string header;
    header.reserve(1024);
    header += "digraph AST {\n";
    header += "  graph [rankdir=TB, fontname=\"Helvetica\", fontsize=10];\n";
    header += "  node  [fontname=\"Helvetica\", fontsize=10, color=\"#777777\"];\n";
    header += "  edge  [fontname=\"Helvetica\", fontsize=9, color=\"#777777\"];\n\n";

    const u32 root = b.add_node("Program", kFillControl, "box");
    for (usize i = 0; i < program.size(); ++i) {
        const u32 child = emit_statement(b, program[i]);
        b.add_edge(root, child, std::format("{}", i));
    }

    std::string result;
    result.reserve(header.size() + b.out.size() + 32);
    result += header;
    result += b.out;
    result += "}\n";
    return result;
}

void write_dot_file(std::string_view path, const std::vector<Statement> &program) {
    std::ofstream f(std::string(path), std::ios::out | std::ios::trunc);
    if (!f) {
        throw std::runtime_error(std::format("Failed to open DOT output file: {}", path));
    }
    const std::string dot = to_dot(program);
    f.write(dot.data(), static_cast<std::streamsize>(dot.size()));
    if (!f) {
        throw std::runtime_error(std::format("Failed to write DOT output file: {}", path));
    }
}

} // namespace ds_lang::AstDot