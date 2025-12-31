#!/usr/bin/env bash
set -euo pipefail

mkdir -p build

/usr/bin/clang++ \
  -std=c++23 \
  -O3 \
  -DNDEBUG \
  -Wall -Wextra -Wpedantic -Werror \
  -Wshadow -Wconversion -Wsign-conversion \
  -Wnull-dereference -Wdouble-promotion -Wimplicit-fallthrough \
  -Wold-style-cast -Woverloaded-virtual -Wcast-align \
  -Wformat=2 -Wfloat-equal -Wswitch-enum \
  -fcolor-diagnostics \
  main.cpp \
  -o build/main