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

    Parser* parser;

    ObjFunction* function;
    FunctionType type;

    Local locals[UINT8_COUNT];
    int localCount;

    Upvalue upvalues[UINT8_COUNT];
    int scopeDepth;
} Compiler;

typedef struct ClassCompiler {
    struct ClassCompiler* enclosing;
    Token name;
    bool hasSuperclass;
} ClassCompiler;

typedef enum {
    PREC_NONE,
    PREC_COMMA,
    PREC_ASSIGNMENT,
    PREC_CONDITIONAL,
    PREC_LOGICAL_OR,
    PREC_LOGICAL_AND,
    PREC_BITWISE_OR,
    PREC_BITWISE_XOR,
    PREC_BITWISE_AND,
    PREC_EQUALITY,
    PREC_RELATIONAL,
    PREC_SHIFT,
    PREC_ADDITIVE,
    PREC_MULTIPLICATIVE,
    PREC_UNARY,
    PREC_POSTFIX,
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(Compiler* compiler, bool canAssign);

static void declaration(Compiler* compiler);
static void class_declaration(Compiler* compiler);
static void function_declaration(Compiler* compiler);
static void variable_declaration(Compiler* compiler);

static void statement(Compiler* compiler);
static void print_statement(Compiler* compiler);
static void return_statement(Compiler* compiler);
static void if_statement(Compiler* compiler);
static void while_statement(Compiler* compiler);
static void for_statement(Compiler* compiler);
static void switch_statement(Compiler* compiler);
static void block_statement(Compiler* compiler);
static void expression_statement(Compiler* compiler);

static void expression(Compiler* compiler);
static void literal(Compiler* compiler, bool canAssign);
static void number(Compiler* compiler, bool canAssign);
static void string(Compiler* compiler, bool canAssign);
static void variable(Compiler* compiler, bool canAssign);
static void unary(Compiler* compiler, bool canAssign);
static void binary(Compiler* compiler, bool canAssign);
static void grouping(Compiler* compiler, bool canAssign);
static void and(Compiler* compiler, bool canAssign);
static void or(Compiler* compiler, bool canAssign);
static void call(Compiler* compiler, bool canAssign);
static void dot(Compiler* compiler, bool canAssign);
static void super_(Compiler* compiler, bool canAssign);
static void this_(Compiler* compiler, bool canAssign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

ParseRule rules[] = {
    [TOKEN_L_PAREN]         = { grouping, call,   PREC_POSTFIX        },
    [TOKEN_R_PAREN]         = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_L_BRACE]         = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_R_BRACE]         = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_L_BRACKET]       = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_R_BRACKET]       = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_COMMA]           = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_DOT]             = { NULL,     dot,    PREC_POSTFIX        },
    [TOKEN_SEMICOLON]       = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_QUESTION]        = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_COLON]           = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_TILDE]           = { unary,    NULL,   PREC_NONE           },
    [TOKEN_MINUS]           = { unary,    binary, PREC_ADDITIVE       },
    [TOKEN_MINUS_EQUAL]     = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_DOUBLE_MINUS]    = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_PLUS]            = { NULL,     binary, PREC_ADDITIVE       },
    [TOKEN_PLUS_EQUAL]      = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_DOUBLE_PLUS]     = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_STAR]            = { NULL,     binary, PREC_MULTIPLICATIVE },
    [TOKEN_STAR_EQUAL]      = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_SLASH]           = { NULL,     binary, PREC_MULTIPLICATIVE },
    [TOKEN_SLASH_EQUAL]     = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_PERCENT]         = { NULL,     binary, PREC_MULTIPLICATIVE },
    [TOKEN_PERCENT_EQUAL]   = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_AMPERSAND]       = { NULL,     binary, PREC_BITWISE_AND    },
    [TOKEN_AMPERSAND_EQUAL] = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_PIPE]            = { NULL,     binary, PREC_BITWISE_OR     },
    [TOKEN_PIPE_EQUAL]      = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_CARET]           = { NULL,     binary, PREC_BITWISE_XOR    },
    [TOKEN_CARET_EQUAL]     = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_BANG]            = { unary,    NULL,   PREC_NONE           },
    [TOKEN_BANG_EQUAL]      = { NULL,     binary, PREC_EQUALITY       },
    [TOKEN_EQUAL]           = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_EQUAL_EQUAL]     = { NULL,     binary, PREC_EQUALITY       },
    [TOKEN_GREATER]         = { NULL,     binary, PREC_RELATIONAL     },
    [TOKEN_GREATER_EQUAL]   = { NULL,     binary, PREC_RELATIONAL     },
    [TOKEN_R_SHIFT]         = { NULL,     binary, PREC_SHIFT          },
    [TOKEN_R_SHIFT_EQUAL]   = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_LESS]            = { NULL,     binary, PREC_RELATIONAL     },
    [TOKEN_LESS_EQUAL]      = { NULL,     binary, PREC_RELATIONAL     },
    [TOKEN_L_SHIFT]         = { NULL,     binary, PREC_SHIFT          },
    [TOKEN_L_SHIFT_EQUAL]   = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_AND]             = { NULL,     and,    PREC_LOGICAL_AND    },
    [TOKEN_BREAK]           = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_CLASS]           = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_CONTINUE]        = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_DEFAULT]         = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_ELSE]            = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_FALSE]           = { literal,  NULL,   PREC_NONE           },
    [TOKEN_FUN]             = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_FOR]             = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_IF]              = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_NIL]             = { literal,  NULL,   PREC_NONE           },
    [TOKEN_OR]              = { NULL,     or,     PREC_LOGICAL_OR     },
    [TOKEN_PRINT]           = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_RETURN]          = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_STATIC]          = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_SUPER]           = { super_,   NULL,   PREC_NONE           },
    [TOKEN_THIS]            = { this_,    NULL,   PREC_NONE           },
    [TOKEN_TRUE]            = { literal,  NULL,   PREC_NONE           },
    [TOKEN_VAR]             = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_VAR]             = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_WHILE]           = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_IDENTIFIER]      = { variable, NULL,   PREC_NONE           },
    [TOKEN_STRING]          = { string,   NULL,   PREC_NONE           },
    [TOKEN_NUMBER]          = { number,   NULL,   PREC_NONE           },
    [TOKEN_ERROR]           = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_NONE]            = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_EOF]             = { NULL,     NULL,   PREC_NONE           },
};

