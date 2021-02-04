# Specify the minimum version
cmake_minimum_required (VERSION 3.18)

# Setup a simple C project
project (Archer DESCRIPTION "A toy proramming language." LANGUAGES C)

# Visual C++: Disable unsafe std warnings
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_definitions(_CRT_SECURE_NO_DEPRECATE)
endif()

# Add the main executable
add_executable (archer
    "src/main.c"
    "src/opcode.h"
    "src/chunk.c"
    "src/chunk.h"
    "src/common.h"
    "src/compiler.c"
    "src/compiler.h"
    "src/disassembler.c"
    "src/disassembler.h"
    "src/memory.c"
    "src/memory.h"
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
    "src/gc.h"
    "src/gc.c"
    "src/vector.h"
    "src/ast.h"
    "src/ast.c"
    "src/astprinter.h"
    "src/astprinter.c"
    "src/object.h"
    "src/object.c"
    "src/obj_string.h"
    "src/obj_string.c"
    "src/obj_native.h"
    "src/obj_native.c"
    "src/obj_function.h"
    "src/obj_function.c"
    "src/library.h"
    "src/library.c"
    "src/obj_list.h"
    "src/obj_list.c"
    "src/obj_map.h"
    "src/obj_map.c"
    "src/obj_coroutine.h"
    "src/obj_coroutine.c"
    "src/file_reader.h"
    "src/file_reader.c"
    "src/obj_module.h"
    "src/obj_module.c"
    "src/obj_iterator.h"
    "src/obj_iterator.c"
    "src/obj_range.h"
    "src/obj_range.c"
    "src/obj_tuple.h"
    "src/obj_tuple.c"
)
