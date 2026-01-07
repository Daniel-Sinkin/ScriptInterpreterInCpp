// ds_lang/src/formatters.cpp
#include "formatters.hpp"

#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "bytecode.hpp"
#include "parser.hpp"
#include "util.hpp"

namespace ds_lang::Fmt {

static std::string escape_for_source(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        case '"':
            out += "\\\"";
            break;
        default:
            out += c;
            break;
        }
    }
    return out;
}

static std::string_view to_string(BinaryOp op) noexcept {
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

static std::string_view to_string(UnaryOp op) noexcept {
    switch (op) {
    case UnaryOp::Neg:
        return "-";
    case UnaryOp::Not:
        return "!";
    }
    return "<?>";
}

static int precedence(BinaryOp op) noexcept {
    switch (op) {
    case BinaryOp::Or:
        return 20;
    case BinaryOp::And:
        return 30;
    case BinaryOp::Eq:
    case BinaryOp::Neq:
        return 40;
    case BinaryOp::Lt:
    case BinaryOp::Le:
    case BinaryOp::Gt:
    case BinaryOp::Ge:
        return 50;
    case BinaryOp::Add:
    case BinaryOp::Sub:
        return 60;
    case BinaryOp::Mul:
    case BinaryOp::Div:
    case BinaryOp::Mod:
        return 70;
    }
    return -1;
}

static void append_indent(std::string &out, int indent) {
    out.append(static_cast<std::size_t>(indent), ' ');
}

static void format_expr_into(std::string &out, const Expression &e, int parent_prec, bool is_rhs) {
    std::visit(
        ds_lang::overloaded{
            [&](const IntegerExpression &n) -> void { out += std::format("{}", n.value); },
            [&](const IdentifierExpression &id) -> void { out += id.name; },
            [&](const UnaryExpression &u) -> void {
                const int my_prec = Parser::kUnaryPrec;
                const bool need_parens = my_prec < parent_prec;
                if (need_parens) {
                    out += "(";
                }

                out += std::string(to_string(u.op));
                if (u.rhs) {
                    format_expr_into(out, *u.rhs, my_prec, true);
                } else {
                    out += "<null-expr>";
                }

                if (need_parens) {
                    out += ")";
                }
            },
            [&](const BinaryExpression &b) -> void {
                const int my_prec = precedence(b.op);
                const bool need_parens = (my_prec < parent_prec) || (is_rhs && my_prec == parent_prec);

                if (need_parens) {
                    out += "(";
                }

                if (b.lhs) {
                    format_expr_into(out, *b.lhs, my_prec, false);
                } else {
                    out += "<null-expr>";
                }

                out += " ";
                out += to_string(b.op);
                out += " ";

                if (b.rhs) {
                    format_expr_into(out, *b.rhs, my_prec, true);
                } else {
                    out += "<null-expr>";
                }

                if (need_parens) {
                    out += ")";
                }
            },
            [&](const CallExpression &c) -> void {
                const int my_prec = Parser::kCallPrec;
                const bool need_parens = my_prec < parent_prec;
                if (need_parens) {
                    out += "(";
                }

                if (c.callee) {
                    format_expr_into(out, *c.callee, my_prec, false);
                } else {
                    out += "<null-expr>";
                }

                out += "(";
                for (std::size_t i = 0; i < c.args.size(); ++i) {
                    if (i) {
                        out += ", ";
                    }
                    if (c.args[i]) {
                        format_expr_into(out, *c.args[i], 0, false);
                    } else {
                        out += "<null-expr>";
                    }
                }
                out += ")";

                if (need_parens) {
                    out += ")";
                }
            },
            [&](const StructAccessExpression& a) -> void {
                if (a.lhs) format_expr_into(out, *a.lhs, Parser::kAccessPrec, false);
                else out += "<null-expr>";
                out += ".";
                out += a.field_name;
            }
        },
        e.node);
}

std::string format_expression(const Expression &e) {
    std::string out;
    out.reserve(64);
    format_expr_into(out, e, 0, false);
    return out;
}

static void format_statement_into(std::string &out, const Statement &s, int indent);

static void format_scope_into(std::string &out, const std::vector<Statement> &scope, int indent) {
    for (std::size_t i = 0; i < scope.size(); ++i) {
        append_indent(out, indent);
        format_statement_into(out, scope[i], indent);
        if (i + 1 < scope.size()) {
            out += "\n";
        }
    }
}

static void format_statement_into(std::string &out, const Statement &s, int indent) {
    std::visit(
        ds_lang::overloaded{
            [&](const IntDeclarationAssignmentStatement &st) {
                out += "int ";
                out += st.identifier;
                out += " = ";
                out += st.expr ? format_expression(*st.expr) : "<null-expr>";
                out += ";";
            },
            [&](const IntDeclarationStatement &st) {
                out += "int ";
                out += st.identifier;
                out += ";";
            },
            [&](const IntAssignmentStatement &st) {
                out += st.identifier;
                out += " = ";
                out += st.expr ? format_expression(*st.expr) : "<null-expr>";
                out += ";";
            },
            [&](const PrintStatement &st) {
                out += "print ";
                out += st.expr ? format_expression(*st.expr) : "<null-expr>";
                out += ";";
            },
            [&](const PrintStringStatement &st) {
                out += "print ";
                out += std::format("\"{}\"", escape_for_source(st.content));
                out += ";";
            },
            [&](const ReturnStatement &st) {
                out += "return ";
                out += st.expr ? format_expression(*st.expr) : "<null-expr>";
                out += ";";
            },
            [&](const ScopeStatement &st) {
                out += "{\n";
                format_scope_into(out, st.scope, indent + 4);
                out += "\n}";
            },
            [&](const IfStatement &st) {
                out += "if (";
                out += st.if_expr ? format_expression(*st.if_expr) : "<null-expr>";
                out += ") {\n";

                if (!st.then_scope.empty()) {
                    format_scope_into(out, st.then_scope, indent + 4);
                    out += "\n";
                }

                if (!st.else_scope.empty()) {
                    append_indent(out, indent);
                    out += "else\n";
                    format_scope_into(out, st.else_scope, indent + 4);
                    out += "\n";
                }
                append_indent(out, indent);
                out += "}";
            },
            [&](const WhileStatement &st) {
                out += "while (";
                out += st.while_expr ? format_expression(*st.while_expr) : "<null-expr>";
                out += ") {\n";
                if (!st.do_scope.empty()) {
                    format_scope_into(out, st.do_scope, indent + 4);
                    out += "\n";
                }
                append_indent(out, indent);
                out += "}";
            },
            [&](const CallExpression &c) {
                assert(false && "Copy paste error probably, should never touch this path");
                if (c.callee) {
                    format_expr_into(out, *c.callee, Parser::kCallPrec, false);
                } else {
                    out += "<null-expr>";
                }

                out += "(";
                for (std::size_t i = 0; i < c.args.size(); ++i) {
                    if (i > 0) {
                        out += ", ";
                    }
                    if (c.args[i]) {
                        format_expr_into(out, *c.args[i], 0, false);
                    } else {
                        out += "<null-expr>";
                    }
                }
                out += ")";
            },
            [&](const FunctionStatement &st) {
                out += "func ";
                out += st.func_name;
                out += "(";
                for (std::size_t i = 0; i < st.vars.size(); ++i) {
                    if (i > 0) {
                        out += ", ";
                    }
                    out += st.vars[i];
                }
                out += ") {\n";

                if (!st.statements.empty()) {
                    format_scope_into(out, st.statements, indent + 4);
                }

                out += '\n';
                append_indent(out, indent);
                out += "}";
            },
            [&](const StructStatement &st) {
                out += "struct ";
                out += st.struct_name;
                out += " {\n";
                for (const auto &var : st.vars) {
                    append_indent(out, indent + 4);
                    out += "int ";
                    out += var;
                    out += ";\n";
                }
                append_indent(out, indent);
                out += "}";
            },
            [&](const StructDeclarationAssignmentStatement &st) {
                out += st.struct_name;
                out += " ";
                out += st.var_name;
                out += " = {";
                for(usize i = 0; i < st.exprs.size(); ++i) {
                    out += format_expression(st.exprs[i]);
                    if(i < st.exprs.size() - 1) {
                        out += ", ";
                    }
                }
                out += "};";
            },
            [&](const StructDeclarationStatement &st) {
                out += st.struct_name;
                out += " ";
                out += st.var_name;
                out += ";";
            },
            [&](const StructAssignmentStatement &st) {
                out += st.var_name;
                out += " = {";
                for(usize i = 0; i < st.exprs.size(); ++i) {
                    out += format_expression(st.exprs[i]);
                    if(i < st.exprs.size() - 1) {
                        out += ", ";
                    }
                }
                out += "};";
            },
            [&](const StructVariableScopeStatement &st) {
                out += "{";
                for(const auto& expr: st.exprs) {
                    format_expression(expr);
                    out += ", ";
                }
                out += "}";
            }
        },
        s.node);
}

std::string format_statement(const Statement &s) {
    std::string out;
    out.reserve(128);
    format_statement_into(out, s, 0);
    return out;
}

std::string format_bytecode_operation(const BytecodeOperation &op) {
    return std::visit(
        ds_lang::overloaded{
            [&](const BytecodePushI64 &o) { return std::format("PUSH_I64 {}", o.value); },
            [&](const BytecodeAdd &) { return std::string("ADD"); },
            [&](const BytecodeSub &) { return std::string("SUB"); },
            [&](const BytecodeMult &) { return std::string("MULT"); },
            [&](const BytecodeDiv &) { return std::string("DIV"); },
            [&](const BytecodeMod &) { return std::string("MOD"); },

            [&](const BytecodeEQ &) { return std::string("EQ"); },
            [&](const BytecodeNEQ &) { return std::string("NEQ"); },
            [&](const BytecodeLT &) { return std::string("LT"); },
            [&](const BytecodeLE &) { return std::string("LE"); },
            [&](const BytecodeGT &) { return std::string("GT"); },
            [&](const BytecodeGE &) { return std::string("GE"); },

            [&](const BytecodeNEG &) { return std::string("NEG"); },
            [&](const BytecodeNOT &) { return std::string("NOT"); },

            [&](const BytecodePop &) { return std::string("POP"); },

            [&](const BytecodeLoadLocal &o) { return std::format("LOAD_LOCAL {}", o.slot); },
            [&](const BytecodeStoreLocal &o) { return std::format("STORE_LOCAL {}", o.slot); },

            [&](const BytecodeJmp &o) { return std::format("JMP {}", o.target_ip); },
            [&](const BytecodeJmpFalse &o) { return std::format("JMP_FALSE {}", o.target_ip); },
            [&](const BytecodeJmpTrue &o) { return std::format("JMP_TRUE {}", o.target_ip); },

            [&](const BytecodeCall &o) { return std::format("CALL {}", o.func_id); },
            [&](const BytecodeCallArgs &o) { return std::format("CALL_ARGS {} {}", o.func_id, o.argc); },

            [&](const BytecodeReturn &) { return std::string("RETURN"); },

            [&](const BytecodePrint &) { return std::string("PRINT"); },
            [&](const BytecodePrintString &o) { return std::format("PRINT \"{}\"", o.content); },
        },
        op);
}

std::string format_function_bytecode(const FunctionBytecode &fn) {
    std::string out;
    out += std::format("FunctionBytecode(num_locals={}, num_params={}, code=[", fn.num_locals, fn.num_params);
    for (std::size_t i = 0; i < fn.code.size(); ++i) {
        if (i) {
            out += ", ";
        }
        out += format_bytecode_operation(fn.code[i]);
    }
    out += "])";
    return out;
}

} // namespace ds_lang::Fmt