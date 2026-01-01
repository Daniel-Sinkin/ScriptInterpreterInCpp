// ds_lang/src/env.cpp
#include "env.hpp"

namespace ds_lang {

std::optional<i64> Environment::get(const std::string& name) const noexcept
{
    auto it = variables_.find(name);
    if (it == variables_.end()) return std::nullopt;
    return it->second;
}

void Environment::set(std::string name, i64 value)
{
    variables_.insert_or_assign(std::move(name), value);
}

} // namespace ds_lang