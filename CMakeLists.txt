cmake_minimum_required (VERSION 3.8)

project ("clox")

add_executable (clox "src/main.c" "src/chunk.c" "src/chunk.h" "src/common.h" "src/compiler.c" "src/compiler.h" "src/debug.c" "src/debug.h" "src/error_code.h" "src/line.c" "src/line.h" "src/memory.c" "src/memory.h" "src/object.c" "src/object.h" "src/scanner.c" "src/scanner.h" "src/stack.c" "src/stack.h" "src/table.c" "src/table.h" "src/value.c" "src/value.h" "src/vm.c" "src/vm.h")
