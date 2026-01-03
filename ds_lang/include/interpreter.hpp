// ds_lang/include/interpreter.hpp
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "parser.hpp"

namespace ds_lang {
class Interpreter {
public:
    explicit Interpreter(bool immediate_print = true) noexcept
        : immediate_print_{immediate_print} {}
    ~Interpreter() noexcept;

    i64 evaluate_expression(const Expression &expr);

    enum class ExecResult {
        Continue,
        Return,
        Error
    };

    ExecResult process_statement(Statement &statement);
    ExecResult process_scope(std::vector<Statement> &statements);


private:
    std::unordered_map<std::string, i64> vars_{};
    std::unordered_map<std::string, FunctionStatement> funcs_{};
    std::optional<i64> return_value_{};
    std::vector<i64> print_buffer_{};
    bool immediate_print_{true};


};
} // namespace ds_lang