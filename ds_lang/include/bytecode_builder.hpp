// ds_lang/include/bytecode_builder.hpp
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "bytecode.hpp"
#include "parser.hpp"
#include "types.hpp"

namespace ds_lang {

class BytecodeBuilder {
public:
    BytecodeBuilder() = default;

    void build(const std::vector<Statement>& program);

    [[nodiscard]] const std::vector<FunctionBytecode>& functions() const noexcept { return functions_; }
    [[nodiscard]] std::optional<u32> entry_function() const noexcept { return entry_func_; }

private:
    std::vector<FunctionBytecode> functions_{};
    std::unordered_map<std::string, u32> func_ids_{};
    std::optional<u32> entry_func_{std::nullopt};

    u32 active_func_{0};

    struct Scope {
        std::unordered_map<std::string, u32> locals;
        u32 saved_next_slot{0};
    };

    std::vector<Scope> scopes_{};
    u32 next_slot_{0};
    u32 max_slot_{0};

private:
    // helpers
    [[nodiscard]] FunctionBytecode& fn();
    [[nodiscard]] const FunctionBytecode& fn() const;
    [[nodiscard]] IPtr ip() const noexcept;

    IPtr emit(BytecodeOperation op);
    BytecodeOperation& at(IPtr ip);
    void patch_jump(IPtr ip, IPtr target);

    void begin_function_locals(const std::vector<std::string>& params);
    void begin_scope();
    void end_scope();

    u32 declare_local(std::string_view name);
    u32 resolve_local(std::string_view name) const;

    u32 resolve_func(std::string_view name) const;

    // build
    void build_statement(const Statement& st);
    void build_expression(const Expression& e);
    void build_scope_statements(const std::vector<Statement>& st);

    [[nodiscard]] bool ends_with_return(const FunctionBytecode& fn) const;
};

} // namespace ds_lang