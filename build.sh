
#!/usr/bin/env bash
set -euo pipefail

clang++ \
  -std=c++23 \
  -O3 \
  -DNDEBUG \
  -Wall \
  -Wextra \
  -Wpedantic \
  -Werror \
  -Wshadow \
  -Wconversion \
  -Wsign-conversion \
  -Wnull-dereference \
  -Wdouble-promotion \
  -Wimplicit-fallthrough \
  -Wold-style-cast \
  -Woverloaded-virtual \
  -Wcast-align \
  -Wformat=2 \
  -Wfloat-equal \
  -Wswitch-enum \
  -fcolor-diagnostics \
  -c main.cpp \
  -o main.o