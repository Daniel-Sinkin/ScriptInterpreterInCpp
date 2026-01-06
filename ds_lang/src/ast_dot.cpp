#include "ast_dot.hpp"

#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace ds_lang::AstDot {
using namespace ds_lang;

namespace {

template <class T>
constexpr bool always_false_v = false;

std::string to_string(UnaryOp op) {
    switch (op) {
    case UnaryOp::Neg:
        return "-";
    case UnaryOp::Not:
        return "!";
    }
    return "?";
}

std::string to_string(BinaryOp op) {
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
    return "?";
}

struct DotBuilder {
    using NodeId = int;

    std::vector<std::string> nodes;
    std::vector<std::string> edges;
    NodeId next_id = 0;

    static std::string escape_dot(std::string_view s) {
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
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out += c;
                break;
            }
        }
        return out;
    }

    NodeId add_node(std::string_view label) {
        const NodeId id = next_id++;
        nodes.push_back(std::format("  n{} [label=\"{}\"];\n", id, escape_dot(label)));
        return id;
    }

    NodeId add_identifier_node(std::string_view label) {
        const NodeId id = next_id++;
        nodes.push_back(std::format(
            "  n{} [label=\"{}\", shape=box, style=filled, fillcolor=\"#e6f2ff\"];\n",
            id, escape_dot(label)));
        return id;
    }

    NodeId add_int_literal_node(std::string_view label) {
        const NodeId id = next_id++;
        nodes.push_back(std::format(
            "  n{} [label=\"{}\", shape=box, style=filled, fillcolor=\"#fff2cc\"];\n",
            id, escape_dot(label)));
        return id;
    }

    NodeId add_string_literal_node(std::string_view label) {
        const NodeId id = next_id++;
        nodes.push_back(std::format(
            "  n{} [label=\"{}\", shape=box, style=filled, fillcolor=\"#f2f2f2\"];\n",
            id, escape_dot(label)));
        return id;
    }

    void add_edge(NodeId from, NodeId to) {
        edges.push_back(std::format("  n{} -> n{};\n", from, to));
    }

    NodeId emit_expression(const Expression &expr) {
        return std::visit([&](const auto &e) -> NodeId {
            using T = std::decay_t<decltype(e)>;

            if constexpr (std::is_same_v<T, IntegerExpression>) {
                return add_int_literal_node(std::format("{}", e.value));

            } else if constexpr (std::is_same_v<T, IdentifierExpression>) {
                return add_identifier_node(e.name);

            } else if constexpr (std::is_same_v<T, UnaryExpression>) {
                const NodeId n = add_node(std::format("UNARY {}", to_string(e.op)));
                add_edge(n, emit_expression(*e.rhs));
                return n;

            } else if constexpr (std::is_same_v<T, BinaryExpression>) {
                const NodeId n = add_node(std::format("BIN {}", to_string(e.op)));
                add_edge(n, emit_expression(*e.lhs));
                add_edge(n, emit_expression(*e.rhs));
                return n;

            } else if constexpr (std::is_same_v<T, CallExpression>) {
                const NodeId n = add_node("CALL");
                add_edge(n, emit_expression(*e.callee));

                const NodeId args = add_node("ARGS");
                add_edge(n, args);
                for (const auto &arg_ptr : e.args) {
                    add_edge(args, emit_expression(*arg_ptr));
                }
                return n;

            } else if constexpr (std::is_same_v<T, StructAccessExpression>) {
                const NodeId n = add_node("ACCESS");
                add_edge(n, emit_expression(*e.lhs));
                add_edge(n, add_identifier_node(e.field_name));
                return n;

            } else {
                static_assert(always_false_v<T>, "Unhandled Expression variant");
            }
        },
                          expr.node);
    }

    // helper for vectors of statements (used by IF/WHILE/FUNC)
    NodeId emit_stmt_list(std::string_view label, const std::vector<Statement> &stmts) {
        const NodeId n = add_node(label);
        for (const auto &st : stmts) {
            add_edge(n, emit_statement(st));
        }
        return n;
    }

    NodeId emit_statement(const Statement &stmt) {
        return std::visit([&](const auto &s) -> NodeId {
            using T = std::decay_t<decltype(s)>;

            if constexpr (std::is_same_v<T, IntDeclarationAssignmentStatement>) {
                const NodeId n = add_node("INT_DECL_ASSIGN");
                add_edge(n, add_identifier_node(s.identifier));
                add_edge(n, emit_expression(*s.expr));
                return n;
            } else if constexpr (std::is_same_v<T, IntDeclarationStatement>) {
                const NodeId n = add_node("INT_DECL");
                add_edge(n, add_identifier_node(s.identifier));
                return n;
            } else if constexpr (std::is_same_v<T, IntAssignmentStatement>) {
                const NodeId n = add_node("INT_ASSIGN");
                add_edge(n, add_identifier_node(s.identifier));
                add_edge(n, emit_expression(*s.expr));
                return n;
            } else if constexpr (std::is_same_v<T, PrintStatement>) {
                const NodeId n = add_node("PRINT");
                add_edge(n, emit_expression(*s.expr));
                return n;
            } else if constexpr (std::is_same_v<T, PrintStringStatement>) {
                const NodeId n = add_node("PRINT_STRING");
                add_edge(n, add_string_literal_node(s.content));
                return n;
            } else if constexpr (std::is_same_v<T, ReturnStatement>) {
                const NodeId n = add_node("RETURN");
                add_edge(n, emit_expression(*s.expr));
                return n;
            } else if constexpr (std::is_same_v<T, ScopeStatement>) {
                const NodeId n = add_node("SCOPE");
                for (const auto &st : s.scope) {
                    add_edge(n, emit_statement(st));
                }
                return n;
            } else if constexpr (std::is_same_v<T, IfStatement>) {
                const NodeId n = add_node("IF");
                add_edge(n, emit_expression(*s.if_expr));
                add_edge(n, emit_stmt_list("THEN", s.then_scope));
                add_edge(n, emit_stmt_list("ELSE", s.else_scope));
                return n;
            } else if constexpr (std::is_same_v<T, WhileStatement>) {
                const NodeId n = add_node("WHILE");
                add_edge(n, emit_expression(*s.while_expr));
                add_edge(n, emit_stmt_list("DO", s.do_scope));
                return n;
            } else if constexpr (std::is_same_v<T, FunctionStatement>) {
                const NodeId n = add_node("FUNC");
                add_edge(n, add_identifier_node(s.func_name));

                const NodeId params = add_node("PARAMS");
                add_edge(n, params);
                for (const auto &v : s.vars) {
                    add_edge(params, add_identifier_node(v));
                }

                add_edge(n, emit_stmt_list("BODY", s.statements));
                return n;
            }

            else if constexpr (std::is_same_v<T, StructStatement>) {
                const NodeId n = add_node("STRUCT");
                add_edge(n, add_identifier_node(s.struct_name));

                const NodeId fields = add_node("FIELDS");
                add_edge(n, fields);
                for (const auto &v : s.vars) {
                    add_edge(fields, add_identifier_node(v));
                }
                return n;
            } else if constexpr (std::is_same_v<T, StructDeclarationAssignmentStatement>) {
                // e.g. MyStruct x = (exprs...)
                const NodeId n = add_node("STRUCT_DECL_ASSIGN");

                const NodeId type = add_node("TYPE");
                add_edge(n, type);
                add_edge(type, add_identifier_node(s.struct_name));

                const NodeId var = add_node("VAR");
                add_edge(n, var);
                add_edge(var, add_identifier_node(s.var_name));

                const NodeId vals = add_node("VALUES");
                add_edge(n, vals);
                for (const auto &e : s.exprs) {
                    add_edge(vals, emit_expression(e));
                }
                return n;
            } else if constexpr (std::is_same_v<T, StructDeclarationStatement>) {
                // e.g. MyStruct x;
                const NodeId n = add_node("STRUCT_DECL");

                const NodeId type = add_node("TYPE");
                add_edge(n, type);
                add_edge(type, add_identifier_node(s.struct_name));

                const NodeId var = add_node("VAR");
                add_edge(n, var);
                add_edge(var, add_identifier_node(s.var_name));

                return n;
            } else if constexpr (std::is_same_v<T, StructAssignmentStatement>) {
                // e.g. x = (exprs...)
                const NodeId n = add_node("STRUCT_ASSIGN");
                add_edge(n, add_identifier_node(s.var_name));

                const NodeId vals = add_node("VALUES");
                add_edge(n, vals);
                for (const auto &e : s.exprs) {
                    add_edge(vals, emit_expression(e));
                }
                return n;
            }

            else {
                static_assert(always_false_v<T>, "Unhandled Statement variant");
            }
        },
                          stmt.node);
    }

    std::string render(const std::vector<Statement> &program) {
        nodes.clear();
        edges.clear();
        next_id = 0;

        const NodeId root = add_node("PROGRAM");
        for (const auto &st : program) {
            add_edge(root, emit_statement(st));
        }

        std::string out;
        out += "digraph AST {\n";
        out += "  node [fontname=\"monospace\"];\n";

        for (const auto &n : nodes)
            out += n;
        for (const auto &e : edges)
            out += e;

        out += "}\n";
        return out;
    }
};

} // namespace

std::string to_dot(const std::vector<Statement> &program) {
    DotBuilder b;
    return b.render(program);
}

void write_dot_file(std::string_view path, const std::vector<Statement> &program) {
    std::filesystem::path p{path};

    std::ofstream os(p, std::ios::binary);
    if (!os) {
        throw std::runtime_error(std::format("AstDot: failed to open file: {}", p.string()));
    }

    const std::string dot = to_dot(program);
    os.write(dot.data(), static_cast<std::streamsize>(dot.size()));
    if (!os) {
        throw std::runtime_error(std::format("AstDot: failed to write file: {}", p.string()));
    }
}

} // namespace ds_lang::AstDot