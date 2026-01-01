#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include "types.hpp"

namespace ds_lang {

struct Environment
{
public:
    [[nodiscard]] std::optional<i64>
    get(const std::string& name) const noexcept;

    void set(std::string name, i64 value);

    std::unordered_map<std::string, i64> variables_;
};

} // namespace ds_lang