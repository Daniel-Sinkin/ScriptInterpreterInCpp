// ds_lang/src/bytecode_builder.cpp
#include "bytecode_builder.hpp"

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>

#include "bytecode.hpp"
#include "parser.hpp"
#include "types.hpp"
#include "util.hpp"

namespace ds_lang {

FunctionBytecode &BytecodeBuilder::fn() { return functions_.at(static_cast<usize>(active_func_)); }
const FunctionBytecode &BytecodeBuilder::fn() const { return functions_.at(static_cast<usize>(active_func_)); }

IPtr BytecodeBuilder::ip() const noexcept { return fn().code.size(); }

IPtr BytecodeBuilder::emit(BytecodeOperation op) {
    const IPtr here = ip();
    fn().code.push_back(std::move(op));
    return here;
}

BytecodeOperation &BytecodeBuilder::at(IPtr where) { return fn().code.at(where); }

void BytecodeBuilder::patch_jump(IPtr where, IPtr target) {
    if (target == INVALIDIPtr)
        throw std::runtime_error("patch_jump: target INVALIDIPtr");

    auto &op = at(where);
    std::visit(
        overloaded{
            [&](BytecodeJmp &j) {
                if (j.target_ip != INVALIDIPtr)
                    throw std::runtime_error("patch_jump: already patched");
                j.target_ip = target;
            },
            [&](BytecodeJmpFalse &j) {
                if (j.target_ip != INVALIDIPtr)
                    throw std::runtime_error("patch_jump: already patched");
                j.target_ip = target;
            },
            [&](BytecodeJmpTrue &j) {
                if (j.target_ip != INVALIDIPtr)
                    throw std::runtime_error("patch_jump: already patched");
                j.target_ip = target;
            },
            [&](auto &) { throw std::runtime_error("patch_jump: not a jump op"); },
        },
        op);
}

void BytecodeBuilder::begin_function_locals(const std::vector<std::string> &params) {
    scopes_.clear();
    scopes_.push_back(Scope{});
    next_slot_ = 0;
    max_slot_ = 0;

    auto &base = scopes_.back().locals;

    for (usize i = 0; i < params.size(); ++i) {
        const std::string &p = params[i];
        if (base.contains(p))
            throw std::runtime_error("Duplicate parameter: " + p);
        base.emplace(p, static_cast<u32>(i));

        if (fn().seen_symbols.size() <= i)
            fn().seen_symbols.resize(i + 1);
        fn().seen_symbols[i] = p;
    }

    next_slot_ = static_cast<u32>(params.size());
    max_slot_ = next_slot_;
}

void BytecodeBuilder::begin_scope() {
    Scope s;
    s.saved_next_slot = next_slot_;
    scopes_.push_back(std::move(s));
}

void BytecodeBuilder::end_scope() {
    if (scopes_.empty())
        throw std::runtime_error("end_scope: no scope");
    next_slot_ = scopes_.back().saved_next_slot;
    scopes_.pop_back();
}

u32 BytecodeBuilder::declare_local(std::string_view name) {
    if (scopes_.empty())
        throw std::runtime_error("declare_local: no scope");
    const std::string key{name};

    auto &cur = scopes_.back().locals;
    if (cur.contains(key))
        throw std::runtime_error("Variable already declared in this scope: " + key);

    const u32 slot = next_slot_;
    cur.emplace(key, slot);

    next_slot_ += 1;
    max_slot_ = std::max(max_slot_, next_slot_);

    const usize u = static_cast<usize>(slot);
    if (fn().seen_symbols.size() <= u)
        fn().seen_symbols.resize(u + 1);
    fn().seen_symbols[u] = key;

    return slot;
}

u32 BytecodeBuilder::resolve_local(std::string_view name) const {
    const std::string key{name};
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        const auto f = it->locals.find(key);
        if (f != it->locals.end())
            return f->second;
    }
    throw std::runtime_error("Undefined variable: " + key);
}

u32 BytecodeBuilder::resolve_func(std::string_view name) const {
    const std::string key{name};
    const auto it = func_ids_.find(key);
    if (it == func_ids_.end())
        throw std::runtime_error("Undefined function: " + key);
    return it->second;
}

[[nodiscard]] bool BytecodeBuilder::ends_with_return(const ds_lang::FunctionBytecode &fn) const {
    return !fn.code.empty() && std::holds_alternative<ds_lang::BytecodeReturn>(fn.code.back());
}

void BytecodeBuilder::build_scope_statements(const std::vector<Statement> &sts) {
    for (const auto &st : sts)
        build_statement(st);
}

