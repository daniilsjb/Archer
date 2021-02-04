#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "compiler.h"
#include "vm.h"
#include "objfunction.h"
#include "objstring.h"
#include "objmodule.h"
#include "common.h"
#include "chunk.h"
#include "parser.h"
#include "memory.h"

#if DEBUG_PRINT_CODE
#include "disassembler.h"
#endif

#if DEBUG_PRINT_AST
#include "astprinter.h"
#endif

typedef struct {
    Token identifier;
    int scopeDepth;
    bool captured;
} Local;

typedef struct {
    uint32_t index;
    bool isLocal;
} Upvalue;

typedef enum { CONTROL_FOR, CONTROL_FOR_IN, CONTROL_WHILE, CONTROL_DO_WHILE, CONTROL_WHEN } ControlType;

typedef struct ControlBreak {
    struct ControlBreak* enclosing;
    size_t address;
} ControlBreak;

typedef struct ControlBlock {
    struct ControlBlock* enclosing;
    ControlType type;
    size_t start;
    size_t end;
    ControlBreak* breaks;
} ControlBlock;

typedef enum {
    TYPE_LAMBDA,
    TYPE_FUNCTION,
    TYPE_METHOD,
    TYPE_STATIC_METHOD,
    TYPE_INITIALIZER,
    TYPE_STATIC_INITIALIZER,
    TYPE_SCRIPT
} CompilerType;

typedef struct Compiler {
    VM* vm;

    struct Compiler* enclosing;

    ObjectModule* mod;

    ObjectFunction* function;
    CompilerType type;

    ControlBlock* controlBlock;

    Local locals[UINT8_COUNT];
    int localCount;

    Upvalue upvalues[UINT8_COUNT];
    int scopeDepth;

    Token token;

    bool error;
    bool panic;
} Compiler;

typedef struct ClassCompiler {
    struct ClassCompiler* enclosing;
    Token name;
    bool hasSuperclass;
} ClassCompiler;

static void compile_tree(Compiler* compiler, AST* ast);

static void compile_declaration(Compiler* compiler, Declaration* decl);
static void compile_import_decl(Compiler* compiler, Declaration* decl);
static void compile_class_decl(Compiler* compiler, Declaration* decl);
static void compile_function_decl(Compiler* compiler, Declaration* decl);
static void compile_variable_decl(Compiler* compiler, Declaration* decl);
static void compile_statement_decl(Compiler* compiler, Declaration* decl);

static void compile_statement(Compiler* compiler, Statement* stmt);
static void compile_for_stmt(Compiler* compiler, Statement* stmt);
static void compile_for_in_stmt(Compiler* compiler, Statement* stmt);
static void compile_while_stmt(Compiler* compiler, Statement* stmt);
static void compile_do_while_stmt(Compiler* compiler, Statement* stmt);
static void compile_break_stmt(Compiler* compiler, Statement* stmt);
static void compile_continue_stmt(Compiler* compiler, Statement* stmt);
static void compile_when_stmt(Compiler* compiler, Statement* stmt);
static void compile_if_stmt(Compiler* compiler, Statement* stmt);
static void compile_return_stmt(Compiler* compiler, Statement* stmt);
static void compile_print_stmt(Compiler* compiler, Statement* stmt);
static void compile_block_stmt(Compiler* compiler, Statement* stmt);
static void compile_expression_stmt(Compiler* compiler, Statement* stmt);

static void compile_expression(Compiler* compiler, Expression* expr);
static void compile_call_expr(Compiler* compiler, Expression* expr);
static void compile_property_expr(Compiler* compiler, Expression* expr);
static void compile_subscript_expr(Compiler* compiler, Expression* expr);
static void compile_super_expr(Compiler* compiler, Expression* expr);
static void compile_assignment_expr(Compiler* compiler, Expression* expr);
static void compile_compound_assignment_expr(Compiler* compiler, Expression* expr);
static void compile_coroutine_expr(Compiler* compiler, Expression* expr);
static void compile_yield_expr(Compiler* compiler, Expression* expr);
static void compile_postfix_inc_expr(Compiler* compiler, Expression* expr);
static void compile_prefix_inc_expr(Compiler* compiler, Expression* expr);
static void compile_logical_expr(Compiler* compiler, Expression* expr);
static void compile_conditional_expr(Compiler* compiler, Expression* expr);
static void compile_elvis_expr(Compiler* compiler, Expression* expr);
static void compile_binary_expr(Compiler* compiler, Expression* expr);
static void compile_unary_expr(Compiler* compiler, Expression* expr);
static void compile_literal_expr(Compiler* compiler, Expression* expr);
static void compile_string_interp_expr(Compiler* compiler, Expression* expr);
static void compile_range_expr(Compiler* compiler, Expression* expr);
static void compile_lambda_expr(Compiler* compiler, Expression* expr);
static void compile_list_expr(Compiler* compiler, Expression* expr);
static void compile_map_expr(Compiler* compiler, Expression* expr);
static void compile_tuple_expr(Compiler* compiler, Expression* expr);
static void compile_identifier_expr(Compiler* compiler, Expression* expr);

static void compile_when_entry(Compiler* compiler, WhenEntry* entry);
static size_t compile_when_entry_list(Compiler* compiler, WhenEntryList* list);
static void compile_map_entry(Compiler* compiler, MapEntry* entry);
static size_t compile_map_entry_list(Compiler* compiler, MapEntryList* list);
static void compile_block(Compiler* compiler, Block* block);
static size_t compile_parameter_list(Compiler* compiler, ParameterList* list);
static void compile_function_body(Compiler* compiler, FunctionBody* body);
static void compile_function(Compiler* compiler, Function* function, CompilerType type, Token identifier, bool coroutine);
static void compile_named_function(Compiler* compiler, NamedFunction* function, CompilerType type);
static size_t compile_method_list(Compiler* compiler, MethodList* list);
static size_t compile_argument_list(Compiler* compiler, ArgumentList* list);
static size_t compile_expression_list(Compiler* compiler, ExpressionList* list);
static size_t compile_declaration_list(Compiler* compiler, DeclarationList* list);

static void compiler_init(Compiler* compiler, VM* vm, CompilerType type, Token identifier, ObjectModule* mod)
{
    compiler->enclosing = vm->compiler;
    vm->compiler = compiler;

    compiler->vm = vm;

    compiler->controlBlock = NULL;

    compiler->function = NULL;

    compiler->function = Function_New(vm);
    compiler->type = type;

    compiler->localCount = 0;
    compiler->scopeDepth = 0;

    compiler->token = identifier;

    compiler->mod = mod;
    compiler->function->mod = mod;

    if (type == TYPE_LAMBDA) {
        compiler->function->name = String_FromCString(vm, "lambda");
    } else if (type == TYPE_SCRIPT) {
        compiler->function->name = String_FromCString(vm, "script");
    } else {
        compiler->function->name = String_Copy(vm, identifier.start, identifier.length);
    }

    Local* local = &compiler->locals[compiler->localCount++];
    local->scopeDepth = 0;
    local->captured = false;

    if (type == TYPE_METHOD || type == TYPE_STATIC_METHOD || type == TYPE_INITIALIZER || type == TYPE_STATIC_INITIALIZER) {
        local->identifier.start = "this";
        local->identifier.length = 4;
    } else {
        local->identifier.start = "";
        local->identifier.length = 0;
    }

    compiler->error = false;
    compiler->panic = false;
}

static void enter_error_mode(Compiler* compiler)
{
    compiler->error = true;
    compiler->panic = true;
    
    Compiler* current = compiler->enclosing;
    while (current) {
        current->error = true;
        current = current->enclosing;
    }
}

