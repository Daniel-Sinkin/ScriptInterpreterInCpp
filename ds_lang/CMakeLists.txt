# ds_lang/CMakeLists.txt
cmake_minimum_required(VERSION 3.23)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(ds_lang VERSION 0.1.0 LANGUAGES CXX)

option(DS_LANG_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

file(GLOB DS_LANG_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_library(ds_lang ${DS_LANG_SOURCES})

target_include_directories(ds_lang
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(ds_lang PUBLIC cxx_std_23)

add_library(ds_lang_warnings INTERFACE)

if (MSVC)
    target_compile_options(ds_lang_warnings INTERFACE /W4)
    if (DS_LANG_WARNINGS_AS_ERRORS)
        target_compile_options(ds_lang_warnings INTERFACE /WX)
    endif()
else()
    target_compile_options(ds_lang_warnings INTERFACE -Wall -Wextra -Wpedantic)
    if (DS_LANG_WARNINGS_AS_ERRORS)
        target_compile_options(ds_lang_warnings INTERFACE -Werror)
    endif()
endif()

# Suppress libc++ header warnings on AppleClang/Clang (applies to all targets)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(ds_lang_warnings INTERFACE
        -Wno-reserved-user-defined-literal
        -Wno-error=reserved-user-defined-literal
        -Wno-system-headers
    )
endif()

target_link_libraries(ds_lang PUBLIC ds_lang_warnings)

add_executable(ds_lang_cli app/main.cpp)
target_link_libraries(ds_lang_cli PUBLIC ds_lang ds_lang_warnings)
target_compile_features(ds_lang_cli PRIVATE cxx_std_23)

enable_testing()

add_executable(ds_lang_tests
    tests/test_lexer.cpp
)

target_include_directories(ds_lang_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

target_link_libraries(ds_lang_tests PRIVATE ds_lang ds_lang_warnings)
target_compile_features(ds_lang_tests PRIVATE cxx_std_23)

set(LEXER_TEST_CASES
    basic_two_lines
    hspace_skipping
    multiple_blank_lines
    invalid_digit_throws
    starts_with_zero_throws
    overflow_throws
    eof_only_once_and_at_end
)

foreach(tc IN LISTS LEXER_TEST_CASES)
    add_test(NAME lexer.${tc}
             COMMAND ds_lang_tests --case ${tc})
endforeach()

add_executable(ds_lang_util_tests
    tests/test_util.cpp
)

target_include_directories(ds_lang_util_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

target_link_libraries(ds_lang_util_tests PRIVATE ds_lang ds_lang_warnings)
target_compile_features(ds_lang_util_tests PRIVATE cxx_std_23)

set(UTIL_TEST_CASES
    empty
    single_zero_ok
    starts_with_zero
    invalid_digit
    ok_small_values
    int64_max_ok
    overflow
)

foreach(tc IN LISTS UTIL_TEST_CASES)
    add_test(NAME util.string_to_i64.${tc}
             COMMAND ds_lang_util_tests --case ${tc})
endforeach()

add_executable(ds_lang_parser_tests
    tests/test_parser.cpp
)

target_include_directories(ds_lang_parser_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

target_link_libraries(ds_lang_parser_tests PRIVATE ds_lang ds_lang_warnings)
target_compile_features(ds_lang_parser_tests PRIVATE cxx_std_23)

set(PARSER_TEST_CASES
    int_assignment
    expression_precedence_mul_over_add
    left_associative_minus
    parentheses_override_precedence
    unary_binds_tighter_than_infix
    call_expression_and_args
    if_else_statement_structure
    if_without_else_has_empty_else_scope
    while_statement_structure
    function_statement_structure
    nested_scope_statement_parsing
    skip_extra_eos_inside_scope
    duplicate_function_args_throw
    only_identifiers_callable
    parenthesized_identifier_callable
    scope_statement_formatting
    missing_expression_throws
    missing_rbrace_throws
)

foreach(tc IN LISTS PARSER_TEST_CASES)
    add_test(NAME parser.${tc}
             COMMAND ds_lang_parser_tests --case ${tc})
endforeach()

add_executable(ds_lang_vm_tests
    tests/test_vm.cpp
)

target_include_directories(ds_lang_vm_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

target_link_libraries(ds_lang_vm_tests PRIVATE ds_lang ds_lang_warnings)
target_compile_features(ds_lang_vm_tests PRIVATE cxx_std_23)

set(VM_TEST_CASES
    push_and_pop
    arithmetic_ops
    div0_throws
    mod0_throws
    comparison_ops
    unary_ops
    locals_load_store
    jmp_skips_code
    jmpfalse_jmptrue
    call_and_return_with_args
    nested_calls
    print_records_value
    step_past_end_throws
)

foreach(tc IN LISTS VM_TEST_CASES)
    add_test(NAME vm.${tc}
             COMMAND ds_lang_vm_tests --case ${tc})
endforeach()