void BytecodeBuilder::build_expression(const Expression &expr) {
    std::visit(
        overloaded{
            [&](const IntegerExpression &e) { emit(BytecodePushI64{e.value}); },

            [&](const IdentifierExpression &e) {
                const u32 slot = resolve_local(e.name);
                emit(BytecodeLoadLocal{slot});
            },

            [&](const UnaryExpression &e) {
                if (!e.rhs)
                    throw std::runtime_error("UnaryExpression missing rhs");
                build_expression(*e.rhs);
                switch (e.op) {
                case UnaryOp::Neg:
                    emit(BytecodeNEG{});
                    break;
                case UnaryOp::Not:
                    emit(BytecodeNOT{});
                    break;
                }
            },

            [&](const BinaryExpression &e) {
                if (!e.lhs || !e.rhs)
                    throw std::runtime_error("BinaryExpression missing operand");

                if (e.op == BinaryOp::And) {
                    build_expression(*e.lhs);
                    const IPtr jump_false = emit(BytecodeJmpFalse{INVALIDIPtr});

                    build_expression(*e.rhs);
                    emit(BytecodeNOT{});
                    emit(BytecodeNOT{});
                    const IPtr jend = emit(BytecodeJmp{INVALIDIPtr});

                    patch_jump(jump_false, ip());
                    emit(BytecodePushI64{0});
                    patch_jump(jend, ip());
                    return;
                }
                if (e.op == BinaryOp::Or) {
                    build_expression(*e.lhs);
                    const IPtr jump_true = emit(BytecodeJmpTrue{INVALIDIPtr}); // pops lhs

                    build_expression(*e.rhs);
                    emit(BytecodeNOT{});
                    emit(BytecodeNOT{});
                    const IPtr jend = emit(BytecodeJmp{INVALIDIPtr});

                    patch_jump(jump_true, ip());
                    emit(BytecodePushI64{1});
                    patch_jump(jend, ip());
                    return;
                }

                build_expression(*e.lhs);
                build_expression(*e.rhs);

                switch (e.op) {
                case BinaryOp::Add:
                    emit(BytecodeAdd{});
                    break;
                case BinaryOp::Sub:
                    emit(BytecodeSub{});
                    break;
                case BinaryOp::Mul:
                    emit(BytecodeMult{});
                    break;
                case BinaryOp::Div:
                    emit(BytecodeDiv{});
                    break;
                case BinaryOp::Mod:
                    emit(BytecodeMod{});
                    break;

                case BinaryOp::Eq:
                    emit(BytecodeEQ{});
                    break;
                case BinaryOp::Neq:
                    emit(BytecodeNEQ{});
                    break;
                case BinaryOp::Lt:
                    emit(BytecodeLT{});
                    break;
                case BinaryOp::Le:
                    emit(BytecodeLE{});
                    break;
                case BinaryOp::Gt:
                    emit(BytecodeGT{});
                    break;
                case BinaryOp::Ge:
                    emit(BytecodeGE{});
                    break;

                case BinaryOp::And:
                case BinaryOp::Or:
                    std::unreachable();
                }
            },

            [&](const CallExpression &e) {
                if (!e.callee)
                    throw std::runtime_error("CallExpression missing callee");
                const auto *id = std::get_if<IdentifierExpression>(&e.callee->node);
                if (!id)
                    throw std::runtime_error("CallExpression callee must be IdentifierExpression");

                for (const auto &arg : e.args) {
                    if (!arg)
                        throw std::runtime_error("CallExpression has null arg");
                    build_expression(*arg);
                }

                const u32 fid = resolve_func(id->name);
                const u32 argc = static_cast<u32>(e.args.size());

                if (argc == 0)
                    emit(BytecodeCall{fid});
                else
                    emit(BytecodeCallArgs{fid, argc});
            },
            [&](const StructAccessExpression &e) {
                if (!e.lhs) {
                    throw std::runtime_error("StructAccessExpression missing lhs");
                }
                throw std::runtime_error("Struct field access is not supported by BytecodeBuilder yet");
            }
        },
        expr.node);
}

