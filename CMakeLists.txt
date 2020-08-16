cmake_minimum_required (VERSION 3.8)

project ("clox")

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_definitions(_CRT_SECURE_NO_DEPRECATE)
endif()

add_executable (clox
    "src/main.c"
    "src/opcode.h"
    "src/chunk.c"
    "src/chunk.h"
    "src/common.h"
    "src/compiler.c"
    "src/compiler.h"
    "src/debug.c"
    "src/debug.h"
    "src/errcode.h"
    "src/memory.c"
    "src/memory.h"
    "src/object.c"
    "src/object.h"
    "src/scanner.c"
    "src/scanner.h"
    "src/table.c"
    "src/table.h"
    "src/value.c"
    "src/value.h"
    "src/vm.c"
    "src/vm.h"
    "src/token.h"
    "src/parser.h"
    "src/parser.c"
    "src/token.c"
    "src/run.c"
    "src/memlib.h"
    "src/memlib.c"
    "src/gc.h"
    "src/gc.c"
)
