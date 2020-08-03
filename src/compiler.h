#pragma once

#include "common.h"
#include "object.h"
#include "chunk.h"

ObjFunction* compile(const char* source);

void mark_compiler_roots();