void BytecodeBuilder::build_statement(const Statement &st) {
    std::visit(
        overloaded{
            [&](const IntDeclarationAssignmentStatement &s) {
                if (!s.expr)
                    throw std::runtime_error("IntDeclarationAssignmentStatement missing expr");
                const u32 slot = declare_local(s.identifier);
                build_expression(*s.expr);
                emit(BytecodeStoreLocal{slot});
            },
            [&](const IntDeclarationStatement &s) {
                const u32 slot = declare_local(s.identifier);
                emit(BytecodePushI64{.value = UNINITIALISED_VALUED});
                emit(BytecodeStoreLocal{slot});
            },
            [&](const IntAssignmentStatement &s) {
                if (!s.expr)
                    throw std::runtime_error("IntAssignmentStatement missing expr");
                const u32 slot = resolve_local(s.identifier);
                build_expression(*s.expr);
                emit(BytecodeStoreLocal{slot});
            },
            [&](const PrintStatement &s) {
                if (!s.expr)
                    throw std::runtime_error("PrintStatement missing expr");
                build_expression(*s.expr);
                emit(BytecodePrint{});
                emit(BytecodePop{});
            },
            [&](const PrintStringStatement &s) {
                emit(BytecodePrintString{s.content});
            },
            [&](const ReturnStatement &s) {
                if (!s.expr)
                    throw std::runtime_error("ReturnStatement missing expr");
                build_expression(*s.expr);
                emit(BytecodeReturn{});
            },
            [&](const ScopeStatement &s) {
                begin_scope();
                build_scope_statements(s.scope);
                end_scope();
            },
            [&](const IfStatement &s) {
                if (!s.if_expr)
                    throw std::runtime_error("IfStatement missing if_expr");

                build_expression(*s.if_expr);
                const IPtr jf = emit(BytecodeJmpFalse{INVALIDIPtr}); // pops cond

                begin_scope();
                build_scope_statements(s.then_scope);
                end_scope();

                if (!s.else_scope.empty()) {
                    const IPtr jend = emit(BytecodeJmp{INVALIDIPtr});
                    patch_jump(jf, ip());

                    begin_scope();
                    build_scope_statements(s.else_scope);
                    end_scope();

                    patch_jump(jend, ip());
                } else {
                    patch_jump(jf, ip());
                }
            },
            [&](const WhileStatement &s) {
                if (!s.while_expr)
                    throw std::runtime_error("WhileStatement missing while_expr");

                const IPtr start = ip();
                build_expression(*s.while_expr);
                const IPtr jf = emit(BytecodeJmpFalse{INVALIDIPtr}); // pops cond

                begin_scope();
                build_scope_statements(s.do_scope);
                end_scope();

                emit(BytecodeJmp{start});
                patch_jump(jf, ip());
            },
            [&](const FunctionStatement &) -> void {
                throw std::runtime_error("Nested functions are not supported");
            },
            [&](const StructStatement &) -> void {
                throw std::runtime_error("Structs must not be declared inside of function scopes.");
            },
            [&](const StructDeclarationAssignmentStatement &) -> void {
                std::unreachable();
            },
            [&](const StructDeclarationStatement &) -> void {
                std::unreachable();
            },
            [&](const StructAssignmentStatement &) -> void {
                std::unreachable();
            },
            [&](const StructVariableScopeStatement &) -> void {
                std::unreachable();
            },
        },
        st.node);
}

void BytecodeBuilder::build(const std::vector<Statement> &program) {
    functions_.clear();
    func_ids_.clear();
    entry_func_.reset();

    std::vector<const FunctionStatement *> funcs;
    for (const Statement &st : program) {
        if (const auto *fnst = std::get_if<FunctionStatement>(&st.node)) {
            funcs.push_back(fnst);
        } else {
            throw std::runtime_error("Only function declarations are allowed at top level (define func main())");
        }
    }

    for (const FunctionStatement *f : funcs) {
        if (func_ids_.contains(f->func_name)) {
            throw std::runtime_error("Duplicate function: " + f->func_name);
        }

        const u32 id = static_cast<u32>(functions_.size());
        func_ids_.emplace(f->func_name, id);

        FunctionBytecode bc;
        bc.num_params = static_cast<u32>(f->vars.size());
        bc.num_locals = bc.num_params; // will grow while compiling
        functions_.push_back(std::move(bc));
    }

    {
        const auto it = func_ids_.find("main");
        if (it == func_ids_.end()) {
            throw std::runtime_error("Missing entry point: define func main()");
        }
        entry_func_ = it->second;
    }

    // Pass 2: compile bodies
    for (const FunctionStatement *f : funcs) {
        active_func_ = resolve_func(f->func_name);

        fn().code.clear();
        fn().seen_symbols.clear();
        fn().num_params = static_cast<u32>(f->vars.size());

        begin_function_locals(f->vars);
        build_scope_statements(f->statements);

        if (!ends_with_return(fn())) {
            emit(BytecodePushI64{0});
            emit(BytecodeReturn{});
        }

        fn().num_locals = max_slot_;
    }
}

} // namespace ds_lang