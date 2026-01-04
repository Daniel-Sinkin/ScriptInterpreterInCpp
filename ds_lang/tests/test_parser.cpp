// tests/test_parser.cpp
#include "common.hpp"

#include <format>
#include <print>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "lexer.hpp"
#include "parser.hpp"

namespace ds_lang::Test {

static std::vector<Statement> parse_scope_code(std::string_view code_with_braces) {
    const auto tokens = ds_lang::Lexer{code_with_braces}.tokenize_all();
    ds_lang::Parser parser{tokens};
    return parser.parse_scope();
}

static std::vector<Statement> parse_block(std::string_view inner_code) {
    std::string code;
    code.reserve(inner_code.size() + 2);
    code.push_back('{');
    code.append(inner_code);
    code.push_back('}');
    return parse_scope_code(code);
}

template <class T>
static const T &expect_stmt_is(const Statement &s) {
    const T *p = std::get_if<T>(&s.node);
    EXPECT_TRUE(p != nullptr);
    return *p;
}

static const Expression &expect_expr_ptr(const std::unique_ptr<Expression> &p) {
    EXPECT_TRUE(p != nullptr);
    return *p;
}

template <class T>
static const T &expect_expr_is(const Expression &e) {
    const T *p = std::get_if<T>(&e.node);
    EXPECT_TRUE(p != nullptr);
    return *p;
}

// ----------------------------------------------------------------------------
// Parser test cases
// ----------------------------------------------------------------------------

static void test_int_assignment() {
    const auto statements = parse_block("int x = 123;");
    EXPECT_EQ(statements.size(), static_cast<std::size_t>(1));

    const auto &st = expect_stmt_is<IntAssignmentStatement>(statements[0]);
    EXPECT_EQ(st.identifier, "x");

    const auto &e = expect_expr_ptr(st.expr);
    const auto &ie = expect_expr_is<IntegerExpression>(e);
    EXPECT_EQ(ie.value, static_cast<i64>(123));

    EXPECT_EQ(std::format("{}", statements[0]), "int x = 123;");
}

static void test_expression_precedence_mul_over_add() {
    const auto statements = parse_block("print 1 + 2 * 3;");
    EXPECT_EQ(statements.size(), static_cast<std::size_t>(1));

    const auto &st = expect_stmt_is<PrintStatement>(statements[0]);
    const auto &e = expect_expr_ptr(st.expr);

    const auto &add = expect_expr_is<BinaryExpression>(e);
    EXPECT_EQ(add.op, BinaryOp::Add);

    const auto &lhs = expect_expr_ptr(add.lhs);
    const auto &rhs = expect_expr_ptr(add.rhs);

    EXPECT_EQ(expect_expr_is<IntegerExpression>(lhs).value, static_cast<i64>(1));

    const auto &mul = expect_expr_is<BinaryExpression>(rhs);
    EXPECT_EQ(mul.op, BinaryOp::Mul);

    EXPECT_EQ(expect_expr_is<IntegerExpression>(expect_expr_ptr(mul.lhs)).value, static_cast<i64>(2));
    EXPECT_EQ(expect_expr_is<IntegerExpression>(expect_expr_ptr(mul.rhs)).value, static_cast<i64>(3));

    EXPECT_EQ(std::format("{}", statements[0]), "print 1 + 2 * 3;");
}

static void test_left_associative_minus() {
    const auto statements = parse_block("print 10 - 3 - 2;");
    EXPECT_EQ(statements.size(), static_cast<std::size_t>(1));

    const auto &st = expect_stmt_is<PrintStatement>(statements[0]);
    const auto &e = expect_expr_ptr(st.expr);

    const auto &sub1 = expect_expr_is<BinaryExpression>(e);
    EXPECT_EQ(sub1.op, BinaryOp::Sub);

    const auto &rhs1 = expect_expr_ptr(sub1.rhs);
    EXPECT_EQ(expect_expr_is<IntegerExpression>(rhs1).value, static_cast<i64>(2));

    const auto &lhs1 = expect_expr_ptr(sub1.lhs);
    const auto &sub0 = expect_expr_is<BinaryExpression>(lhs1);
    EXPECT_EQ(sub0.op, BinaryOp::Sub);

    EXPECT_EQ(expect_expr_is<IntegerExpression>(expect_expr_ptr(sub0.lhs)).value, static_cast<i64>(10));
    EXPECT_EQ(expect_expr_is<IntegerExpression>(expect_expr_ptr(sub0.rhs)).value, static_cast<i64>(3));

    EXPECT_EQ(std::format("{}", statements[0]), "print 10 - 3 - 2;");
}

static void test_parentheses_override_precedence() {
    const auto statements = parse_block("print (1 + 2) * 3;");
    EXPECT_EQ(statements.size(), static_cast<std::size_t>(1));

    const auto &st = expect_stmt_is<PrintStatement>(statements[0]);
    const auto &e = expect_expr_ptr(st.expr);

    const auto &mul = expect_expr_is<BinaryExpression>(e);
    EXPECT_EQ(mul.op, BinaryOp::Mul);

    const auto &lhs = expect_expr_ptr(mul.lhs);
    const auto &rhs = expect_expr_ptr(mul.rhs);

    EXPECT_EQ(expect_expr_is<IntegerExpression>(rhs).value, static_cast<i64>(3));

    const auto &add = expect_expr_is<BinaryExpression>(lhs);
    EXPECT_EQ(add.op, BinaryOp::Add);
    EXPECT_EQ(expect_expr_is<IntegerExpression>(expect_expr_ptr(add.lhs)).value, static_cast<i64>(1));
    EXPECT_EQ(expect_expr_is<IntegerExpression>(expect_expr_ptr(add.rhs)).value, static_cast<i64>(2));

    EXPECT_EQ(std::format("{}", statements[0]), "print (1 + 2) * 3;");
}

static void test_unary_binds_tighter_than_infix() {
    const auto statements = parse_block("print -(1 + 2) * 3;");
    EXPECT_EQ(statements.size(), static_cast<std::size_t>(1));

    const auto &st = expect_stmt_is<PrintStatement>(statements[0]);
    const auto &e = expect_expr_ptr(st.expr);

    const auto &mul = expect_expr_is<BinaryExpression>(e);
    EXPECT_EQ(mul.op, BinaryOp::Mul);

    const auto &lhs = expect_expr_ptr(mul.lhs);
    const auto &rhs = expect_expr_ptr(mul.rhs);
    EXPECT_EQ(expect_expr_is<IntegerExpression>(rhs).value, static_cast<i64>(3));

    const auto &unary = expect_expr_is<UnaryExpression>(lhs);
    EXPECT_EQ(unary.op, UnaryOp::Neg);

    const auto &inner = expect_expr_ptr(unary.rhs);
    const auto &add = expect_expr_is<BinaryExpression>(inner);
    EXPECT_EQ(add.op, BinaryOp::Add);

    EXPECT_EQ(std::format("{}", statements[0]), "print -(1 + 2) * 3;");
}

static void test_call_expression_and_args() {
    const auto statements = parse_block("print foo(1, 2 + 3);");
    EXPECT_EQ(statements.size(), static_cast<std::size_t>(1));

    const auto &st = expect_stmt_is<PrintStatement>(statements[0]);
    const auto &e = expect_expr_ptr(st.expr);

    const auto &call = expect_expr_is<CallExpression>(e);

    const auto &callee_expr = expect_expr_ptr(call.callee);
    const auto &callee = expect_expr_is<IdentifierExpression>(callee_expr);
    EXPECT_EQ(callee.name, "foo");

    EXPECT_EQ(call.args.size(), static_cast<std::size_t>(2));
    EXPECT_EQ(expect_expr_is<IntegerExpression>(expect_expr_ptr(call.args[0])).value, static_cast<i64>(1));

    const auto &arg1 = expect_expr_ptr(call.args[1]);
    const auto &add = expect_expr_is<BinaryExpression>(arg1);
    EXPECT_EQ(add.op, BinaryOp::Add);

    EXPECT_EQ(std::format("{}", statements[0]), "print foo(1, 2 + 3);");
}

static void test_if_else_statement_structure() {
    const auto statements = parse_block("if (x < 3) { print 1; } else { print 2; }");
    EXPECT_EQ(statements.size(), static_cast<std::size_t>(1));

    const auto &st = expect_stmt_is<IfStatement>(statements[0]);

    const auto &cond = expect_expr_ptr(st.if_expr);
    const auto &lt = expect_expr_is<BinaryExpression>(cond);
    EXPECT_EQ(lt.op, BinaryOp::Lt);

    EXPECT_EQ(expect_expr_is<IdentifierExpression>(expect_expr_ptr(lt.lhs)).name, "x");
    EXPECT_EQ(expect_expr_is<IntegerExpression>(expect_expr_ptr(lt.rhs)).value, static_cast<i64>(3));

    EXPECT_EQ(st.then_scope.size(), static_cast<std::size_t>(1));
    EXPECT_EQ(st.else_scope.size(), static_cast<std::size_t>(1));

    const auto &then_print = expect_stmt_is<PrintStatement>(st.then_scope[0]);
    EXPECT_EQ(expect_expr_is<IntegerExpression>(expect_expr_ptr(then_print.expr)).value, static_cast<i64>(1));

    const auto &else_print = expect_stmt_is<PrintStatement>(st.else_scope[0]);
    EXPECT_EQ(expect_expr_is<IntegerExpression>(expect_expr_ptr(else_print.expr)).value, static_cast<i64>(2));
}

static void test_if_without_else_has_empty_else_scope() {
    const auto statements = parse_block("if (1) { print 1; }");
    EXPECT_EQ(statements.size(), static_cast<std::size_t>(1));

    const auto &st = expect_stmt_is<IfStatement>(statements[0]);
    EXPECT_EQ(st.then_scope.size(), static_cast<std::size_t>(1));
    EXPECT_TRUE(st.else_scope.empty());
}

static void test_while_statement_structure() {
    const auto statements = parse_block("while (x < 3) { print x; }");
    EXPECT_EQ(statements.size(), static_cast<std::size_t>(1));

    const auto &st = expect_stmt_is<WhileStatement>(statements[0]);

    const auto &cond = expect_expr_ptr(st.while_expr);
    const auto &lt = expect_expr_is<BinaryExpression>(cond);
    EXPECT_EQ(lt.op, BinaryOp::Lt);

    EXPECT_EQ(expect_expr_is<IdentifierExpression>(expect_expr_ptr(lt.lhs)).name, "x");
    EXPECT_EQ(expect_expr_is<IntegerExpression>(expect_expr_ptr(lt.rhs)).value, static_cast<i64>(3));

    EXPECT_EQ(st.do_scope.size(), static_cast<std::size_t>(1));
    const auto &body_print = expect_stmt_is<PrintStatement>(st.do_scope[0]);
    EXPECT_EQ(expect_expr_is<IdentifierExpression>(expect_expr_ptr(body_print.expr)).name, "x");
}

static void test_function_statement_structure() {
    const auto statements = parse_block("func add(a, b) { return a + b; }");
    EXPECT_EQ(statements.size(), static_cast<std::size_t>(1));

    const auto &st = expect_stmt_is<FunctionStatement>(statements[0]);
    EXPECT_EQ(st.func_name, "add");
    EXPECT_EQ(st.vars.size(), static_cast<std::size_t>(2));
    EXPECT_EQ(st.vars[0], "a");
    EXPECT_EQ(st.vars[1], "b");

    EXPECT_EQ(st.statements.size(), static_cast<std::size_t>(1));
    const auto &ret = expect_stmt_is<ReturnStatement>(st.statements[0]);

    const auto &e = expect_expr_ptr(ret.expr);
    const auto &add = expect_expr_is<BinaryExpression>(e);
    EXPECT_EQ(add.op, BinaryOp::Add);
    EXPECT_EQ(expect_expr_is<IdentifierExpression>(expect_expr_ptr(add.lhs)).name, "a");
    EXPECT_EQ(expect_expr_is<IdentifierExpression>(expect_expr_ptr(add.rhs)).name, "b");
}

static void test_nested_scope_statement_parsing() {
    // Outer parse_block wraps this again, so this exercises parse_scope_statement().
    const auto statements = parse_block("{ print 1; }");
    EXPECT_EQ(statements.size(), static_cast<std::size_t>(1));

    const auto &st = expect_stmt_is<ScopeStatement>(statements[0]);
    EXPECT_EQ(st.scope.size(), static_cast<std::size_t>(1));

    const auto &p = expect_stmt_is<PrintStatement>(st.scope[0]);
    EXPECT_EQ(expect_expr_is<IntegerExpression>(expect_expr_ptr(p.expr)).value, static_cast<i64>(1));
}

static void test_skip_extra_eos_inside_scope() {
    const auto statements = parse_block(";;;int x = 1;;;;print x;;;");
    EXPECT_EQ(statements.size(), static_cast<std::size_t>(2));

    (void)expect_stmt_is<IntAssignmentStatement>(statements[0]);
    (void)expect_stmt_is<PrintStatement>(statements[1]);
}

static void test_duplicate_function_args_throw() {
    EXPECT_THROW(parse_block("func f(a, a) { return a; }"));
}

static void test_only_identifiers_callable() {
    EXPECT_THROW(parse_block("print (a + b)(1);"));
}

static void test_parenthesized_identifier_callable() {
    const auto statements = parse_block("print (foo)(1);");
    EXPECT_EQ(statements.size(), static_cast<std::size_t>(1));

    const auto &st = expect_stmt_is<PrintStatement>(statements[0]);
    const auto &e = expect_expr_ptr(st.expr);
    const auto &call = expect_expr_is<CallExpression>(e);

    const auto &callee = expect_expr_is<IdentifierExpression>(expect_expr_ptr(call.callee));
    EXPECT_EQ(callee.name, "foo");
    EXPECT_EQ(call.args.size(), static_cast<std::size_t>(1));
    EXPECT_EQ(expect_expr_is<IntegerExpression>(expect_expr_ptr(call.args[0])).value, static_cast<i64>(1));
}

static void test_scope_statement_formatting() {
    auto inner = parse_block("int x = 1; print x;");
    for(const auto& statement: inner) {
        std::println("{}", statement);
    }
    Statement s;
    s.node = ScopeStatement{.scope = std::move(inner)};

    auto real = std::format("{}", s);
    std::println("{}", real);
    EXPECT_EQ(real, "{\n    int x = 1;\n    print x;\n}");
}

static void test_missing_expression_throws() {
    EXPECT_THROW(parse_block("int x = ;"));
}

static void test_missing_rbrace_throws() {
    EXPECT_THROW(parse_scope_code("{ print 1; "));
}

} // namespace ds_lang::Test