static void error(Compiler* compiler, const char* message)
{
    if (compiler->panic) {
        return;
    }

    Token* token = &compiler->token;
    fprintf(stderr, "[Line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at the end");
    } else if (token->type != TOKEN_ERROR && token->type != TOKEN_NONE) {
        fprintf(stderr, " at '%.*s'", (int)token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    enter_error_mode(compiler);
}

static Chunk* current_chunk(Compiler* compiler)
{
    return &compiler->function->chunk;
}

static void emit_byte(Compiler* compiler, uint8_t byte)
{
    chunk_write(compiler->vm, current_chunk(compiler), byte, compiler->token.line);
}

static void emit_bytes(Compiler* compiler, uint8_t a, uint8_t b)
{
    emit_byte(compiler, a);
    emit_byte(compiler, b);
}

static size_t emit_jump(Compiler* compiler, uint8_t instruction)
{
    emit_byte(compiler, instruction);
    emit_byte(compiler, 0xFF);
    emit_byte(compiler, 0xFF);
    return current_chunk(compiler)->count - 2;
}

static void emit_return(Compiler* compiler)
{
    if (compiler->type == TYPE_INITIALIZER || compiler->type == TYPE_STATIC_INITIALIZER) {
        emit_bytes(compiler, OP_LOAD_LOCAL, 0);
    } else {
        emit_byte(compiler, OP_LOAD_NIL);
    }

    emit_byte(compiler, OP_RETURN);
}

static uint8_t make_constant(Compiler* compiler, Value value)
{
    int constant = chunk_add_constant(compiler->vm, current_chunk(compiler), value);
    if (constant > UINT8_MAX) {
        error(compiler, "Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void emit_constant(Compiler* compiler, Value value)
{
    emit_bytes(compiler, OP_LOAD_CONSTANT, make_constant(compiler, value));
}

static void emit_loop(Compiler* compiler, size_t loopStart, uint8_t instruction)
{
    emit_byte(compiler, instruction);

    size_t offset = current_chunk(compiler)->count - loopStart + 2;
    if (offset > UINT16_MAX) {
        error(compiler, "Loop body is too large.");
    }

    emit_byte(compiler, (offset >> 0) & 0xFF);
    emit_byte(compiler, (offset >> 8) & 0xFF);
}

static void patch_jump(Compiler* compiler, size_t offset)
{
    size_t jump = current_chunk(compiler)->count - offset - 2;

    if (jump > UINT16_MAX) {
        error(compiler, "Too much code to jump over.");
    }

    current_chunk(compiler)->code[offset    ] = (jump >> 0) & 0xFF;
    current_chunk(compiler)->code[offset + 1] = (jump >> 8) & 0xFF;
}

static void patch_breaks(Compiler* compiler, ControlBreak* breaks)
{
    ControlBreak* current = breaks;
    while (current) {
        ControlBreak* next = current->enclosing;
        patch_jump(compiler, current->address);
        free(current);
        current = next;
    }
}

static ObjectFunction* finish_compilation(VM* vm)
{
    emit_return(vm->compiler);
    ObjectFunction* function = vm->compiler->function;

#if DEBUG_PRINT_CODE
    if (!vm->compiler->error) {
        disassemble_chunk(current_chunk(vm->compiler), function->name->chars);
    }
#endif

    vm->compiler = vm->compiler->enclosing;
    return function;
}

static ControlBreak* make_control_break(Compiler* compiler, size_t address, ControlBreak* enclosing)
{
    ControlBreak* controlBreak = malloc(sizeof(ControlBreak));
    if (!controlBreak) {
        return NULL;
    }

    controlBreak->address = address;
    controlBreak->enclosing = enclosing;
    return controlBreak;
}

static void push_control_break(Compiler* compiler, size_t address)
{
    if (compiler->controlBlock) {
        ControlBreak** breaks = &compiler->controlBlock->breaks;
        *breaks = make_control_break(compiler, address, *breaks);
    }
}

static void push_control_break_to_block(Compiler* compiler, size_t address, ControlBlock* block)
{
    ControlBreak** breaks = &block->breaks;
    *breaks = make_control_break(compiler, address, *breaks);
}

static void push_control_block(Compiler* compiler, ControlType type, size_t start, size_t end)
{
    ControlBlock* block = malloc(sizeof(ControlBlock));
    block->type = type;
    block->start = start;
    block->end = end;
    block->enclosing = compiler->controlBlock;
    block->breaks = NULL;

    compiler->controlBlock = block;
}

static void pop_control_block(Compiler* compiler)
{
    ControlBlock* next = compiler->controlBlock->enclosing;
    free(compiler->controlBlock);
    compiler->controlBlock = next;
}

static void enter_control_block(Compiler* compiler, ControlType type, size_t start, size_t end)
{
    push_control_block(compiler, type, start, end);
}

static void exit_control_block(Compiler* compiler)
{
    compiler->controlBlock->end = current_chunk(compiler)->count;
    patch_breaks(compiler, compiler->controlBlock->breaks);
    pop_control_block(compiler);
}

static Local* top_local(Compiler* compiler)
{
    return &compiler->locals[compiler->localCount - 1];
}

static Local* pop_local(Compiler* compiler)
{
    compiler->localCount--;
    return top_local(compiler);
}

static void remove_locals(Compiler* compiler)
{
    Local* local = top_local(compiler);
    while (compiler->localCount > 0 && local->scopeDepth > compiler->scopeDepth) {
        emit_byte(compiler, local->captured ? OP_CLOSE_UPVALUE : OP_POP);
        local = pop_local(compiler);
    }
}

static void begin_scope(Compiler* compiler)
{
    compiler->scopeDepth++;
}

static void end_scope(Compiler* compiler)
{
    compiler->scopeDepth--;
    remove_locals(compiler);
}

static void initialize_local(Compiler* compiler)
{
    if (compiler->scopeDepth != 0) {
        top_local(compiler)->scopeDepth = compiler->scopeDepth;
    }
}

static void initialize_local_relative(Compiler* compiler, int n)
{
    if (compiler->scopeDepth != 0) {
        compiler->locals[compiler->localCount - 1 - n].scopeDepth = compiler->scopeDepth;
    }
}

static void define_variable(Compiler* compiler, uint8_t global)
{
    if (compiler->scopeDepth == 0) {
        emit_bytes(compiler, OP_DEFINE_GLOBAL, global);
    } else {
        initialize_local(compiler);
    }
}

static void add_local(Compiler* compiler, Token identifier)
{
    if (compiler->localCount == UINT8_COUNT) {
        error(compiler, "Too many local variables in function.");
        return;
    }

    Local* local = &compiler->locals[compiler->localCount++];
    local->identifier = identifier;
    local->scopeDepth = -1;
    local->captured = false;
}

static int resolve_local(Compiler* compiler, Token* identifier)
{
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];

        if (lexemes_equal(identifier, &local->identifier)) {
            if (local->scopeDepth == -1) {
                error(compiler, "Cannot read local variable in its own initializer.");
            }

            return i;
        }
    }

    return -1;
}

static int add_upvalue(Compiler* compiler, uint8_t index, bool isLocal)
{
    size_t upvalueCount = compiler->function->upvalueCount;

    for (size_t i = 0; i < upvalueCount; i++) {
        Upvalue* upvalue = &compiler->upvalues[i];
        if (upvalue->index == index && upvalue->isLocal == isLocal) {
            return (int)i;
        }
    }

    if (upvalueCount == UINT8_COUNT) {
        error(compiler, "Too many closure variables in function.");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return (int)compiler->function->upvalueCount++;
}

static int resolve_upvalue(Compiler* compiler, Token* identifier)
{
    if (!compiler->enclosing) {
        return -1;
    }

    int local = resolve_local(compiler->enclosing, identifier);
    if (local != -1) {
        compiler->enclosing->locals[local].captured = true;
        return add_upvalue(compiler, (uint8_t)local, true);
    }

    int upvalue = resolve_upvalue(compiler->enclosing, identifier);
    if (upvalue != -1) {
        return add_upvalue(compiler, (uint8_t)upvalue, false);
    }

    return -1;
}

static void declare_local_variable(Compiler* compiler, Token identifier)
{
    if (compiler->scopeDepth == 0) {
        return;
    }

    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (local->scopeDepth != -1 && local->scopeDepth < compiler->scopeDepth) {
            break;
        }

        if (lexemes_equal(&identifier, &local->identifier)) {
            error(compiler, "Variable with this name already declared in this scope.");
        }
    }

    add_local(compiler, identifier);
}

static uint8_t make_identifier_constant(Compiler* compiler, Token identifier)
{
    return make_constant(compiler, OBJ_VAL(String_Copy(compiler->vm, identifier.start, identifier.length)));
}

static uint8_t declare_variable(Compiler* compiler, Token identifier)
{
    if (compiler->scopeDepth == 0) {
        return make_identifier_constant(compiler, identifier);
    } else {
        declare_local_variable(compiler, identifier);
        return 0;
    }
}

static void named_variable(Compiler* compiler, Token identifier, ExprContext context)
{
    int scope = -1;
    uint8_t operation;

    if ((scope = resolve_local(compiler, &identifier)) != -1) {
        operation = context == LOAD ? OP_LOAD_LOCAL : OP_STORE_LOCAL;
    } else if ((scope = resolve_upvalue(compiler, &identifier)) != -1) {
        operation = context == LOAD ? OP_LOAD_UPVALUE : OP_STORE_UPVALUE;
    } else {
        scope = make_identifier_constant(compiler, identifier);
        operation = context == LOAD ? OP_LOAD_GLOBAL : OP_STORE_GLOBAL;
    }
    
    emit_bytes(compiler, operation, (uint8_t)scope);
}

void compile_tree(Compiler* compiler, AST* ast)
{
    compile_declaration_list(compiler, ast->body);
}

void compile_declaration(Compiler* compiler, Declaration* decl)
{
    compiler->panic = false;

    switch (decl->type) {
        case DECL_IMPORT: compile_import_decl(compiler, decl); return;
        case DECL_CLASS: compile_class_decl(compiler, decl); return;
        case DECL_FUNCTION: compile_function_decl(compiler, decl); return;
        case DECL_VARIABLE: compile_variable_decl(compiler, decl); return;
        case DECL_STATEMENT: compile_statement_decl(compiler, decl); return;
    }
}

void compile_import_decl(Compiler* compiler, Declaration* decl)
{
    compile_expression(compiler, decl->as.importDecl.moduleName);
    emit_byte(compiler, OP_IMPORT_MODULE);
    emit_byte(compiler, OP_POP);

    switch (decl->as.importDecl.type) {
        case IMPORT_ALL: {
            emit_byte(compiler, OP_IMPORT_ALL);
            break;
        }
        case IMPORT_AS: {
            uint8_t global = declare_variable(compiler, decl->as.importDecl.with.alias);
            define_variable(compiler, global);
            break;
        }
        case IMPORT_FOR: {
            emit_byte(compiler, OP_SAVE_MODULE);
            for (ParameterList* current = decl->as.importDecl.with.names; current != NULL; current = current->next) {
                emit_byte(compiler, OP_IMPORT_BY_NAME);
                emit_byte(compiler, make_identifier_constant(compiler, current->parameter));

                uint8_t global = declare_variable(compiler, current->parameter);
                define_variable(compiler, global);
            }
            break;
        }
    }
}

static void compile_method(Compiler* compiler, Method* method)
{
    NamedFunction* function = method->namedFunction;

    Token identifier = function->identifier;
    compiler->token = identifier;
    uint8_t name = make_identifier_constant(compiler, identifier);

    CompilerType type = method->isStatic ? TYPE_STATIC_METHOD : TYPE_METHOD;
    if (identifier.length == 4 && memcmp(identifier.start, "init", 4) == 0) {
        type = method->isStatic ? TYPE_STATIC_INITIALIZER : TYPE_INITIALIZER;
    }

    compile_named_function(compiler, function, type);
    emit_bytes(compiler, method->isStatic ? OP_STATIC_METHOD : OP_METHOD, name);
}

void compile_class_decl(Compiler* compiler, Declaration* decl)
{
    Token identifier = decl->as.classDecl.identifier;
    compiler->token = identifier;
    uint8_t name = make_identifier_constant(compiler, identifier);

    declare_local_variable(compiler, identifier);

    emit_bytes(compiler, OP_CLASS, name);
    define_variable(compiler, name);

    ClassCompiler classCompiler = { .name = identifier, .enclosing = compiler->vm->classCompiler, .hasSuperclass = false };
    compiler->vm->classCompiler = &classCompiler;

    Token superclass = decl->as.classDecl.superclass;
    if (superclass.type != TOKEN_NONE) {
        compiler->token = superclass;
        named_variable(compiler, superclass, LOAD);

        if (lexemes_equal(&identifier, &superclass)) {
            error(compiler, "A class cannot inherit from itself.");
        }

        begin_scope(compiler);
        add_local(compiler, synthetic_token("super"));
        define_variable(compiler, 0);

        named_variable(compiler, identifier, LOAD);
        emit_byte(compiler, OP_INHERIT);

        classCompiler.hasSuperclass = true;
    }

    named_variable(compiler, identifier, LOAD);

    compile_method_list(compiler, decl->as.classDecl.body);

    emit_byte(compiler, OP_END_CLASS);

    if (classCompiler.hasSuperclass) {
        end_scope(compiler);
    }

    compiler->vm->classCompiler = compiler->vm->classCompiler->enclosing;
}

void compile_function_decl(Compiler* compiler, Declaration* decl)
{
    Token identifier = decl->as.functionDecl.function->identifier;
    compiler->token = identifier;
    uint8_t global = declare_variable(compiler, identifier);
    initialize_local(compiler);
    compile_named_function(compiler, decl->as.functionDecl.function, TYPE_FUNCTION);
    define_variable(compiler, global);
}

static void compile_single_variable_decl(Compiler* compiler, Declaration* decl, Token identifier)
{
    compiler->token = identifier;
    uint8_t global = declare_variable(compiler, identifier);

    Expression* value = decl->as.variableDecl.value;
    if (value) {
        compile_expression(compiler, value);
    } else {
        emit_byte(compiler, OP_LOAD_NIL);
    }

    define_variable(compiler, global);
}

static void unpack_tuple(Compiler* compiler, Expression* tuple, size_t length)
{
    if (tuple == NULL) {
        for (size_t i = 0; i < length; i++) {
            emit_byte(compiler, OP_LOAD_NIL);
        }
        return;
    }

    compile_expression(compiler, tuple);
    emit_bytes(compiler, OP_TUPLE_UNPACK, (uint8_t)length);
}

static void compile_multiple_variable_decl(Compiler* compiler, Declaration* decl, ParameterList* identifiers)
{
    size_t length = ast_parameter_list_length(identifiers);
    if (length > 255) {
        error(compiler, "Cannot unpack into more than 255 variables.");
    }

    uint8_t* globals = malloc(length);
    size_t i = 0;
    for (ParameterList* current = identifiers; current != NULL; current = current->next) {
        compiler->token = current->parameter;
        globals[i++] = declare_variable(compiler, current->parameter);
    }

    unpack_tuple(compiler, decl->as.variableDecl.value, length);

    if (compiler->scopeDepth == 0) {
        for (int i = (int)length - 1; i >= 0; i--) {
            emit_bytes(compiler, OP_DEFINE_GLOBAL, globals[i]);
        }
    } else {
        for (int i = 0; i < (int)length; i++) {
            initialize_local_relative(compiler, i);
        }
    }

    free(globals);
}

void compile_variable_decl(Compiler* compiler, Declaration* decl)
{
    VariableTarget* target = decl->as.variableDecl.target;
    if (target->type == VAR_SINGLE) {
        compile_single_variable_decl(compiler, decl, target->as.single);
    } else {
        compile_multiple_variable_decl(compiler, decl, target->as.unpack);
    }
}

void compile_statement_decl(Compiler* compiler, Declaration* decl)
{
    compile_statement(compiler, decl->as.statement);
}

void compile_statement(Compiler* compiler, Statement* stmt)
{
    switch (stmt->type) {
        case STMT_FOR: compile_for_stmt(compiler, stmt); return;
        case STMT_FOR_IN: compile_for_in_stmt(compiler, stmt); return;
        case STMT_WHILE: compile_while_stmt(compiler, stmt); return;
        case STMT_DO_WHILE: compile_do_while_stmt(compiler, stmt); return;
        case STMT_BREAK: compile_break_stmt(compiler, stmt); return;
        case STMT_CONTINUE: compile_continue_stmt(compiler, stmt); return;
        case STMT_WHEN: compile_when_stmt(compiler, stmt); return;
        case STMT_IF: compile_if_stmt(compiler, stmt); return;
        case STMT_RETURN: compile_return_stmt(compiler, stmt); return;
        case STMT_PRINT: compile_print_stmt(compiler, stmt); return;
        case STMT_BLOCK: compile_block_stmt(compiler, stmt); return;
        case STMT_EXPRESSION: compile_expression_stmt(compiler, stmt); return;
    }
}

void compile_for_stmt(Compiler* compiler, Statement* stmt)
{
    begin_scope(compiler);

    Declaration* initializer = stmt->as.forStmt.initializer;
    if (initializer) {
        compile_declaration(compiler, initializer);
    }

    size_t loopStart = current_chunk(compiler)->count;
    size_t exitJump = -1;

    Expression* condition = stmt->as.forStmt.condition;
    if (condition) {
        compile_expression(compiler, condition);
        exitJump = emit_jump(compiler, OP_POP_JUMP_IF_FALSE);
    }

    Expression* increment = stmt->as.forStmt.increment;
    if (increment) {
        size_t bodyJump = emit_jump(compiler, OP_JUMP);

        size_t incrementStart = current_chunk(compiler)->count;
        compile_expression(compiler, increment);
        emit_byte(compiler, OP_POP);

        emit_loop(compiler, loopStart, OP_LOOP);
        loopStart = incrementStart;
        patch_jump(compiler, bodyJump);
    }

    enter_control_block(compiler, CONTROL_FOR, loopStart, 0xFFFF);

    Statement* body = stmt->as.forStmt.body;
    compile_statement(compiler, body);

    emit_loop(compiler, loopStart, OP_LOOP);

    if (exitJump != -1) {
        patch_jump(compiler, exitJump);
    }

    exit_control_block(compiler);
    end_scope(compiler);
}

static void declare_for_in_variable(Compiler* compiler, Token identifier)
{
    compiler->token = identifier;
    emit_byte(compiler, OP_LOAD_NIL);
    declare_local_variable(compiler, identifier);
    initialize_local(compiler);
}

static void for_in_declare_elements(Compiler* compiler, VariableTarget* target)
{
    if (target->type == VAR_UNPACK) {
        for (ParameterList* current = target->as.unpack; current != NULL; current = current->next) {
            declare_for_in_variable(compiler, current->parameter);
        }
    } else {
        declare_for_in_variable(compiler, target->as.single);
    }
}

static void for_in_store_elements(Compiler* compiler, VariableTarget* target)
{
    if (target->type == VAR_UNPACK) {
        uint8_t count = (uint8_t)ast_parameter_list_length(target->as.unpack);
        emit_bytes(compiler, OP_TUPLE_UNPACK, count);
        for (ParameterList* current = ast_parameter_list_end(target->as.unpack); current != NULL; current = current->prev) {
            named_variable(compiler, current->parameter, STORE);
            emit_byte(compiler, OP_POP);
        }
    } else {
        named_variable(compiler, target->as.single, STORE);
        emit_byte(compiler, OP_POP);
    }
}

void compile_for_in_stmt(Compiler* compiler, Statement* stmt)
{
    begin_scope(compiler);

    Declaration* element = stmt->as.forInStmt.element;
    VariableTarget* target = element->as.variableDecl.target;

    for_in_declare_elements(compiler, target);

    Expression* collection = stmt->as.forInStmt.collection;
    compile_expression(compiler, collection);
    emit_byte(compiler, OP_ITERATOR);
    add_local(compiler, empty_token());
    initialize_local(compiler);

    size_t loopStart = current_chunk(compiler)->count;
    size_t exitJump = emit_jump(compiler, OP_FOR_ITERATOR);

    for_in_store_elements(compiler, target);

    enter_control_block(compiler, CONTROL_FOR_IN, loopStart, 0xFFFF);

    Statement* body = stmt->as.forInStmt.body;
    compile_statement(compiler, body);

    emit_loop(compiler, loopStart, OP_LOOP);
    patch_jump(compiler, exitJump);

    exit_control_block(compiler);
    end_scope(compiler);
}

void compile_while_stmt(Compiler* compiler, Statement* stmt)
{
    size_t loopStart = current_chunk(compiler)->count;
    push_control_block(compiler, CONTROL_WHILE, loopStart, 0xFFFF);

    Expression* condition = stmt->as.whileStmt.condition;
    compile_expression(compiler, condition);

    size_t exitJump = emit_jump(compiler, OP_POP_JUMP_IF_FALSE);

    Statement* body = stmt->as.whileStmt.body;
    compile_statement(compiler, body);

    emit_loop(compiler, loopStart, OP_LOOP);
    patch_jump(compiler, exitJump);

    exit_control_block(compiler);
}

void compile_do_while_stmt(Compiler* compiler, Statement* stmt)
{
    size_t loopStart = current_chunk(compiler)->count;
    push_control_block(compiler, CONTROL_DO_WHILE, loopStart, 0xFFFF);

    Statement* body = stmt->as.doWhileStmt.body;
    compile_statement(compiler, body);

    Expression* condition = stmt->as.doWhileStmt.condition;
    compile_expression(compiler, condition);

    emit_loop(compiler, loopStart, OP_POP_LOOP_IF_TRUE);

    exit_control_block(compiler);
}

static bool block_is_loop(ControlBlock* block)
{
    return block->type == CONTROL_FOR || block->type == CONTROL_FOR_IN || block->type == CONTROL_WHILE || block->type == CONTROL_DO_WHILE;
}

static ControlBlock* closest_loop(Compiler* compiler)
{
    ControlBlock* block = compiler->controlBlock;
    while (block) {
        if (block_is_loop(block)) {
            return block;
        }

        block = block->enclosing;
    }

    return NULL;
}

void compile_break_stmt(Compiler* compiler, Statement* stmt)
{
    Token keyword = stmt->as.breakStmt.keyword;
    compiler->token = keyword;

    ControlBlock* block = closest_loop(compiler);
    if (!block) {
        error(compiler, "Cannot use 'break' outside of a loop.");
    } else {
        size_t address = emit_jump(compiler, OP_JUMP);
        push_control_break_to_block(compiler, address, block);
    }
}

void compile_continue_stmt(Compiler* compiler, Statement* stmt)
{
    Token keyword = stmt->as.continueStmt.keyword;
    compiler->token = keyword;

    ControlBlock* block = closest_loop(compiler);
    if (!block) {
        error(compiler, "Cannot use 'continue' outside of a loop.");
    } else {
        emit_loop(compiler, block->start, OP_LOOP);
    }
}

void compile_when_stmt(Compiler* compiler, Statement* stmt)
{
    size_t start = current_chunk(compiler)->count;
    enter_control_block(compiler, CONTROL_WHEN, start, 0xFFFF);

    Expression* control = stmt->as.whenStmt.control;
    compile_expression(compiler, control);

    WhenEntryList* entries = stmt->as.whenStmt.entries;
    compile_when_entry_list(compiler, entries);

    Statement* elseBranch = stmt->as.whenStmt.elseBranch;
    if (elseBranch) {
        emit_byte(compiler, OP_POP);
        compile_statement(compiler, elseBranch);
    }

    exit_control_block(compiler);
}

void compile_if_stmt(Compiler* compiler, Statement* stmt)
{
    Expression* condition = stmt->as.ifStmt.condition;
    compile_expression(compiler, condition);

    size_t thenJump = emit_jump(compiler, OP_JUMP_IF_FALSE);
    emit_byte(compiler, OP_POP);

    Statement* thenBranch = stmt->as.ifStmt.thenBranch;
    compile_statement(compiler, thenBranch);

    size_t elseJump = emit_jump(compiler, OP_JUMP);

    patch_jump(compiler, thenJump);
    emit_byte(compiler, OP_POP);

    Statement* elseBranch = stmt->as.ifStmt.elseBranch;
    if (elseBranch) {
        compile_statement(compiler, elseBranch);
    }

    patch_jump(compiler, elseJump);
}

void compile_return_stmt(Compiler* compiler, Statement* stmt)
{
    compiler->token = stmt->as.returnStmt.keyword;

    if (compiler->type == TYPE_SCRIPT) {
        error(compiler, "Can only return from functions.");
    }

    Expression* value = stmt->as.returnStmt.expression;
    if (value) {
        if (compiler->type == TYPE_INITIALIZER || compiler->type == TYPE_STATIC_INITIALIZER) {
            error(compiler, "Cannot return a value from an initializer.");
        }
        compile_expression(compiler, value);
        emit_byte(compiler, OP_RETURN);
    } else {
        emit_return(compiler);
    }
}

void compile_print_stmt(Compiler* compiler, Statement* stmt)
{
    compile_expression(compiler, stmt->as.printStmt.expression);
    emit_byte(compiler, OP_PRINT);
}

void compile_block_stmt(Compiler* compiler, Statement* stmt)
{
    begin_scope(compiler);
    compile_block(compiler, stmt->as.blockStmt.block);
    end_scope(compiler);
}

void compile_expression_stmt(Compiler* compiler, Statement* stmt)
{
    compile_expression(compiler, stmt->as.expression);
    emit_byte(compiler, OP_POP);
}

void compile_expression(Compiler* compiler, Expression* expr)
{
    switch (expr->type) {
        case EXPR_CALL: compile_call_expr(compiler, expr); return;
        case EXPR_PROPERTY: compile_property_expr(compiler, expr); return;
        case EXPR_SUBSCRIPT: compile_subscript_expr(compiler, expr); return;
        case EXPR_SUPER: compile_super_expr(compiler, expr); return;
        case EXPR_ASSIGNMENT: compile_assignment_expr(compiler, expr); return;
        case EXPR_COMPOUND_ASSIGNMNET: compile_compound_assignment_expr(compiler, expr); return;
        case EXPR_COROUTINE: compile_coroutine_expr(compiler, expr); return;
        case EXPR_YIELD: compile_yield_expr(compiler, expr); return;
        case EXPR_POSTFIX_INC: compile_postfix_inc_expr(compiler, expr); return;
        case EXPR_PREFIX_INC: compile_prefix_inc_expr(compiler, expr); return;
        case EXPR_LOGICAL: compile_logical_expr(compiler, expr); return;
        case EXPR_CONDITIONAL: compile_conditional_expr(compiler, expr); return;
        case EXPR_ELVIS: compile_elvis_expr(compiler, expr); return;
        case EXPR_BINARY: compile_binary_expr(compiler, expr); return;
        case EXPR_UNARY: compile_unary_expr(compiler, expr); return;
        case EXPR_LITERAL: compile_literal_expr(compiler, expr); return;
        case EXPR_STRING_INTERP: compile_string_interp_expr(compiler, expr); return;
        case EXPR_RANGE: compile_range_expr(compiler, expr); return;
        case EXPR_LAMBDA: compile_lambda_expr(compiler, expr); return;
        case EXPR_LIST: compile_list_expr(compiler, expr); return;
        case EXPR_MAP: compile_map_expr(compiler, expr); return;
        case EXPR_TUPLE: compile_tuple_expr(compiler, expr); return;
        case EXPR_IDENTIFIER: compile_identifier_expr(compiler, expr); return;
    }
}

static void compile_invocation(Compiler* compiler, Expression* expr)
{
    Expression* callee = expr->as.callExpr.callee;

    Expression* object = callee->as.propertyExpr.object;
    compile_expression(compiler, object);

    ArgumentList* arguments = expr->as.callExpr.arguments;
    uint8_t argumentCount = (uint8_t)compile_argument_list(compiler, arguments);

    Token property = callee->as.propertyExpr.property;
    compiler->token = property;
    uint8_t name = make_identifier_constant(compiler, property);

    bool safe = callee->as.propertyExpr.safe;
    emit_bytes(compiler, safe ? OP_INVOKE_SAFE : OP_INVOKE , name);
    emit_byte(compiler, argumentCount);
}

static void compile_super_invocation(Compiler* compiler, Expression* expr)
{
    Expression* callee = expr->as.callExpr.callee;

    Token keyword = callee->as.superExpr.keyword;
    compiler->token = keyword;

    if (!compiler->vm->classCompiler) {
        error(compiler, "Cannot use 'super' outside of a class.");
    } else if (!compiler->vm->classCompiler->hasSuperclass) {
        error(compiler, "Cannot use 'super' in a class with no superclass.");
    } else if (compiler->type == TYPE_STATIC_METHOD) {
        error(compiler, "Cannot use 'super' in a static method.");
    }

    Token method = callee->as.superExpr.method;
    compiler->token = method;
    uint8_t name = make_identifier_constant(compiler, method);

    named_variable(compiler, synthetic_token("this"), LOAD);

    ArgumentList* arguments = expr->as.callExpr.arguments;
    uint8_t argumentCount = (uint8_t)compile_argument_list(compiler, arguments);

    named_variable(compiler, synthetic_token("super"), LOAD);
    emit_bytes(compiler, OP_SUPER_INVOKE, name);
    emit_byte(compiler, argumentCount);
}

void compile_call_expr(Compiler* compiler, Expression* expr)
{
    Expression* callee = expr->as.callExpr.callee;
    if (callee->type == EXPR_PROPERTY) {
        compile_invocation(compiler, expr);
    } else if (callee->type == EXPR_SUPER) {
        compile_super_invocation(compiler, expr);
    } else {
        compile_expression(compiler, callee);

        ArgumentList* arguments = expr->as.callExpr.arguments;
        uint8_t argumentCount = (uint8_t)compile_argument_list(compiler, arguments);

        emit_bytes(compiler, OP_CALL, argumentCount);
    }
}

void compile_property_expr(Compiler* compiler, Expression* expr)
{
    Expression* object = expr->as.propertyExpr.object;
    compile_expression(compiler, object);

    Token property = expr->as.propertyExpr.property;
    compiler->token = property;
    uint8_t name = make_identifier_constant(compiler, property);

    ExprContext context = expr->as.propertyExpr.context;
    bool safe = expr->as.propertyExpr.safe;
    uint8_t operation = context == LOAD ? (safe ? OP_LOAD_PROPERTY_SAFE : OP_LOAD_PROPERTY) : (safe ? OP_STORE_PROPERTY_SAFE : OP_STORE_PROPERTY);
    emit_bytes(compiler, operation, name);
}

void compile_subscript_expr(Compiler* compiler, Expression* expr)
{
    Expression* object = expr->as.subscriptExpr.object;
    compile_expression(compiler, object);

    Expression* index = expr->as.subscriptExpr.index;
    compile_expression(compiler, index);

    ExprContext context = expr->as.subscriptExpr.context;
    bool safe = expr->as.subscriptExpr.safe;
    uint8_t operation = context == LOAD ? (safe ? OP_LOAD_SUBSCRIPT_SAFE : OP_LOAD_SUBSCRIPT) : (safe ? OP_STORE_SUBSCRIPT_SAFE : OP_STORE_SUBSCRIPT);
    emit_byte(compiler, operation);
}

void compile_super_expr(Compiler* compiler, Expression* expr)
{
    Token keyword = expr->as.superExpr.keyword;
    compiler->token = keyword;

    if (!compiler->vm->classCompiler) {
        error(compiler, "Cannot use 'super' outside of a class.");
    } else if (!compiler->vm->classCompiler->hasSuperclass) {
        error(compiler, "Cannot use 'super' in a class with no superclass.");
    } else if (compiler->type == TYPE_STATIC_METHOD) {
        error(compiler, "Cannot use 'super' in a static method.");
    }

    Token method = expr->as.superExpr.method;
    compiler->token = method;
    uint8_t name = make_identifier_constant(compiler, method);

    named_variable(compiler, synthetic_token("this"), LOAD);
    named_variable(compiler, synthetic_token("super"), LOAD);
    emit_bytes(compiler, OP_GET_SUPER, name);
}

static void compile_assignment_target(Compiler* compiler, AssignmentTarget* target)
{
    if (target->type == VAR_UNPACK) {
        uint8_t count = (uint8_t)ast_expression_list_length(target->as.unpack);

        emit_byte(compiler, OP_DUP);
        emit_bytes(compiler, OP_TUPLE_UNPACK, count);

        for (ExpressionList* current = ast_expression_list_end(target->as.unpack); current != NULL; current = current->prev) {
            compile_expression(compiler, current->expression);
            emit_byte(compiler, OP_POP);
        }
    } else {
        compile_expression(compiler, target->as.single);
    }
}

void compile_assignment_expr(Compiler* compiler, Expression* expr)
{
    compile_expression(compiler, expr->as.assignmentExpr.value);
    compile_assignment_target(compiler, expr->as.assignmentExpr.target);
}

static OpCode compound_opcode(Token op)
{
    switch (op.type) {
        case TOKEN_PLUS_EQUAL: return OP_ADD;
        case TOKEN_MINUS_EQUAL: return OP_SUBTRACT;
        case TOKEN_STAR_EQUAL: return OP_MULTIPLY;
        case TOKEN_SLASH_EQUAL: return OP_DIVIDE;
        case TOKEN_PERCENT_EQUAL: return OP_MODULO;
        case TOKEN_DOUBLE_STAR_EQUAL: return OP_POWER;
        case TOKEN_AMPERSAND_EQUAL: return OP_BITWISE_AND;
        case TOKEN_PIPE_EQUAL: return OP_BITWISE_OR;
        case TOKEN_CARET_EQUAL: return OP_BITWISE_XOR;
        case TOKEN_L_SHIFT_EQUAL: return OP_BITWISE_LEFT_SHIFT;
        case TOKEN_R_SHIFT_EQUAL: return OP_BITWISE_RIGHT_SHIFT;
        default: return -1;
    }
}

static void compile_compound_identifier_assignment(Compiler* compiler, Expression* expr, Expression* target)
{
    Token identifier = target->as.identifierExpr.identifier;
    compiler->token = identifier;
    named_variable(compiler, identifier, LOAD);

    compile_expression(compiler, expr->as.compoundAssignmentExpr.value);

    Token op = expr->as.compoundAssignmentExpr.op;
    compiler->token = op;
    emit_byte(compiler, compound_opcode(op));

    named_variable(compiler, identifier, STORE);
}

static void compile_compound_property_assignment(Compiler* compiler, Expression* expr, Expression* target)
{
    compile_expression(compiler, target->as.propertyExpr.object);
    emit_byte(compiler, OP_DUP);

    bool safe = target->as.propertyExpr.safe;

    Token property = target->as.propertyExpr.property;
    compiler->token = property;
    uint8_t name = make_identifier_constant(compiler, property);
    emit_bytes(compiler, safe ? OP_LOAD_PROPERTY_SAFE : OP_LOAD_PROPERTY, name);

    compile_expression(compiler, expr->as.compoundAssignmentExpr.value);

    Token op = expr->as.compoundAssignmentExpr.op;
    compiler->token = op;
    emit_byte(compiler, compound_opcode(op));

    emit_byte(compiler, OP_SWAP);
    emit_bytes(compiler, safe ? OP_STORE_PROPERTY_SAFE : OP_STORE_PROPERTY, name);
}

static void compile_compound_subscript_assignment(Compiler* compiler, Expression* expr, Expression* target)
{
    compile_expression(compiler, target->as.subscriptExpr.object);
    compile_expression(compiler, target->as.subscriptExpr.index);
    emit_byte(compiler, OP_DUP_TWO);

    bool safe = target->as.subscriptExpr.safe;
    emit_byte(compiler, safe ? OP_LOAD_SUBSCRIPT_SAFE : OP_LOAD_SUBSCRIPT);

    compile_expression(compiler, expr->as.compoundAssignmentExpr.value);

    Token op = expr->as.compoundAssignmentExpr.op;
    compiler->token = op;
    emit_byte(compiler, compound_opcode(op));

    emit_byte(compiler, OP_SWAP_THREE);
    emit_byte(compiler, safe ? OP_STORE_SUBSCRIPT_SAFE : OP_STORE_SUBSCRIPT);
}

void compile_compound_assignment_expr(Compiler* compiler, Expression* expr)
{
    Expression* targetExpression = expr->as.compoundAssignmentExpr.target->as.single;
    switch (targetExpression->type) {
        case EXPR_IDENTIFIER: {
            compile_compound_identifier_assignment(compiler, expr, targetExpression);
            break;
        }
        case EXPR_PROPERTY: {
            compile_compound_property_assignment(compiler, expr, targetExpression);
            break;
        }
        case EXPR_SUBSCRIPT: {
            compile_compound_subscript_assignment(compiler, expr, targetExpression);
            break;
        }
        default: {
            error(compiler, "Invalid compund assignment target.");
        }
    }
}

void compile_coroutine_expr(Compiler* compiler, Expression* expr)
{
    compiler->token = expr->as.coroutineExpr.keyword;

    compile_expression(compiler, expr->as.coroutineExpr.expression);
    emit_byte(compiler, OP_COROUTINE);
}

void compile_yield_expr(Compiler* compiler, Expression* expr)
{
    compiler->token = expr->as.yieldExpr.keyword;

    if (compiler->type == TYPE_SCRIPT || compiler->type == TYPE_INITIALIZER || compiler->type == TYPE_STATIC_INITIALIZER) {
        error(compiler, "Can only yield from non-initializer functions.");
    }

    Expression* value = expr->as.yieldExpr.expression;
    if (value) {
        compile_expression(compiler, value);
    } else {
        emit_byte(compiler, OP_LOAD_NIL);
    }

    emit_byte(compiler, OP_YIELD);
}

static OpCode increment_operation(Token token)
{
    switch (token.type) {
        case TOKEN_DOUBLE_PLUS: return OP_INC;
        case TOKEN_DOUBLE_MINUS: return OP_DEC;
        default: return -1;
    }
}

static void compile_identifier_postfix_inc(Compiler* compiler, Expression* expr)
{
    Expression* target = expr->as.postfixIncExpr.target;

    Token identifier = target->as.identifierExpr.identifier;
    compiler->token = identifier;
    named_variable(compiler, identifier, LOAD);
    emit_byte(compiler, OP_DUP);

    Token op = expr->as.postfixIncExpr.op;
    compiler->token = op;
    emit_byte(compiler, increment_operation(op));

    named_variable(compiler, identifier, STORE);
    emit_byte(compiler, OP_POP);
}

static void compile_property_postfix_inc(Compiler* compiler, Expression* expr)
{
    Expression* target = expr->as.postfixIncExpr.target;

    compile_expression(compiler, target->as.propertyExpr.object);
    emit_byte(compiler, OP_DUP);

    Token property = target->as.propertyExpr.property;
    compiler->token = property;
    uint8_t name = make_identifier_constant(compiler, property);
    emit_bytes(compiler, OP_LOAD_PROPERTY, name);
    emit_bytes(compiler, OP_DUP, OP_SWAP_THREE);

    Token op = expr->as.postfixIncExpr.op;
    compiler->token = op;
    emit_byte(compiler, increment_operation(op));

    emit_byte(compiler, OP_SWAP);
    emit_bytes(compiler, OP_STORE_PROPERTY, name);
    emit_byte(compiler, OP_POP);
}

static void compile_subscript_postfix_inc(Compiler* compiler, Expression* expr)
{
    Expression* target = expr->as.postfixIncExpr.target;

    compile_expression(compiler, target->as.subscriptExpr.object);
    compile_expression(compiler, target->as.subscriptExpr.index);
    emit_byte(compiler, OP_DUP_TWO);

    emit_byte(compiler, OP_LOAD_SUBSCRIPT);
    emit_bytes(compiler, OP_DUP, OP_SWAP_FOUR);

    Token op = expr->as.postfixIncExpr.op;
    compiler->token = op;
    emit_byte(compiler, increment_operation(op));

    emit_byte(compiler, OP_SWAP_THREE);
    emit_byte(compiler, OP_STORE_SUBSCRIPT);
    emit_byte(compiler, OP_POP);
}

void compile_postfix_inc_expr(Compiler* compiler, Expression* expr)
{
    Expression* target = expr->as.postfixIncExpr.target;
    switch (target->type) {
        case EXPR_IDENTIFIER: {
            compile_identifier_postfix_inc(compiler, expr);
            break;
        }
        case EXPR_PROPERTY: {
            compile_property_postfix_inc(compiler, expr);
            break;
        }
        case EXPR_SUBSCRIPT: {
            compile_subscript_postfix_inc(compiler, expr);
            break;
        }
        default: {
            error(compiler, "Invalid assignment target.");
        }
    }
}

static void compile_identifier_prefix_inc(Compiler* compiler, Expression* expr)
{
    Expression* target = expr->as.prefixIncExpr.target;
    Token identifier = target->as.identifierExpr.identifier;
    compiler->token = identifier;

    named_variable(compiler, identifier, LOAD);

    Token op = expr->as.prefixIncExpr.op;
    emit_byte(compiler, increment_operation(op));

    named_variable(compiler, identifier, STORE);
}

static void compile_property_prefix_inc(Compiler* compiler, Expression* expr)
{
    Expression* target = expr->as.prefixIncExpr.target;

    compile_expression(compiler, target->as.propertyExpr.object);
    emit_byte(compiler, OP_DUP);

    Token property = target->as.propertyExpr.property;
    compiler->token = property;
    uint8_t name = make_identifier_constant(compiler, property);
    emit_bytes(compiler, OP_LOAD_PROPERTY, name);

    Token op = expr->as.prefixIncExpr.op;
    emit_byte(compiler, increment_operation(op));

    emit_byte(compiler, OP_SWAP);
    emit_bytes(compiler, OP_STORE_PROPERTY, name);
}

static void compile_subscript_prefix_inc(Compiler* compiler, Expression* expr)
{
    Expression* target = expr->as.prefixIncExpr.target;

    compile_expression(compiler, target->as.subscriptExpr.object);
    compile_expression(compiler, target->as.subscriptExpr.index);
    emit_byte(compiler, OP_DUP_TWO);

    emit_byte(compiler, OP_LOAD_SUBSCRIPT);

    Token op = expr->as.prefixIncExpr.op;
    emit_byte(compiler, increment_operation(op));

    emit_byte(compiler, OP_SWAP_THREE);
    emit_byte(compiler, OP_STORE_SUBSCRIPT);
}

void compile_prefix_inc_expr(Compiler* compiler, Expression* expr)
{
    Expression* target = expr->as.prefixIncExpr.target;
    switch (target->type) {
        case EXPR_IDENTIFIER: {
            compile_identifier_prefix_inc(compiler, expr);
            break;
        }
        case EXPR_PROPERTY: {
            compile_property_prefix_inc(compiler, expr);
            break;
        }
        case EXPR_SUBSCRIPT: {
            compile_subscript_prefix_inc(compiler, expr);
            break;
        }
        default: {
            error(compiler, "Invalid assignment target.");
        }
    }
}

static void compile_and(Compiler* compiler, Expression* expr)
{
    compile_expression(compiler, expr->as.logicalExpr.left);

    Token op = expr->as.logicalExpr.op;
    compiler->token = op;
    size_t endJump = emit_jump(compiler, OP_JUMP_IF_FALSE);

    emit_byte(compiler, OP_POP);
    compile_expression(compiler, expr->as.logicalExpr.right);

    patch_jump(compiler, endJump);
}

static void compile_or(Compiler* compiler, Expression* expr)
{
    compile_expression(compiler, expr->as.logicalExpr.left);

    Token op = expr->as.logicalExpr.op;
    compiler->token = op;
    size_t elseJump = emit_jump(compiler, OP_JUMP_IF_FALSE);
    size_t endJump = emit_jump(compiler, OP_JUMP);

    patch_jump(compiler, elseJump);
    emit_byte(compiler, OP_POP);

    compile_expression(compiler, expr->as.logicalExpr.right);
    patch_jump(compiler, endJump);
}

void compile_logical_expr(Compiler* compiler, Expression* expr)
{
    Token op = expr->as.logicalExpr.op;
    switch (op.type) {
        case TOKEN_AND: compile_and(compiler, expr); return;
        case TOKEN_OR: compile_or(compiler, expr); return;
    }
}

void compile_conditional_expr(Compiler* compiler, Expression* expr)
{
    Expression* condition = expr->as.conditionalExpr.condition;
    compile_expression(compiler, condition);

    size_t elseJump = emit_jump(compiler, OP_POP_JUMP_IF_FALSE);

    Expression* thenBranch = expr->as.conditionalExpr.thenBranch;
    compile_expression(compiler, thenBranch);

    size_t endJump = emit_jump(compiler, OP_JUMP);
    patch_jump(compiler, elseJump);

    Expression* elseBranch = expr->as.conditionalExpr.elseBranch;
    compile_expression(compiler, elseBranch);

    patch_jump(compiler, endJump);
}

void compile_elvis_expr(Compiler* compiler, Expression* expr)
{
    Expression* left = expr->as.elvisExpr.left;
    compile_expression(compiler, left);

    size_t elseJump = emit_jump(compiler, OP_JUMP_IF_NOT_NIL);
    emit_byte(compiler, OP_POP);

    Expression* right = expr->as.elvisExpr.right;
    compile_expression(compiler, right);

    patch_jump(compiler, elseJump);
}

void compile_binary_expr(Compiler* compiler, Expression* expr)
{
    compile_expression(compiler, expr->as.binaryExpr.left);
    compile_expression(compiler, expr->as.binaryExpr.right);

    Token op = expr->as.binaryExpr.op;
    compiler->token = op;
    switch (op.type) {
        case TOKEN_BANG_EQUAL: emit_byte(compiler, OP_NOT_EQUAL); return;
        case TOKEN_EQUAL_EQUAL: emit_byte(compiler, OP_EQUAL); return;
        case TOKEN_GREATER: emit_byte(compiler, OP_GREATER); return;
        case TOKEN_GREATER_EQUAL: emit_byte(compiler, OP_GREATER_EQUAL); return;
        case TOKEN_LESS: emit_byte(compiler, OP_LESS); return;
        case TOKEN_LESS_EQUAL: emit_byte(compiler, OP_LESS_EQUAL); return;
        case TOKEN_PLUS: emit_byte(compiler, OP_ADD); return;
        case TOKEN_MINUS: emit_byte(compiler, OP_SUBTRACT); return;
        case TOKEN_STAR: emit_byte(compiler, OP_MULTIPLY); return;
        case TOKEN_SLASH: emit_byte(compiler, OP_DIVIDE); return;
        case TOKEN_PERCENT: emit_byte(compiler, OP_MODULO); return;
        case TOKEN_DOUBLE_STAR: emit_byte(compiler, OP_POWER); return;
        case TOKEN_AMPERSAND: emit_byte(compiler, OP_BITWISE_AND); return;
        case TOKEN_PIPE: emit_byte(compiler, OP_BITWISE_OR); return;
        case TOKEN_CARET: emit_byte(compiler, OP_BITWISE_XOR); return;
        case TOKEN_L_SHIFT: emit_byte(compiler, OP_BITWISE_LEFT_SHIFT); return;
        case TOKEN_R_SHIFT: emit_byte(compiler, OP_BITWISE_RIGHT_SHIFT); return;
    }
}

void compile_unary_expr(Compiler* compiler, Expression* expr)
{
    compile_expression(compiler, expr->as.unaryExpr.expression);

    Token op = expr->as.unaryExpr.op;
    compiler->token = op;
    switch (op.type) {
        case TOKEN_BANG: emit_byte(compiler, OP_NOT); return;
        case TOKEN_MINUS: emit_byte(compiler, OP_NEGATE); return;
        case TOKEN_TILDE: emit_byte(compiler, OP_BITWISE_NOT); return;
    }
}

static void compile_number_literal(Compiler* compiler, Token literal)
{
    Value value = NUMBER_VAL(strtod(literal.start, NULL));
    emit_constant(compiler, value);
}

static char complete_escape_sequence(const char* start)
{
    switch (*start) {
        case 'a': return '\a';
        case 'b': return '\b';
        case 'f': return '\f';
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case 'v': return '\v';
        case '\\': return '\\';
        case '\'': return '\'';
        case '\"': return '\"';
        case '$': return '$';
    }

    return -1;
}

static void compile_string_literal(Compiler* compiler, Token literal)
{
    const char* end = literal.start + literal.length;

    size_t bufferLength = literal.length;
    for (const char* current = literal.start; current < end; current++) {
        if (*current == '\\') {
            bufferLength--;
            ++current;
        }
    }

    char* stringBuffer = malloc(bufferLength);

    size_t i = 0;
    for (const char* current = literal.start; current < end; current++) {
        if (*current == '\\') {
            stringBuffer[i] = complete_escape_sequence(++current);
        } else {
            stringBuffer[i] = *current;
        }
        i++;
    }

    ObjectString* string = String_Copy(compiler->vm, stringBuffer, bufferLength);
    free(stringBuffer);

    emit_constant(compiler, OBJ_VAL(string));
}

static void compile_this_literal(Compiler* compiler, Token literal)
{
    if (!compiler->vm->classCompiler) {
        error(compiler, "Cannot use 'this' outside of a class.");
        return;
    }

    named_variable(compiler, literal, LOAD);
}

static void compile_language_literal(Compiler* compiler, Token literal)
{
    switch (literal.type) {
        case TOKEN_TRUE: emit_byte(compiler, OP_LOAD_TRUE); return;
        case TOKEN_FALSE: emit_byte(compiler, OP_LOAD_FALSE); return;
        case TOKEN_NIL: emit_byte(compiler, OP_LOAD_NIL); return;
        case TOKEN_THIS: compile_this_literal(compiler, literal); return;
    }
}

void compile_literal_expr(Compiler* compiler, Expression* expr)
{
    Token value = expr->as.literalExpr.value;
    compiler->token = value;
    switch (value.type) {
        case TOKEN_NUMBER: compile_number_literal(compiler, value); return;
        case TOKEN_STRING:
        case TOKEN_STRING_INTERP_BEGIN:
        case TOKEN_STRING_INTERP:
        case TOKEN_STRING_INTERP_END: compile_string_literal(compiler, value); return;
        default: compile_language_literal(compiler, value); return;
    }
}

void compile_string_interp_expr(Compiler* compiler, Expression* expr)
{
    size_t count = compile_expression_list(compiler, expr->as.stringInterpExpr.values);
    if (count > 255) {
        error(compiler, "Cannot interpolate more than 255 strings.");
    }

    emit_bytes(compiler, OP_BUILD_STRING, (uint8_t)count);
}

void compile_range_expr(Compiler* compiler, Expression* expr)
{
    compile_expression(compiler, expr->as.rangeExpr.begin);
    compile_expression(compiler, expr->as.rangeExpr.end);

    if (expr->as.rangeExpr.step) {
        compile_expression(compiler, expr->as.rangeExpr.step);
    } else {
        emit_constant(compiler, NUMBER_VAL(1.0f));
    }

    emit_byte(compiler, OP_RANGE);
}

void compile_lambda_expr(Compiler* compiler, Expression* expr)
{
    compile_function(compiler, expr->as.lambdaExpr.function, TYPE_LAMBDA, empty_token(), false);
}

void compile_list_expr(Compiler* compiler, Expression* expr)
{
    size_t count = compile_expression_list(compiler, expr->as.listExpr.elements);
    if (count > 255) {
        error(compiler, "Cannot have more than 255 elements in a list expression.");
    }
    emit_bytes(compiler, OP_LIST, (uint8_t)count);
}

void compile_map_expr(Compiler* compiler, Expression* expr)
{
    size_t count = compile_map_entry_list(compiler, expr->as.mapExpr.entries);
    if (count > 255) {
        error(compiler, "Cannot have more than 255 entries in a map expression.");
    }
    emit_bytes(compiler, OP_MAP, (uint8_t)count);
}

void compile_tuple_expr(Compiler* compiler, Expression* expr)
{
    size_t count = compile_expression_list(compiler, expr->as.tupleExpr.elements);
    if (count > 255) {
        error(compiler, "Cannot have more than 255 elements in a tuple expression.");
    }
    emit_bytes(compiler, OP_TUPLE, (uint8_t)count);
}

void compile_identifier_expr(Compiler* compiler, Expression* expr)
{
    Token identifier = expr->as.identifierExpr.identifier;
    compiler->token = identifier;

    ExprContext context = expr->as.identifierExpr.context;
    named_variable(compiler, identifier, context);
}

void compile_when_entry(Compiler* compiler, WhenEntry* entry)
{
    ControlBreak* caseJumps = NULL;

    ExpressionList* currentCase = entry->cases;
    while (currentCase) {
        compile_expression(compiler, currentCase->expression);
        size_t address = emit_jump(compiler, OP_POP_JUMP_IF_EQUAL);
        caseJumps = make_control_break(compiler, address, caseJumps);
        currentCase = currentCase->next;
    }

    size_t nextEntry = emit_jump(compiler, OP_JUMP);

    patch_breaks(compiler, caseJumps);
    emit_byte(compiler, OP_POP);
    compile_statement(compiler, entry->body);

    size_t address = emit_jump(compiler, OP_JUMP);
    push_control_break(compiler, address);

    patch_jump(compiler, nextEntry);
}

size_t compile_when_entry_list(Compiler* compiler, WhenEntryList* list)
{
    WhenEntryList* current = list;
    size_t count = 0;

    while (current) {
        compile_when_entry(compiler, current->entry);
        count++;
        current = current->next;
    }

    return count;
}

void compile_map_entry(Compiler* compiler, MapEntry* entry)
{
    compile_expression(compiler, entry->key);
    compile_expression(compiler, entry->value);
}

size_t compile_map_entry_list(Compiler* compiler, MapEntryList* list)
{
    MapEntryList* current = list;
    size_t count = 0;

    while (current) {
        compile_map_entry(compiler, current->entry);
        count++;
        current = current->next;
    }

    return count;
}

void compile_block(Compiler* compiler, Block* block)
{
    compile_declaration_list(compiler, block->body);
}

size_t compile_argument_list(Compiler* compiler, ArgumentList* list)
{
    ArgumentList* current = list;
    size_t count = 0;

    while (current) {
        Expression* argument = current->expression;
        compile_expression(compiler, argument);

        count++;
        if (count > 255) {
            error(compiler, "Cannot have more than 255 arguments.");
        }

        current = current->next;
    }

    return count;
}

size_t compile_expression_list(Compiler* compiler, ExpressionList* list)
{
    ExpressionList* current = list;
    size_t count = 0;

    while (current) {
        compile_expression(compiler, current->expression);
        count++;

        current = current->next;
    }

    return count;
}

size_t compile_parameter_list(Compiler* compiler, ParameterList* list)
{
    ParameterList* current = list;
    size_t count = 0;
    while (current) {
        Token parameter = current->parameter;
        compiler->token = parameter;

        uint8_t index = declare_variable(compiler, parameter);
        define_variable(compiler, index);

        count++;
        if (count > 255) {
            error(compiler, "Cannot have more than 255 parameters.");
        }

        current = current->next;
    }

    return count;
}

void compile_function_body(Compiler* compiler, FunctionBody* body)
{
    switch (body->notation) {
        case FUNC_EXPRESSION: {
            if (compiler->type == TYPE_INITIALIZER || compiler->type == TYPE_STATIC_INITIALIZER) {
                error(compiler, "Initializer cannot be an expression.");
            }

            compile_expression(compiler, body->as.expression);
            emit_byte(compiler, OP_RETURN);
            break;
        }
        case FUNC_BLOCK: {
            begin_scope(compiler);
            compile_block(compiler, body->as.block);
            end_scope(compiler);
            break;
        }
    }
}

void compile_function(Compiler* compiler, Function* function, CompilerType type, Token identifier, bool coroutine)
{
    Compiler newCompiler;
    compiler_init(&newCompiler, compiler->vm, type, identifier, compiler->mod);
    begin_scope(&newCompiler);

    newCompiler.function->arity = (int)compile_parameter_list(&newCompiler, function->parameters);
    if (type == TYPE_STATIC_INITIALIZER && newCompiler.function->arity > 0) {
        error(compiler, "Static initializer cannot accept parameters.");
    }

    if (coroutine && (type == TYPE_INITIALIZER || type == TYPE_STATIC_INITIALIZER)) {
        error(compiler, "Initializer cannot be a coroutine.");
    }

    compile_function_body(&newCompiler, function->body);

    ObjectFunction* compiled = finish_compilation(newCompiler.vm);

    emit_bytes(compiler, OP_CLOSURE, make_constant(compiler, OBJ_VAL(compiled)));
    for (size_t i = 0; i < compiled->upvalueCount; i++) {
        emit_byte(compiler, newCompiler.upvalues[i].isLocal ? 1 : 0);
        emit_byte(compiler, newCompiler.upvalues[i].index);
    }

    if (coroutine) {
        emit_byte(compiler, OP_COROUTINE);
    }
}

void compile_named_function(Compiler* compiler, NamedFunction* namedFunction, CompilerType type)
{
    compile_function(compiler, namedFunction->function, type, namedFunction->identifier, namedFunction->coroutine);
}

size_t compile_method_list(Compiler* compiler, MethodList* list)
{
    MethodList* current = list;
    size_t count = 0;
    while (current) {
        compile_method(compiler, current->method);
        current = current->next;
        count++;
    }
    return count;
}

size_t compile_declaration_list(Compiler* compiler, DeclarationList* list)
{
    DeclarationList* current = list;
    size_t count = 0;
    while (current) {
        compile_declaration(compiler, current->declaration);
        current = current->next;
    }
    return count;
}

ObjectFunction* compile(VM* vm, const char* source, ObjectModule* mod)
{
    vm->compiler = NULL;
    vm->classCompiler = NULL;

    AST* ast = parse(source);
    if (!ast) {
        return NULL;
    }

#if DEBUG_PRINT_AST
    print_ast(ast);
#endif

    Compiler compiler;
    compiler.vm = vm;
    compiler_init(&compiler, vm, TYPE_SCRIPT, empty_token(), mod);

    compile_tree(&compiler, ast);
    ObjectFunction* function = finish_compilation(vm);

    ast_delete_tree(ast);

    return compiler.error ? NULL : function;
}

void mark_compiler_roots(VM* vm)
{
    GC* gc = &vm->gc;
    for (Compiler* compiler = vm->compiler; compiler != NULL; compiler = compiler->enclosing) {
        GC_MarkObject(gc, (Object*)compiler->function);
    }
}
