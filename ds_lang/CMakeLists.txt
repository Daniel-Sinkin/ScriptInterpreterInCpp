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

if (MSVC)
    target_compile_options(ds_lang PRIVATE /W4)
    if (DS_LANG_WARNINGS_AS_ERRORS)
        target_compile_options(ds_lang PRIVATE /WX)
    endif()
else()
    target_compile_options(ds_lang PRIVATE -Wall -Wextra -Wpedantic)
    if (DS_LANG_WARNINGS_AS_ERRORS)
        target_compile_options(ds_lang PRIVATE -Werror)
    endif()
endif()


add_executable(ds_lang_cli app/main.cpp)
target_link_libraries(ds_lang_cli PRIVATE ds_lang)
target_compile_features(ds_lang_cli PRIVATE cxx_std_23)