int main(int argc, char **argv) {
    using namespace ds_lang::Test;

    struct TestCase {
        const char *name;
        void (*fn)();
    };

    const TestCase tests[] = {
        {"int_assignment", test_int_assignment},
        {"expression_precedence_mul_over_add", test_expression_precedence_mul_over_add},
        {"left_associative_minus", test_left_associative_minus},
        {"parentheses_override_precedence", test_parentheses_override_precedence},
        {"unary_binds_tighter_than_infix", test_unary_binds_tighter_than_infix},
        {"call_expression_and_args", test_call_expression_and_args},
        {"if_else_statement_structure", test_if_else_statement_structure},
        {"if_without_else_has_empty_else_scope", test_if_without_else_has_empty_else_scope},
        {"while_statement_structure", test_while_statement_structure},
        {"function_statement_structure", test_function_statement_structure},
        {"nested_scope_statement_parsing", test_nested_scope_statement_parsing},
        {"skip_extra_eos_inside_scope", test_skip_extra_eos_inside_scope},
        {"duplicate_function_args_throw", test_duplicate_function_args_throw},
        {"only_identifiers_callable", test_only_identifiers_callable},
        {"parenthesized_identifier_callable", test_parenthesized_identifier_callable},
        {"scope_statement_formatting", test_scope_statement_formatting},
        {"missing_expression_throws", test_missing_expression_throws},
        {"missing_rbrace_throws", test_missing_rbrace_throws},
    };

    std::string_view only;
    if (argc == 3 && std::string_view(argv[1]) == "--case") {
        only = argv[2];
    } else if (argc != 1) {
        std::println("Usage: {} [--case <name>]", argv[0]);
        return 2;
    }

    int passed = 0;
    int ran = 0;

    for (const auto &t : tests) {
        if (!only.empty() && only != t.name) {
            continue;
        }
        ++ran;
        try {
            t.fn();
            ++passed;
        } catch (const std::exception &e) {
            std::println("FAIL {}: {}", t.name, e.what());
            return 1;
        } catch (...) {
            std::println("FAIL {}: unknown exception", t.name);
            return 1;
        }
    }

    if (!only.empty() && ran == 0) {
        std::println("Unknown test case: {}", only);
        return 2;
    }

    std::println("OK {}/{}", passed, ran);
    return 0;
}