static void compiler_init(Compiler* compiler, VM* vm, FunctionType type)
{
    compiler->enclosing = vm->compiler;
    vm->compiler = compiler;

    if (compiler->enclosing) {
        compiler->vm = compiler->enclosing->vm;
        compiler->parser = compiler->enclosing->parser;
    }

    compiler->function = NULL;
    compiler->type = type;

    compiler->localCount = 0;
    compiler->scopeDepth = 0;

    compiler->function = new_function(vm);

    if (type != TYPE_SCRIPT) {
        compiler->function->name = copy_string(vm, compiler->parser->previous.start, compiler->parser->previous.length);
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
}

static void error_at(Compiler* compiler, Token* token, const char* message)
{
    if (compiler->parser->panic) {
        return;
    }

    fprintf(stderr, "[Line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at the end");
    } else if (token->type != TOKEN_ERROR) {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser_enter_error_mode(compiler->parser);
}

static void error_at_current(Compiler* compiler, const char* message)
{
    error_at(compiler, &compiler->parser->current, message);
}

static void error(Compiler* compiler, const char* message)
{
    error_at(compiler, &compiler->parser->previous, message);
}

static bool check(Compiler* compiler, TokenType type)
{
    return parser_check(compiler->parser, type);
}

static TokenType peek_type(Compiler* compiler)
{
    return parser_peek_type(compiler->parser);
}

static void advance(Compiler* compiler)
{
    parser_move_previous(compiler->parser);

    while (true) {
        if (parser_advance(compiler->parser)) {
            break;
        }

        error(compiler, compiler->parser->current.start);
    }
}

static void consume(Compiler* compiler, TokenType type, const char* message)
{
    if (!check(compiler, type)) {
        error_at_current(compiler, message);
    } else {
        advance(compiler);
    }
}

static bool match(Compiler* compiler, TokenType type)
{
    if (!check(compiler, type)) {
        return false;
    }

    advance(compiler);
    return true;
}

static ParseRule* get_rule(TokenType type)
{
    return &rules[type];
}

static void parse_precedence(Compiler* compiler, Precedence precedence)
{
    advance(compiler);

    ParseFn prefixRule = get_rule(compiler->parser->previous.type)->prefix;
    if (prefixRule == NULL) {
        error(compiler, "Expected an expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(compiler, canAssign);

    while (precedence <= get_rule(compiler->parser->current.type)->precedence) {
        advance(compiler);
        ParseFn infixRule = get_rule(compiler->parser->previous.type)->infix;
        infixRule(compiler, canAssign);
    }

    if (canAssign && match(compiler, TOKEN_EQUAL)) {
        error(compiler, "Invalid assignment target.");
    }
}

static Chunk* current_chunk(Compiler* compiler)
{
    return &compiler->function->chunk;
}

static void emit_byte(Compiler* compiler, uint8_t byte)
{
    chunk_write(compiler->vm, current_chunk(compiler), byte, compiler->parser->previous.line);
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
    if (!vm->compiler->parser->error) {
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
    if (compiler->enclosing == NULL) {
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

static void declare_variable(Compiler* compiler)
{
    if (compiler->scopeDepth == 0) {
        return;
    }

    Token* identifier = &compiler->parser->previous;

    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];
        if (local->depth != -1 && local->depth < compiler->scopeDepth) {
            break;
        }

        if (lexemes_equal(identifier, &local->identifier)) {
            error(compiler, "Variable with this name already declared in this scope.");
        }
    }

    add_local(compiler, *identifier);
}

static uint8_t identifier_constant(Compiler* compiler, Token* identifier)
{
    return make_constant(compiler, OBJ_VAL(copy_string(compiler->vm, identifier->start, identifier->length)));
}

static uint8_t parse_variable(Compiler* compiler, const char* message)
{
    consume(compiler, TOKEN_IDENTIFIER, message);

    declare_variable(compiler);
    if (compiler->scopeDepth > 0) {
        return 0;
    }

    return identifier_constant(compiler, &compiler->parser->previous);
}

static void named_variable(Compiler* compiler, Token identifier, bool canAssign)
{
    int scope = resolve_local(compiler, &identifier);
    uint8_t loadOp, storeOp;

    if (scope != -1) {
        loadOp = OP_LOAD_LOCAL;
        storeOp = OP_STORE_LOCAL;
    } else if ((scope = resolve_upvalue(compiler, &identifier)) != -1) {
        loadOp = OP_LOAD_UPVALUE;
        storeOp = OP_STORE_UPVALUE;
    } else {
        scope = identifier_constant(compiler, &identifier);
        loadOp = OP_LOAD_GLOBAL;
        storeOp = OP_STORE_GLOBAL;
    }

    if (canAssign && match(compiler, TOKEN_EQUAL)) {
        expression(compiler);
        emit_bytes(compiler, storeOp, (uint8_t)scope);
    } else {
        emit_bytes(compiler, loadOp, (uint8_t)scope);
    }
}

static void declaration(Compiler* compiler)
{
    if (compiler->parser->panic) {
        parser_synchronize(compiler->parser);
    }

    switch (peek_type(compiler)) {
        case TOKEN_FUN: advance(compiler); function_declaration(compiler); return;
        case TOKEN_CLASS: advance(compiler); class_declaration(compiler); return;
        case TOKEN_VAR: advance(compiler); variable_declaration(compiler); return;
        default: statement(compiler); return;
    }
}

static void block(Compiler* compiler)
{
    while (!check(compiler, TOKEN_R_BRACE) && !check(compiler, TOKEN_EOF)) {
        declaration(compiler);
    }

    consume(compiler, TOKEN_R_BRACE, "Expected '}' after block.");
}

static void function(Compiler* compiler, FunctionType type)
{
    Compiler newCompiler;
    compiler_init(&newCompiler, compiler->vm, type);
    begin_scope(&newCompiler);

    consume(&newCompiler, TOKEN_L_PAREN, "Expected '(' after function name.");
    if (!check(&newCompiler, TOKEN_R_PAREN)) {
        do {
            newCompiler.function->arity++;
            if (newCompiler.function->arity > 255) {
                error(&newCompiler, "Cannot have more than 255 parameters.");
            }

            uint32_t parameter = parse_variable(&newCompiler, "Expected parameter name.");
            define_variable(&newCompiler, parameter);
        } while (match(&newCompiler, TOKEN_COMMA));
    }
    consume(&newCompiler, TOKEN_R_PAREN, "Expected ')' after function parameters.");

    consume(&newCompiler, TOKEN_L_BRACE, "Expected '{' before function body.");
    block(&newCompiler);

    ObjFunction* function = finish_compilation(newCompiler.vm);

    emit_bytes(compiler, OP_CLOSURE, make_constant(compiler, OBJ_VAL(function)));
    for (size_t i = 0; i < function->upvalueCount; i++) {
        emit_byte(compiler, newCompiler.upvalues[i].isLocal ? 1 : 0);
        emit_byte(compiler, newCompiler.upvalues[i].index);
    }
}

static void function_declaration(Compiler* compiler)
{
    uint8_t global = parse_variable(compiler, "Expected function name.");
    mark_initialized(compiler);
    function(compiler, TYPE_FUNCTION);
    define_variable(compiler, global);
}


static void method(Compiler* compiler)
{
    consume(compiler, TOKEN_IDENTIFIER, "Expected method name.");
    uint8_t constant = identifier_constant(compiler, &compiler->parser->previous);

    FunctionType type = TYPE_METHOD;
    if (compiler->parser->previous.length == 4 && memcmp(compiler->parser->previous.start, "init", 4) == 0) {
        type = TYPE_INITIALIZER;
    }
    function(compiler, type);
    emit_bytes(compiler, OP_METHOD, constant);
}

static void class_declaration(Compiler* compiler)
{
    consume(compiler, TOKEN_IDENTIFIER, "Expected class name.");
    Token className = compiler->parser->previous;
    uint8_t nameConstant = identifier_constant(compiler, &compiler->parser->previous);
    declare_variable(compiler);

    emit_bytes(compiler, OP_CLASS, nameConstant);
    define_variable(compiler, nameConstant);

    ClassCompiler classCompiler = { .name = compiler->parser->previous, .enclosing = compiler->vm->classCompiler, .hasSuperclass = false };
    compiler->vm->classCompiler = &classCompiler;

    if (match(compiler, TOKEN_LESS)) {
        consume(compiler, TOKEN_IDENTIFIER, "Expected superclass name.");
        variable(compiler, false);

        if (lexemes_equal(&className, &compiler->parser->previous)) {
            error(compiler, "A class cannot inherit from itself.");
        }

        begin_scope(compiler);
        add_local(compiler, synthetic_token("super"));
        define_variable(compiler, 0);

        named_variable(compiler, className, false);
        emit_byte(compiler, OP_INHERIT);

        compiler->vm->classCompiler->hasSuperclass = true;
    }

    named_variable(compiler, className, false);
    consume(compiler, TOKEN_L_BRACE, "Expected '{' after class declaration.");
    while (!check(compiler, TOKEN_R_BRACE) && !check(compiler, TOKEN_EOF)) {
        method(compiler);
    }
    consume(compiler, TOKEN_R_BRACE, "Expected '}' after class body.");
    emit_byte(compiler, OP_POP);

    if (classCompiler.hasSuperclass) {
        end_scope(compiler);
    }

    compiler->vm->classCompiler = compiler->vm->classCompiler->enclosing;
}

static void variable_declaration(Compiler* compiler)
{
    uint8_t global = parse_variable(compiler, "Expected variable name.");

    if (match(compiler, TOKEN_EQUAL)) {
        expression(compiler);
    } else {
        emit_byte(compiler, OP_LOAD_NIL);
    }

    consume(compiler, TOKEN_SEMICOLON, "Expected ';' after variable declaration.");
    define_variable(compiler, global);
}

static void statement(Compiler* compiler)
{
    switch (peek_type(compiler)) {
        case TOKEN_PRINT: advance(compiler); print_statement(compiler); return;
        case TOKEN_RETURN: advance(compiler); return_statement(compiler); return;
        case TOKEN_IF: advance(compiler); if_statement(compiler); return;
        case TOKEN_WHILE: advance(compiler); while_statement(compiler); return;
        case TOKEN_FOR: advance(compiler); for_statement(compiler); return;
        case TOKEN_SWITCH: advance(compiler); switch_statement(compiler); return;
        case TOKEN_L_BRACE: advance(compiler); block_statement(compiler); return;
        default: expression_statement(compiler); return;
    }
}

static void print_statement(Compiler* compiler)
{
    expression(compiler);
    consume(compiler, TOKEN_SEMICOLON, "Expected ';' after print statement.");
    emit_byte(compiler, OP_PRINT);
}

static void return_statement(Compiler* compiler)
{
    if (compiler->type == TYPE_SCRIPT) {
        error(compiler, "Can only return from functions.");
    }

    if (match(compiler, TOKEN_SEMICOLON)) {
        emit_return(compiler);
    } else {
        if (compiler->type == TYPE_INITIALIZER) {
            error(compiler, "Cannot return a value from an initializer.");
        }

        expression(compiler);
        consume(compiler, TOKEN_SEMICOLON, "Expected ';' after return value.");
        emit_byte(compiler, OP_RETURN);
    }
}

static void if_statement(Compiler* compiler)
{
    consume(compiler, TOKEN_L_PAREN, "Expected '(' before if-condition.");
    expression(compiler);
    consume(compiler, TOKEN_R_PAREN, "Expected ')' after if-condition.");

    size_t thenJump = emit_jump(compiler, OP_JUMP_IF_FALSE);
    emit_byte(compiler, OP_POP);
    statement(compiler);

    size_t elseJump = emit_jump(compiler, OP_JUMP);

    patch_jump(compiler, thenJump);
    emit_byte(compiler, OP_POP);

    if (match(compiler, TOKEN_ELSE)) {
        statement(compiler);
    }

    patch_jump(compiler, elseJump);
}

static void while_statement(Compiler* compiler)
{
    size_t loopStart = current_chunk(compiler)->count;

    consume(compiler, TOKEN_L_PAREN, "Expected '(' before while-condition.");
    expression(compiler);
    consume(compiler, TOKEN_R_PAREN, "Expected ')' after while-condition.");

    size_t exitJump = emit_jump(compiler, OP_JUMP_IF_FALSE);
    emit_byte(compiler, OP_POP);
    statement(compiler);

    emit_loop(compiler, loopStart);

    patch_jump(compiler, exitJump);
    emit_byte(compiler, OP_POP);
}

static void for_statement(Compiler* compiler)
{
    begin_scope(compiler);

    consume(compiler, TOKEN_L_PAREN, "Expected '(' after 'for'.");
    if (match(compiler, TOKEN_SEMICOLON)) {

    } else if (match(compiler, TOKEN_VAR)) {
        variable_declaration(compiler);
    } else {
        expression_statement(compiler);
    }

    size_t loopStart = current_chunk(compiler)->count;

    size_t exitJump = -1;
    if (!match(compiler, TOKEN_SEMICOLON)) {
        expression(compiler);
        consume(compiler, TOKEN_SEMICOLON, "Expected ';' after for-condition.");

        exitJump = emit_jump(compiler, OP_JUMP_IF_FALSE);
        emit_byte(compiler, OP_POP);
    }

    if (!match(compiler, TOKEN_R_PAREN)) {
        size_t bodyJump = emit_jump(compiler, OP_JUMP);

        size_t incrementStart = current_chunk(compiler)->count;
        expression(compiler);
        emit_byte(compiler, OP_POP);
        consume(compiler, TOKEN_R_PAREN, "Expected ')' after for-increment.");

        emit_loop(compiler, loopStart);
        loopStart = incrementStart;
        patch_jump(compiler, bodyJump);
    }

    statement(compiler);

    emit_loop(compiler, loopStart);

    if (exitJump != -1) {
        patch_jump(compiler, exitJump);
        emit_byte(compiler, OP_POP);
    }

    end_scope(compiler);
}

static void switch_statement(Compiler* compiler)
{
    consume(compiler, TOKEN_L_PAREN, "Expected '(' before switch-expression.");
    expression(compiler);
    consume(compiler, TOKEN_R_PAREN, "Expected ')' after switch-expression.");

    consume(compiler, TOKEN_L_BRACE, "Expected '{' before switch-body.");

    size_t switchStart = emit_jump(compiler, OP_JUMP);

    size_t switchExit = current_chunk(compiler)->count;
    size_t exitJump = emit_jump(compiler, OP_JUMP);

    patch_jump(compiler, switchStart);

    while (match(compiler, TOKEN_CASE)) {
        expression(compiler);
        consume(compiler, TOKEN_COLON, "Expected ':' after switch-case expression.");

        size_t skipJump = emit_jump(compiler, OP_JUMP_IF_NOT_EQUAL);
        emit_byte(compiler, OP_POP);

        while (!check(compiler, TOKEN_CASE) && !check(compiler, TOKEN_DEFAULT) && !check(compiler, TOKEN_R_BRACE) && !check(compiler, TOKEN_EOF)) {
            statement(compiler);
        }

        emit_loop(compiler, switchExit);
        patch_jump(compiler, skipJump);
        emit_byte(compiler, OP_POP);
    }

    if (match(compiler, TOKEN_DEFAULT)) {
        consume(compiler, TOKEN_COLON, "Expected ':' after switch-default expression.");
        while (!check(compiler, TOKEN_R_BRACE) && !check(compiler, TOKEN_EOF)) {
            statement(compiler);
        }
    }

    consume(compiler, TOKEN_R_BRACE, "Expected '}' after switch-body.");

    patch_jump(compiler, exitJump);

    emit_byte(compiler, OP_POP);
}

static void block_statement(Compiler* compiler)
{
    begin_scope(compiler);
    block(compiler);
    end_scope(compiler);
}

static void expression_statement(Compiler* compiler)
{
    expression(compiler);
    consume(compiler, TOKEN_SEMICOLON, "Expected ';' after expression statement.");
    emit_byte(compiler, OP_POP);
}

static void expression(Compiler* compiler)
{
    parse_precedence(compiler, PREC_COMMA);
}

static void literal(Compiler* compiler, bool canAssign)
{
    switch (compiler->parser->previous.type) {
        case TOKEN_TRUE: emit_byte(compiler, OP_LOAD_TRUE); break;
        case TOKEN_FALSE: emit_byte(compiler, OP_LOAD_FALSE); break;
        case TOKEN_NIL: emit_byte(compiler, OP_LOAD_NIL); break;
        default: return;
    }
}

static void number(Compiler* compiler, bool canAssign)
{
    double value = strtod(compiler->parser->previous.start, NULL);
    emit_constant(compiler, NUMBER_VAL(value));
}

static void string(Compiler* compiler, bool canAssign)
{
    emit_constant(compiler, OBJ_VAL(copy_string(compiler->vm, compiler->parser->previous.start + 1, (size_t)compiler->parser->previous.length - 2)));
}

static void variable(Compiler* compiler, bool canAssign)
{
    named_variable(compiler, compiler->parser->previous, canAssign);
}

static void unary(Compiler* compiler, bool canAssign)
{
    TokenType operatorType = compiler->parser->previous.type;
    parse_precedence(compiler, PREC_UNARY);
    switch (operatorType) {
        case TOKEN_BANG: emit_byte(compiler, OP_NOT); break;
        case TOKEN_MINUS: emit_byte(compiler, OP_NEGATE); break;
        case TOKEN_TILDE: emit_byte(compiler, OP_BITWISE_NOT); break;
    }
}

static void binary(Compiler* compiler, bool canAssign)
{
    TokenType operatorType = compiler->parser->previous.type;

    ParseRule* rule = get_rule(operatorType);
    parse_precedence(compiler, (Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL: emit_byte(compiler, OP_NOT_EQUAL); break;
        case TOKEN_EQUAL_EQUAL: emit_byte(compiler, OP_EQUAL); break;
        case TOKEN_GREATER: emit_byte(compiler, OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emit_byte(compiler, OP_GREATER_EQUAL); break;
        case TOKEN_LESS: emit_byte(compiler, OP_LESS); break;
        case TOKEN_LESS_EQUAL: emit_byte(compiler, OP_LESS_EQUAL); break;
        case TOKEN_PLUS: emit_byte(compiler, OP_ADD); break;
        case TOKEN_MINUS: emit_byte(compiler, OP_SUBTRACT); break;
        case TOKEN_STAR: emit_byte(compiler, OP_MULTIPLY); break;
        case TOKEN_SLASH: emit_byte(compiler, OP_DIVIDE); break;
        case TOKEN_PERCENT: emit_byte(compiler, OP_MODULO); break;
        case TOKEN_AMPERSAND: emit_byte(compiler, OP_BITWISE_AND); break;
        case TOKEN_PIPE: emit_byte(compiler, OP_BITWISE_OR); break;
        case TOKEN_CARET: emit_byte(compiler, OP_BITWISE_XOR); break;
        case TOKEN_L_SHIFT: emit_byte(compiler, OP_BITWISE_LEFT_SHIFT); break;
        case TOKEN_R_SHIFT: emit_byte(compiler, OP_BITWISE_RIGHT_SHIFT); break;
    }
}

static void grouping(Compiler* compiler, bool canAssign)
{
    expression(compiler);
    consume(compiler, TOKEN_R_PAREN, "Expected ')' after expression.");
}

static void and(Compiler* compiler, bool canAssign)
{
    size_t endJump = emit_jump(compiler, OP_JUMP_IF_FALSE);

    emit_byte(compiler, OP_POP);
    parse_precedence(compiler, PREC_LOGICAL_AND);

    patch_jump(compiler, endJump);
}

static void or(Compiler * compiler, bool canAssign)
{
    size_t elseJump = emit_jump(compiler, OP_JUMP_IF_FALSE);
    size_t endJump = emit_jump(compiler, OP_JUMP);

    patch_jump(compiler, elseJump);
    emit_byte(compiler, OP_POP);

    parse_precedence(compiler, PREC_LOGICAL_OR);
    patch_jump(compiler, endJump);
}

static uint8_t argument_list(Compiler* compiler)
{
    uint8_t count = 0;
    if (!check(compiler, TOKEN_R_PAREN)) {
        do {
            expression(compiler);
            if (count == 255) {
                error(compiler, "Cannot have more than 255 arguments.");
            }
            count++;
        } while (match(compiler, TOKEN_COMMA));
    }

    consume(compiler, TOKEN_R_PAREN, "Expected ')' after function arguments.");
    return count;
}

static void dot(Compiler* compiler, bool canAssign)
{
    consume(compiler, TOKEN_IDENTIFIER, "Expected property name.");
    uint8_t name = identifier_constant(compiler, &compiler->parser->previous);

    if (canAssign && match(compiler, TOKEN_EQUAL)) {
        expression(compiler);
        emit_bytes(compiler, OP_STORE_PROPERTY, name);
    } else if (match(compiler, TOKEN_L_PAREN)) {
        uint8_t argCount = argument_list(compiler);
        emit_bytes(compiler, OP_INVOKE, name);
        emit_byte(compiler, argCount);
    } else {
        emit_bytes(compiler, OP_LOAD_PROPERTY, name);
    }
}

static void call(Compiler* compiler, bool canAssign)
{
    uint8_t argumentCount = argument_list(compiler);
    emit_bytes(compiler, OP_CALL, argumentCount);
}

static void this_(Compiler* compiler, bool canAssign)
{
    if (!compiler->vm->classCompiler) {
        error(compiler, "Cannot use 'this' outside of a class.");
        return;
    }
    variable(compiler, false);
}

static void super_(Compiler* compiler, bool canAssign)
{
    if (!compiler->vm->classCompiler) {
        error(compiler, "Cannot use 'super' outside of a class.");
    } else if (!compiler->vm->classCompiler->hasSuperclass) {
        error(compiler, "Cannot use 'super' in a class with no superclass.");
    }

    consume(compiler, TOKEN_DOT, "Expected '.' after 'super'.");
    consume(compiler, TOKEN_IDENTIFIER, "Expected superclass method name.");
    uint8_t name = identifier_constant(compiler, &compiler->parser->previous);

    named_variable(compiler, synthetic_token("this"), false);
    if (match(compiler, TOKEN_L_PAREN)) {
        uint8_t argCount = argument_list(compiler);
        named_variable(compiler, synthetic_token("super"), false);
        emit_bytes(compiler, OP_SUPER_INVOKE, name);
        emit_byte(compiler, argCount);
    } else {
        named_variable(compiler, synthetic_token("super"), false);
        emit_bytes(compiler, OP_GET_SUPER, name);
    }
}

ObjFunction* compile(VM* vm, const char* source)
{
    vm->compiler = NULL;
    vm->classCompiler = NULL;

    Parser parser;
    parser_init(&parser, source);

    Compiler compiler;
    compiler.vm = vm;
    compiler.parser = &parser;
    compiler_init(&compiler, vm, TYPE_SCRIPT);

    advance(&compiler);

    while (!match(vm->compiler, TOKEN_EOF)) {
        declaration(vm->compiler);
    }

    ObjFunction* function = finish_compilation(vm);
    return parser.error ? NULL : function;
}

void mark_compiler_roots(VM* vm)
{
    for (Compiler* compiler = vm->compiler; compiler != NULL; compiler = compiler->enclosing) {
        gc_mark_object(vm, (Obj*)compiler->function);
    }
}
