#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "compiler.h"
#include "vm.h"
#include "common.h"
#include "chunk.h"
#include "parser.h"
#include "memory.h"

#if DEBUG_PRINT_CODE
#include "debug.h"
#endif

#if DEBUG_PRINT_AST
#include "astprinter.h"
#endif

typedef struct {
    Token identifier;
    int depth;
    bool isCaptured;
} Local;

typedef struct {
    uint32_t index;
    bool isLocal;
} Upvalue;

typedef enum {
    TYPE_FUNCTION,
    TYPE_METHOD,
    TYPE_INITIALIZER,
    TYPE_SCRIPT
} FunctionType;

typedef struct Compiler {
    VM* vm;

    struct Compiler* enclosing;

    ObjFunction* function;
    FunctionType type;

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
static void compile_class_decl(Compiler* compiler, Declaration* decl);
static void compile_function_decl(Compiler* compiler, Declaration* decl);
static void compile_variable_decl(Compiler* compiler, Declaration* decl);
static void compile_statement_decl(Compiler* compiler, Declaration* decl);

static void compile_statement(Compiler* compiler, Statement* stmt);
static void compile_for_stmt(Compiler* compiler, Statement* stmt);
static void compile_while_stmt(Compiler* compiler, Statement* stmt);
static void compile_if_stmt(Compiler* compiler, Statement* stmt);
static void compile_return_stmt(Compiler* compiler, Statement* stmt);
static void compile_print_stmt(Compiler* compiler, Statement* stmt);
static void compile_block_stmt(Compiler* compiler, Statement* stmt);
static void compile_expression_stmt(Compiler* compiler, Statement* stmt);

static void compile_expression(Compiler* compiler, Expression* expr);
static void compile_call_expr(Compiler* compiler, Expression* expr);
static void compile_property_expr(Compiler* compiler, Expression* expr);
static void compile_super_expr(Compiler* compiler, Expression* expr);
static void compile_assignment_expr(Compiler* compiler, Expression* expr);
static void compile_compound_assignment_expr(Compiler* compiler, Expression* expr);
static void compile_logical_expr(Compiler* compiler, Expression* expr);
static void compile_binary_expr(Compiler* compiler, Expression* expr);
static void compile_unary_expr(Compiler* compiler, Expression* expr);
static void compile_literal_expr(Compiler* compiler, Expression* expr);
static void compile_identifier_expr(Compiler* compiler, Expression* expr);

static void compile_argument_list(Compiler* compiler, ArgumentList* list);
static void compile_parameter_list(Compiler* compiler, ParameterList* list);
static void compile_function(Compiler* compiler, Function* function, FunctionType type);
static void compile_function_list(Compiler* compiler, FunctionList* list);
static void compile_declaration_list(Compiler* compiler, DeclarationList* list);

static void compiler_init(Compiler* compiler, VM* vm, FunctionType type, Token identifier)
{
    compiler->enclosing = vm->compiler;
    vm->compiler = compiler;

    if (compiler->enclosing) {
        compiler->vm = compiler->enclosing->vm;
    }

    compiler->function = NULL;
    compiler->type = type;

    compiler->localCount = 0;
    compiler->scopeDepth = 0;

    compiler->token = empty_token();

    compiler->function = new_function(vm);

    if (type != TYPE_SCRIPT) {
        compiler->token = identifier;
        compiler->function->name = copy_string(vm, identifier.start, identifier.length);
    }

    Local* local = &compiler->locals[compiler->localCount++];
    local->depth = 0;
    local->isCaptured = false;

    if (type != TYPE_FUNCTION) {
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
    if (compiler->type == TYPE_INITIALIZER) {
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

static void patch_jump(Compiler* compiler, size_t offset)
{
    size_t jump = current_chunk(compiler)->count - offset - 2;

    if (jump > UINT16_MAX) {
        error(compiler, "Too much code to jump over.");
    }

    current_chunk(compiler)->code[offset    ] = (jump >> 0) & 0xFF;
    current_chunk(compiler)->code[offset + 1] = (jump >> 8) & 0xFF;
}

static void emit_loop(Compiler* compiler, size_t loopStart)
{
    emit_byte(compiler, OP_LOOP);

    size_t offset = current_chunk(compiler)->count - loopStart + 2;
    if (offset > UINT16_MAX) {
        error(compiler, "Loop body is too large.");
    }

    emit_byte(compiler, (offset >> 0) & 0xFF);
    emit_byte(compiler, (offset >> 8) & 0xFF);
}

static ObjFunction* finish_compilation(VM* vm)
{
    emit_return(vm->compiler);
    ObjFunction* function = vm->compiler->function;

#if DEBUG_PRINT_CODE
    if (!vm->compiler->error) {
        disassemble_chunk(current_chunk(vm->compiler), function->name != NULL ? function->name->chars : "<script>");
    }
#endif

    vm->compiler = vm->compiler->enclosing;
    return function;
}

static void begin_scope(Compiler* compiler)
{
    compiler->scopeDepth++;
}

static void end_scope(Compiler* compiler)
{
    compiler->scopeDepth--;

    while (compiler->localCount > 0 && compiler->locals[compiler->localCount - 1].depth > compiler->scopeDepth) {
        if (compiler->locals[compiler->localCount - 1].isCaptured) {
            emit_byte(compiler, OP_CLOSE_UPVALUE);
        } else {
            emit_byte(compiler, OP_POP);
        }

        compiler->localCount--;
    }
}

static void mark_initialized(Compiler* compiler)
{
    if (compiler->scopeDepth == 0) {
        return;
    }

    compiler->locals[compiler->localCount - 1].depth = compiler->scopeDepth;
}

static void define_variable(Compiler* compiler, uint8_t global)
{
    if (compiler->scopeDepth > 0) {
        mark_initialized(compiler);
        return;
    }

    emit_byte(compiler, OP_DEFINE_GLOBAL);
    emit_byte(compiler, global);
}

static void add_local(Compiler* compiler, Token identifier)
{
    if (compiler->localCount == UINT8_COUNT) {
        error(compiler, "Too many local variables in function.");
        return;
    }

    Local* local = &compiler->locals[compiler->localCount++];
    local->identifier = identifier;
    local->depth = -1;
    local->isCaptured = false;
}

static int resolve_local(Compiler* compiler, Token* identifier)
{
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];

        if (lexemes_equal(identifier, &local->identifier)) {
            if (local->depth == -1) {
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
        compiler->enclosing->locals[local].isCaptured = true;
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
        if (local->depth != -1 && local->depth < compiler->scopeDepth) {
            break;
        }

        if (lexemes_equal(&identifier, &local->identifier)) {
            error(compiler, "Variable with this name already declared in this scope.");
        }
    }

    add_local(compiler, identifier);
}

static uint8_t identifier_constant(Compiler* compiler, Token identifier)
{
    return make_constant(compiler, OBJ_VAL(copy_string(compiler->vm, identifier.start, identifier.length)));
}

static uint8_t declare_variable(Compiler* compiler, Token identifier)
{
    declare_local_variable(compiler, identifier);
    if (compiler->scopeDepth > 0) {
        return 0;
    }

    return identifier_constant(compiler, identifier);
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
        scope = identifier_constant(compiler, identifier);
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
        case DECL_CLASS: compile_class_decl(compiler, decl); return;
        case DECL_FUNCTION: compile_function_decl(compiler, decl); return;
        case DECL_VARIABLE: compile_variable_decl(compiler, decl); return;
        case DECL_STATEMENT: compile_statement_decl(compiler, decl); return;
    }
}

static void compile_method(Compiler* compiler, Function* function)
{
    Token identifier = function->identifier;
    compiler->token = identifier;
    uint8_t constant = identifier_constant(compiler, identifier);

    FunctionType type = TYPE_METHOD;
    if (identifier.length == 4 && memcmp(identifier.start, "init", 4) == 0) {
        type = TYPE_INITIALIZER;
    }

    compile_function(compiler, function, type);
    emit_bytes(compiler, OP_METHOD, constant);
}

void compile_class_decl(Compiler* compiler, Declaration* decl)
{
    Token identifier = decl->as.classDecl.identifier;
    compiler->token = identifier;
    uint8_t nameConstant = identifier_constant(compiler, identifier);

    declare_local_variable(compiler, identifier);

    emit_bytes(compiler, OP_CLASS, nameConstant);
    define_variable(compiler, nameConstant);

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

    FunctionList* current = decl->as.classDecl.body;
    while (current) {
        compile_method(compiler, current->function);
        current = current->next;
    }

    emit_byte(compiler, OP_POP);

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
    mark_initialized(compiler);
    compile_function(compiler, decl->as.functionDecl.function, TYPE_FUNCTION);
    define_variable(compiler, global);
}

void compile_variable_decl(Compiler* compiler, Declaration* decl)
{
    Token identifier = decl->as.variableDecl.identifier;
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

void compile_statement_decl(Compiler* compiler, Declaration* decl)
{
    compile_statement(compiler, decl->as.statement);
}

void compile_statement(Compiler* compiler, Statement* stmt)
{
    switch (stmt->type) {
        case STMT_FOR: compile_for_stmt(compiler, stmt); return;
        case STMT_WHILE: compile_while_stmt(compiler, stmt); return;
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

        exitJump = emit_jump(compiler, OP_JUMP_IF_FALSE);
        emit_byte(compiler, OP_POP);
    }

    Expression* increment = stmt->as.forStmt.increment;
    if (increment) {
        size_t bodyJump = emit_jump(compiler, OP_JUMP);

        size_t incrementStart = current_chunk(compiler)->count;
        compile_expression(compiler, increment);
        emit_byte(compiler, OP_POP);

        emit_loop(compiler, loopStart);
        loopStart = incrementStart;
        patch_jump(compiler, bodyJump);
    }

    Statement* body = stmt->as.forStmt.body;
    compile_statement(compiler, body);

    emit_loop(compiler, loopStart);

    if (exitJump != -1) {
        patch_jump(compiler, exitJump);
        emit_byte(compiler, OP_POP);
    }

    end_scope(compiler);
}

void compile_while_stmt(Compiler* compiler, Statement* stmt)
{
    size_t loopStart = current_chunk(compiler)->count;

    Expression* condition = stmt->as.whileStmt.condition;
    compile_expression(compiler, condition);

    size_t exitJump = emit_jump(compiler, OP_JUMP_IF_FALSE);
    emit_byte(compiler, OP_POP);

    Statement* body = stmt->as.whileStmt.body;
    compile_statement(compiler, body);

    emit_loop(compiler, loopStart);

    patch_jump(compiler, exitJump);
    emit_byte(compiler, OP_POP);
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
    if (compiler->type == TYPE_SCRIPT) {
        error(compiler, "Can only return from functions.");
    }

    Expression* value = stmt->as.returnStmt.expression;
    if (value) {
        if (compiler->type == TYPE_INITIALIZER) {
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
    compile_declaration_list(compiler, stmt->as.blockStmt.body);
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
        case EXPR_SUPER: compile_super_expr(compiler, expr); return;
        case EXPR_ASSIGNMENT: compile_assignment_expr(compiler, expr); return;
        case EXPR_COMPOUND_ASSIGNMNET: compile_compound_assignment_expr(compiler, expr); return;
        case EXPR_LOGICAL: compile_logical_expr(compiler, expr); return;
        case EXPR_BINARY: compile_binary_expr(compiler, expr); return;
        case EXPR_UNARY: compile_unary_expr(compiler, expr); return;
        case EXPR_LITERAL: compile_literal_expr(compiler, expr); return;
        case EXPR_IDENTIFIER: compile_identifier_expr(compiler, expr); return;
    }
}

void compile_call_expr(Compiler* compiler, Expression* expr)
{
    compile_expression(compiler, expr->as.callExpr.callee);

    ArgumentList* arguments = expr->as.callExpr.arguments;
    uint8_t argumentCount = (uint8_t)ast_argument_list_length(arguments);
    compile_argument_list(compiler, arguments);

    emit_bytes(compiler, OP_CALL, argumentCount);
}

void compile_property_expr(Compiler* compiler, Expression* expr)
{
    Expression* object = expr->as.propertyExpr.object;
    compile_expression(compiler, object);

    Token property = expr->as.propertyExpr.property;
    compiler->token = property;
    uint8_t name = identifier_constant(compiler, property);

    ExprContext context = expr->as.propertyExpr.context;
    uint8_t operation = context == LOAD ? OP_LOAD_PROPERTY : OP_STORE_PROPERTY;
    emit_bytes(compiler, operation, name);
}

void compile_super_expr(Compiler* compiler, Expression* expr)
{
    Token keyword = expr->as.superExpr.keyword;
    compiler->token = keyword;

    if (!compiler->vm->classCompiler) {
        error(compiler, "Cannot use 'super' outside of a class.");
    } else if (!compiler->vm->classCompiler->hasSuperclass) {
        error(compiler, "Cannot use 'super' in a class with no superclass.");
    }

    Token method = expr->as.superExpr.method;
    compiler->token = method;
    uint8_t name = identifier_constant(compiler, method);

    named_variable(compiler, synthetic_token("this"), LOAD);
    named_variable(compiler, synthetic_token("super"), LOAD);
    emit_bytes(compiler, OP_GET_SUPER, name);
}

void compile_assignment_expr(Compiler* compiler, Expression* expr)
{
    compile_expression(compiler, expr->as.assignmentExpr.value);
    compile_expression(compiler, expr->as.assignmentExpr.target);
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

void compile_compound_assignment_expr(Compiler* compiler, Expression* expr)
{
    Expression* target = expr->as.compoundAssignmentExpr.target;
    switch (target->type) {
        case EXPR_IDENTIFIER: {
            named_variable(compiler, target->as.identifierExpr.identifier, LOAD);
            compile_expression(compiler, expr->as.compoundAssignmentExpr.value);

            Token op = expr->as.compoundAssignmentExpr.op;
            compiler->token = op;
            emit_byte(compiler, compound_opcode(op));

            named_variable(compiler, target->as.identifierExpr.identifier, STORE);
            break;
        }
        case EXPR_PROPERTY: {
            compile_expression(compiler, target->as.propertyExpr.object);
            emit_byte(compiler, OP_DUP);
            
            Token property = target->as.propertyExpr.property;
            compiler->token = property;
            uint8_t name = identifier_constant(compiler, property);
            emit_bytes(compiler, OP_LOAD_PROPERTY, name);

            compile_expression(compiler, expr->as.compoundAssignmentExpr.value);

            Token op = expr->as.compoundAssignmentExpr.op;
            compiler->token = op;
            emit_byte(compiler, compound_opcode(op));

            emit_byte(compiler, OP_SWAP);
            emit_bytes(compiler, OP_STORE_PROPERTY, name);
            break;
        }
        default: {
            error(compiler, "Invalid compund assignment target.");
        }
    }
}

static void compile_and(Compiler* compiler, Expression* expr)
{
    compile_expression(compiler, expr->as.logicalExpr.left);

    Token op = expr->as.logicalExpr.op;
    compiler->token = op;
    size_t endJump = emit_jump(compiler, OP_JUMP_IF_FALSE, op);

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
    compiler->token = literal;
    Value value = NUMBER_VAL(strtod(literal.start, NULL));
    emit_constant(compiler, value);
}

static void compile_string_literal(Compiler* compiler, Token literal)
{
    compiler->token = literal;
    Value value = OBJ_VAL(copy_string(compiler->vm, literal.start + 1, literal.length - 2));
    emit_constant(compiler, value);
}

static void compile_this_literal(Compiler* compiler, Token literal)
{
    compiler->token = literal;
    if (!compiler->vm->classCompiler) {
        error(compiler, "Cannot use 'this' outside of a class.");
        return;
    }

    named_variable(compiler, literal, LOAD);
}

static void compile_language_literal(Compiler* compiler, Token literal)
{
    switch (literal.type) {
        case TOKEN_TRUE: emit_byte(compiler, OP_LOAD_TRUE, literal); return;
        case TOKEN_FALSE: emit_byte(compiler, OP_LOAD_FALSE, literal); return;
        case TOKEN_NIL: emit_byte(compiler, OP_LOAD_NIL, literal); return;
        case TOKEN_THIS: compile_this_literal(compiler, literal); return;
    }
}

void compile_literal_expr(Compiler* compiler, Expression* expr)
{
    Token value = expr->as.literalExpr.value;
    switch (value.type) {
        case TOKEN_NUMBER: compile_number_literal(compiler, value); return;
        case TOKEN_STRING: compile_string_literal(compiler, value); return;
        default: compile_language_literal(compiler, value); return;
    }
}

void compile_identifier_expr(Compiler* compiler, Expression* expr)
{
    Token identifier = expr->as.identifierExpr.identifier;
    compiler->token = identifier;

    ExprContext context = expr->as.identifierExpr.context;
    named_variable(compiler, identifier, context);
}

void compile_argument_list(Compiler* compiler, ArgumentList* list)
{
    ArgumentList* current = list;
    size_t count = 0;

    while (current) {
        Expression* argument = current->expression;
        compile_expression(compiler, argument);

        count++;
        if (count > 255) {
            error(compiler, "Cannot have more than 255 parameters.");
        }

        current = current->next;
    }
}

void compile_parameter_list(Compiler* compiler, ParameterList* list)
{
    ParameterList* current = list;
    while (current) {
        Token parameter = current->parameter;
        compiler->token = parameter;

        compiler->function->arity++;
        if (compiler->function->arity > 255) {
            error(compiler, "Cannot have more than 255 parameters.");
        }

        uint8_t index = declare_variable(compiler, parameter);
        define_variable(compiler, index);

        current = current->next;
    }
}

void compile_function(Compiler* compiler, Function* function, FunctionType type)
{
    Compiler newCompiler;
    compiler->token = function->identifier;
    compiler_init(&newCompiler, compiler->vm, type, function->identifier);
    begin_scope(&newCompiler);

    compile_parameter_list(&newCompiler, function->parameters);

    begin_scope(&newCompiler);
    compile_declaration_list(&newCompiler, function->body);
    end_scope(&newCompiler);

    ObjFunction* compiled = finish_compilation(newCompiler.vm, empty_token());

    emit_bytes(compiler, OP_CLOSURE, make_constant(compiler, OBJ_VAL(compiled), empty_token()), empty_token());
    for (size_t i = 0; i < compiled->upvalueCount; i++) {
        emit_byte(compiler, newCompiler.upvalues[i].isLocal ? 1 : 0, empty_token());
        emit_byte(compiler, newCompiler.upvalues[i].index, empty_token());
    }
}

void compile_function_list(Compiler* compiler, FunctionList* list)
{
    FunctionList* current = list;
    while (current) {
        compile_function(compiler, current->function, TYPE_METHOD);
        current = current->next;
    }
}

void compile_declaration_list(Compiler* compiler, DeclarationList* list)
{
    DeclarationList* current = list;
    while (current) {
        compile_declaration(compiler, current->declaration);
        current = current->next;
    }
}

ObjFunction* compile(VM* vm, const char* source)
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
    compiler_init(&compiler, vm, TYPE_SCRIPT, empty_token());

    compile_tree(&compiler, ast);
    ObjFunction* function = finish_compilation(vm, empty_token());

    ast_delete_tree(ast);

    return compiler.error ? NULL : function;
}

void mark_compiler_roots(VM* vm)
{
    for (Compiler* compiler = vm->compiler; compiler != NULL; compiler = compiler->enclosing) {
        gc_mark_object(vm, (Obj*)compiler->function);
    }
}
