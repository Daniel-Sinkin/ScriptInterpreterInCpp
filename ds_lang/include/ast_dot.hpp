// ds_lang/include/ast_dot.hpp
#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "parser.hpp"

namespace ds_lang::AstDot {
[[nodiscard]] std::string to_dot(const std::vector<Statement>& program);
void write_dot_file(std::string_view path, const std::vector<Statement>& program);
} // namespace ds_lang::AstDot