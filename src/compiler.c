#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "compiler.h"
#include "vm.h"
#include "scanner.h"
#include "object.h"
#include "table.h"
#include "memory.h"

#if DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
    Token current;
    Token previous;
    bool error;
    bool panic;
} Parser;

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

typedef void (*ParseFn)(bool canAssign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

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
    struct Compiler* enclosing;

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

Parser parser;
Compiler* current = NULL;
ClassCompiler* currentClass = NULL;

static void expression();
static void declaration();
static void statement();
static ParseRule* get_rule(TokenType type);
static void parse_precedence(Precedence precedence);
static uint8_t parse_variable(const char* message);
static uint8_t identifier_constant(Token* identifier);
static void expression_statement();
static void variable_declaration();

static void compiler_init(Compiler* compiler, FunctionType type)
{
    compiler->enclosing = current;
    current = compiler;

    current->function = NULL;
    current->type = type;

    current->localCount = 0;
    current->scopeDepth = 0;

    current->function = new_function();

    if (type != TYPE_SCRIPT) {
        current->function->name = copy_string(parser.previous.start, parser.previous.length);
    }

    Local* local = &current->locals[current->localCount++];
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

static Chunk* current_chunk()
{
    return &current->function->chunk;
}

static void error_at(Token* token, const char* message)
{
    if (parser.panic) {
        return;
    }

    fprintf(stderr, "[Line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at the end");
    } else if (token->type != TOKEN_ERROR) {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    parser.error = true;
    parser.panic = true;
}

static void error_at_current(const char* message)
{
    error_at(&parser.current, message);
}

static void error(const char* message)
{
    error_at(&parser.previous, message);
}

static bool check(TokenType type)
{
    return parser.current.type == type;
}

static void advance()
{
    parser.previous = parser.current;

    while (true) {
        parser.current = scan_token();
        if (!check(TOKEN_ERROR)) {
            break;
        }

        error(parser.current.start);
    }
}

static void consume(TokenType type, const char* message)
{
    if (!check(type)) {
        error_at_current(message);
    } else {
        advance();
    }
}

static bool match(TokenType type)
{
    if (!check(type)) {
        return false;
    }

    advance();
    return true;
}

static void synchronize()
{
    parser.panic = false;

    while (!check(TOKEN_EOF)) {
        if (parser.previous.type == TOKEN_SEMICOLON) {
            return;
        }

        switch (parser.current.type) {
            case TOKEN_CLASS: return;
            case TOKEN_STATIC: return;
            case TOKEN_FUN: return;
            case TOKEN_VAR: return;
            case TOKEN_FOR: return;
            case TOKEN_IF: return;
            case TOKEN_WHILE: return;
            case TOKEN_PRINT: return;
            case TOKEN_BREAK: return;
            case TOKEN_CONTINUE: return;
            case TOKEN_RETURN: return;
        }

        advance();
    }
}

static void emit_byte(uint8_t byte)
{
    chunk_write(current_chunk(), byte, parser.previous.line);
}

static void emit_bytes(uint8_t a, uint8_t b)
{
    emit_byte(a);
    emit_byte(b);
}

static size_t emit_jump(uint8_t instruction)
{
    emit_byte(instruction);
    emit_byte(0xFF);
    emit_byte(0xFF);
    return current_chunk()->count - 2;
}

static void emit_return()
{
    if (current->type == TYPE_INITIALIZER) {
        emit_bytes(OP_GET_LOCAL, 0);
    } else {
        emit_byte(OP_NIL);
    }
    emit_byte(OP_RETURN);
}

static uint8_t make_constant(Value value)
{
    int constant = chunk_add_constant(current_chunk(), value);
    if (constant > UINT8_MAX) {
        error("Too many constants in one chunk.");
        return 0;
    }

    return (uint8_t)constant;
}

static void emit_constant(Value value)
{
    emit_bytes(OP_CONSTANT, make_constant(value));
}

static void patch_jump(size_t offset)
{
    size_t jump = current_chunk()->count - offset - 2;

    if (jump > UINT16_MAX) {
        error("Too much code to jump over.");
    }

    current_chunk()->code[offset    ] = (jump >> 0) & 0xFF;
    current_chunk()->code[offset + 1] = (jump >> 8) & 0xFF;
}

static void emit_loop(size_t loopStart)
{
    emit_byte(OP_LOOP);

    size_t offset = current_chunk()->count - loopStart + 2;
    if (offset > UINT16_MAX) {
        error("Loop body is too large.");
    }

    emit_byte((offset >> 0) & 0xFF);
    emit_byte((offset >> 8) & 0xFF);
}

static ObjFunction* finish_compilation()
{
    emit_return();
    ObjFunction* function = current->function;

#if DEBUG_PRINT_CODE
    if (!parser.error) {
        disassemble_chunk(current_chunk(), function->name != NULL ? function->name->chars : "<script>");
    }
#endif

    current = current->enclosing;
    return function;
}

static void mark_initialized()
{
    if (current->scopeDepth == 0) {
        return;
    }

    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void define_variable(uint8_t global)
{
    if (current->scopeDepth > 0) {
        mark_initialized();
        return;
    }

    emit_byte(OP_DEFINE_GLOBAL);
    emit_byte(global);
}

static void add_local(Token identifier)
{
    if (current->localCount == UINT8_COUNT) {
        error("Too many local variables in function.");
        return;
    }

    Local* local = &current->locals[current->localCount++];
    local->identifier = identifier;
    local->depth = -1;
    local->isCaptured = false;
}

static bool identifiers_equal(Token* a, Token* b)
{
    return a->length == b->length && memcmp(a->start, b->start, a->length) == 0;
}

static int resolve_local(Compiler* compiler, Token* identifier)
{
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local* local = &compiler->locals[i];

        if (identifiers_equal(identifier, &local->identifier)) {
            if (local->depth == -1) {
                error("Cannot read local variable in its own initializer.");
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
            return i;
        }
    }

    if (upvalueCount == UINT8_COUNT) {
        error("Too many closure variables in function.");
        return 0;
    }

    compiler->upvalues[upvalueCount].isLocal = isLocal;
    compiler->upvalues[upvalueCount].index = index;
    return compiler->function->upvalueCount++;
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

static Token synthetic_token(const char* lexeme)
{
    return (Token) { .start = lexeme, .length = (int)strlen(lexeme) };
}

static void declare_variable()
{
    if (current->scopeDepth == 0) {
        return;
    }

    Token* identifier = &parser.previous;

    for (int i = current->localCount - 1; i >= 0; i--) {
        Local* local = &current->locals[i];
        if (local->depth != -1 && local->depth < current->scopeDepth) {
            break;
        }

        if (identifiers_equal(identifier, &local->identifier)) {
            error("Variable with this name already declared in this scope.");
        }
    }

    add_local(*identifier);
}

static void literal(bool canAssign)
{
    switch (parser.previous.type) {
        case TOKEN_TRUE: emit_byte(OP_TRUE); break;
        case TOKEN_FALSE: emit_byte(OP_FALSE); break;
        case TOKEN_NIL: emit_byte(OP_NIL); break;
        default: return;
    }
}

static void number(bool canAssign)
{
    double value = strtod(parser.previous.start, NULL);
    emit_constant(NUMBER_VAL(value));
}

static void string(bool canAssign)
{
    emit_constant(OBJ_VAL(copy_string(parser.previous.start + 1, parser.previous.length - 2)));
}

static void named_variable(Token identifier, bool canAssign)
{
    int scope = resolve_local(current, &identifier);
    uint8_t getOp, setOp;

    if (scope != -1) {
        getOp = OP_GET_LOCAL;
        setOp = OP_SET_LOCAL;
    } else if ((scope = resolve_upvalue(current, &identifier)) != -1) {
        getOp = OP_GET_UPVALUE;
        setOp = OP_SET_UPVALUE;
    } else {
        scope = identifier_constant(&identifier);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emit_bytes(setOp, (uint8_t)scope);
    } else {
        emit_bytes(getOp, (uint8_t)scope);
    }
}

static void variable(bool canAssign)
{
    named_variable(parser.previous, canAssign);
}

static void this_(bool canAssign)
{
    if (currentClass == NULL) {
        error("Cannot use 'this' outside of a class.");
        return;
    }
    variable(false);
}

static uint8_t argument_list()
{
    uint8_t count = 0;
    if (!check(TOKEN_RIGHT_PARENTHESIS)) {
        do {
            expression();
            if (count == 255) {
                error("Cannot have more than 255 arguments.");
            }
            count++;
        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RIGHT_PARENTHESIS, "Expected ')' after function arguments.");
    return count;
}

static void super_(bool canAssign)
{
    if (currentClass == NULL) {
        error("Cannot use 'super' outside of a class.");
    } else if (!currentClass->hasSuperclass) {
        error("Cannot use 'super' in a class with no superclass.");
    }

    consume(TOKEN_DOT, "Expected '.' after 'super'.");
    consume(TOKEN_IDENTIFIER, "Expected superclass method name.");
    uint8_t name = identifier_constant(&parser.previous);

    named_variable(synthetic_token("this"), false);
    if (match(TOKEN_LEFT_PARENTHESIS)) {
        uint8_t argCount = argument_list();
        named_variable(synthetic_token("super"), false);
        emit_bytes(OP_SUPER_INVOKE, name);
        emit_byte(argCount);
    } else {
        named_variable(synthetic_token("super"), false);
        emit_bytes(OP_GET_SUPER, name);
    }
}

static void unary(bool canAssign)
{
    TokenType operatorType = parser.previous.type;
    parse_precedence(PREC_UNARY);
    switch (operatorType) {
        case TOKEN_BANG: emit_byte(OP_NOT); break;
        case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
        case TOKEN_TILDE: emit_byte(OP_BITWISE_NOT); break;
    }
}

static void binary(bool canAssign)
{
    TokenType operatorType = parser.previous.type;

    ParseRule* rule = get_rule(operatorType);
    parse_precedence((Precedence)(rule->precedence + 1));

    switch (operatorType) {
        case TOKEN_BANG_EQUAL: emit_byte(OP_NOT_EQUAL); break;
        case TOKEN_EQUAL_EQUAL: emit_byte(OP_EQUAL); break;
        case TOKEN_GREATER: emit_byte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emit_byte(OP_GREATER_EQUAL); break;
        case TOKEN_LESS: emit_byte(OP_LESS); break;
        case TOKEN_LESS_EQUAL: emit_byte(OP_LESS_EQUAL); break;
        case TOKEN_PLUS: emit_byte(OP_ADD); break;
        case TOKEN_MINUS: emit_byte(OP_SUBTRACT); break;
        case TOKEN_STAR: emit_byte(OP_MULTIPLY); break;
        case TOKEN_SLASH: emit_byte(OP_DIVIDE); break;
        case TOKEN_PERCENT: emit_byte(OP_MODULO); break;
        case TOKEN_AMPERSAND: emit_byte(OP_BITWISE_AND); break;
        case TOKEN_PIPE: emit_byte(OP_BITWISE_OR); break;
        case TOKEN_CARET: emit_byte(OP_BITWISE_XOR); break;
        case TOKEN_LESS_LESS: emit_byte(OP_BITWISE_LEFT_SHIFT); break;
        case TOKEN_GREATER_GREATER: emit_byte(OP_BITWISE_RIGHT_SHIFT); break;
    }
}

static void call(bool canAssign)
{
    uint8_t argumentCount = argument_list();
    emit_bytes(OP_CALL, argumentCount);
}

static void dot(bool canAssign)
{
    consume(TOKEN_IDENTIFIER, "Expected property name.");
    uint8_t name = identifier_constant(&parser.previous);

    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        emit_bytes(OP_SET_PROPERTY, name);
    } else if (match(TOKEN_LEFT_PARENTHESIS)) {
        uint8_t argCount = argument_list();
        emit_bytes(OP_INVOKE, name);
        emit_byte(argCount);
    } else {
        emit_bytes(OP_GET_PROPERTY, name);
    }
}

static void begin_scope()
{
    current->scopeDepth++;
}

static void end_scope()
{
    current->scopeDepth--;

    while (current->localCount > 0 && current->locals[current->localCount - 1].depth > current->scopeDepth) {
        if (current->locals[current->localCount - 1].isCaptured) {
            emit_byte(OP_CLOSE_UPVALUE);
        } else {
            emit_byte(OP_POP);
        }
        
        current->localCount--;
    }
}

static void expression()
{
    parse_precedence(PREC_COMMA);
}

static void grouping(bool canAssign)
{
    expression();
    consume(TOKEN_RIGHT_PARENTHESIS, "Expected ')' after expression.");
}

static void print_statement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';' after print statement.");
    emit_byte(OP_PRINT);
}

static void return_statement()
{
    if (current->type == TYPE_SCRIPT) {
        error("Can only return from functions.");
    }

    if (match(TOKEN_SEMICOLON)) {
        emit_return();
    } else {
        if (current->type == TYPE_INITIALIZER) {
            error("Cannot return a value from an initializer.");
        }

        expression();
        consume(TOKEN_SEMICOLON, "Expected ';' after return value.");
        emit_byte(OP_RETURN);
    }
}

static void if_statement()
{
    consume(TOKEN_LEFT_PARENTHESIS, "Expected '(' before if-condition.");
    expression();
    consume(TOKEN_RIGHT_PARENTHESIS, "Expected ')' after if-condition.");

    size_t thenJump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
    statement();

    size_t elseJump = emit_jump(OP_JUMP);

    patch_jump(thenJump);
    emit_byte(OP_POP);

    if (match(TOKEN_ELSE)) {
        statement();
    }

    patch_jump(elseJump);
}

static void while_statement()
{
    size_t loopStart = current_chunk()->count;

    consume(TOKEN_LEFT_PARENTHESIS, "Expected '(' before while-condition.");
    expression();
    consume(TOKEN_RIGHT_PARENTHESIS, "Expected ')' after while-condition.");

    size_t exitJump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
    statement();

    emit_loop(loopStart);

    patch_jump(exitJump);
    emit_byte(OP_POP);
}

static void for_statement()
{
    begin_scope();

    consume(TOKEN_LEFT_PARENTHESIS, "Expected '(' after 'for'.");
    if (match(TOKEN_SEMICOLON)) {

    } else if (match(TOKEN_VAR)) {
        variable_declaration();
    } else {
        expression_statement();
    }

    size_t loopStart = current_chunk()->count;

    size_t exitJump = -1;
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expected ';' after for-condition.");

        exitJump = emit_jump(OP_JUMP_IF_FALSE);
        emit_byte(OP_POP);
    }

    if (!match(TOKEN_RIGHT_PARENTHESIS)) {
        size_t bodyJump = emit_jump(OP_JUMP);

        size_t incrementStart = current_chunk()->count;
        expression();
        emit_byte(OP_POP);
        consume(TOKEN_RIGHT_PARENTHESIS, "Expected ')' after for-increment.");

        emit_loop(loopStart);
        loopStart = incrementStart;
        patch_jump(bodyJump);
    }

    statement();

    emit_loop(loopStart);

    if (exitJump != -1) {
        patch_jump(exitJump);
        emit_byte(OP_POP);
    }

    end_scope();
}

static void switch_statement()
{
    consume(TOKEN_LEFT_PARENTHESIS, "Expected '(' before switch-expression.");
    expression();
    consume(TOKEN_RIGHT_PARENTHESIS, "Expected ')' after switch-expression.");

    consume(TOKEN_LEFT_BRACE, "Expected '{' before switch-body.");

    size_t switchStart = emit_jump(OP_JUMP);

    size_t switchExit = current_chunk()->count;
    size_t exitJump = emit_jump(OP_JUMP);

    patch_jump(switchStart);

    while (match(TOKEN_CASE)) {
        expression();
        consume(TOKEN_COLON, "Expected ':' after switch-case expression.");

        size_t skipJump = emit_jump(OP_JUMP_IF_NOT_EQUAL);
        emit_byte(OP_POP);

        while (!check(TOKEN_CASE) && !check(TOKEN_DEFAULT) && !check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
            statement();
        }

        emit_loop(switchExit);
        patch_jump(skipJump);
        emit_byte(OP_POP);
    }

    if (match(TOKEN_DEFAULT)) {
        consume(TOKEN_COLON, "Expected ':' after switch-default expression.");
        while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
            statement();
        }
    }

    consume(TOKEN_RIGHT_BRACE, "Expected '}' after switch-body.");

    patch_jump(exitJump);

    emit_byte(OP_POP);
}

static void and(bool canAssign)
{
    size_t endJump = emit_jump(OP_JUMP_IF_FALSE);

    emit_byte(OP_POP);
    parse_precedence(PREC_LOGICAL_AND);

    patch_jump(endJump);
}

static void or(bool canAssign)
{
    size_t elseJump = emit_jump(OP_JUMP_IF_FALSE);
    size_t endJump = emit_jump(OP_JUMP);

    patch_jump(elseJump);
    emit_byte(OP_POP);

    parse_precedence(PREC_LOGICAL_OR);
    patch_jump(endJump);
}

static void block()
{
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expected '}' after block.");
}

static void expression_statement()
{
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';' after expression statement.");
    emit_byte(OP_POP);
}

static void function(FunctionType type)
{
    Compiler compiler;
    compiler_init(&compiler, type);
    begin_scope();

    consume(TOKEN_LEFT_PARENTHESIS, "Expected '(' after function name.");
    if (!check(TOKEN_RIGHT_PARENTHESIS)) {
        do {
            current->function->arity++;
            if (current->function->arity > 255) {
                error("Cannot have more than 255 parameters.");
            }

            uint32_t parameter = parse_variable("Expected parameter name.");
            define_variable(parameter);
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PARENTHESIS, "Expected ')' after function parameters.");

    consume(TOKEN_LEFT_BRACE, "Expected '{' before function body.");
    block();

    ObjFunction* function = finish_compilation();

    emit_bytes(OP_CLOSURE, make_constant(OBJ_VAL(function)));
    for (size_t i = 0; i < function->upvalueCount; i++) {
        emit_byte(compiler.upvalues[i].isLocal ? 1 : 0);
        emit_byte(compiler.upvalues[i].index);
    }
}

static void method()
{
    consume(TOKEN_IDENTIFIER, "Expected method name.");
    uint8_t constant = identifier_constant(&parser.previous);

    FunctionType type = TYPE_METHOD;
    if (parser.previous.length == 4 && memcmp(parser.previous.start, "init", 4) == 0) {
        type = TYPE_INITIALIZER;
    }
    function(type);
    emit_bytes(OP_METHOD, constant);
}

static void class_declaration()
{
    consume(TOKEN_IDENTIFIER, "Expected class name.");
    Token className = parser.previous;
    uint8_t nameConstant = identifier_constant(&parser.previous);
    declare_variable();

    emit_bytes(OP_CLASS, nameConstant);
    define_variable(nameConstant);

    ClassCompiler classCompiler = { .name = parser.previous, .enclosing = currentClass, .hasSuperclass = false };
    currentClass = &classCompiler;

    if (match(TOKEN_LESS)) {
        consume(TOKEN_IDENTIFIER, "Expected superclass name.");
        variable(false);

        if (identifiers_equal(&className, &parser.previous)) {
            error("A class cannot inherit from itself.");
        }

        begin_scope();
        add_local(synthetic_token("super"));
        define_variable(0);

        named_variable(className, false);
        emit_byte(OP_INHERIT);

        currentClass->hasSuperclass = true;
    }

    named_variable(className, false);
    consume(TOKEN_LEFT_BRACE, "Expected '{' after class declaration.");
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        method();
    }
    consume(TOKEN_RIGHT_BRACE, "Expected '}' after class body.");
    emit_byte(OP_POP);

    if (classCompiler.hasSuperclass) {
        end_scope();
    }

    currentClass = currentClass->enclosing;
}

static void function_declaration()
{
    uint8_t global = parse_variable("Expected function name.");
    mark_initialized();
    function(TYPE_FUNCTION);
    define_variable(global);
}

static void variable_declaration()
{
    uint8_t global = parse_variable("Expected variable name.");

    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emit_byte(OP_NIL);
    }

    consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration.");
    define_variable(global);
}

static void declaration()
{
    if (parser.panic) {
        synchronize();
    }

    if (match(TOKEN_CLASS)) {
        class_declaration();
    } else if (match(TOKEN_FUN)) {
        function_declaration();
    } else if (match(TOKEN_VAR)) {
        variable_declaration();
    } else {
        statement();
    }
}

static void statement()
{
    if (match(TOKEN_PRINT)) {
        print_statement();
    } else if (match(TOKEN_RETURN)) {
        return_statement();
    } else if (match(TOKEN_IF)) {
        if_statement();
    } else if (match(TOKEN_WHILE)) {
        while_statement();
    } else if (match(TOKEN_FOR)) {
        for_statement();
    } else if (match(TOKEN_SWITCH)) {
        switch_statement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        begin_scope();
        block();
        end_scope();
    } else {
        expression_statement();
    }
}

ParseRule rules[] = {
    [TOKEN_LEFT_PARENTHESIS]      = { grouping, call,   PREC_POSTFIX        },
    [TOKEN_RIGHT_PARENTHESIS]     = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_LEFT_BRACE]            = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_RIGHT_BRACE]           = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_LEFT_BRACKET]          = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_RIGHT_BRACKET]         = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_COMMA]                 = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_DOT]                   = { NULL,     dot,    PREC_POSTFIX        },
    [TOKEN_SEMICOLON]             = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_QUESTION_MARK]         = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_COLON]                 = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_TILDE]                 = { unary,    NULL,   PREC_NONE           },
    [TOKEN_MINUS]                 = { unary,    binary, PREC_ADDITIVE       },
    [TOKEN_MINUS_EQUAL]           = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_MINUS_MINUS]           = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_PLUS]                  = { NULL,     binary, PREC_ADDITIVE       },
    [TOKEN_PLUS_EQUAL]            = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_PLUS_PLUS]             = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_STAR]                  = { NULL,     binary, PREC_MULTIPLICATIVE },
    [TOKEN_STAR_EQUAL]            = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_SLASH]                 = { NULL,     binary, PREC_MULTIPLICATIVE },
    [TOKEN_SLASH_EQUAL]           = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_PERCENT]               = { NULL,     binary, PREC_MULTIPLICATIVE },
    [TOKEN_PERCENT_EQUAL]         = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_AMPERSAND]             = { NULL,     binary, PREC_BITWISE_AND    },
    [TOKEN_AMPERSAND_EQUAL]       = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_PIPE]                  = { NULL,     binary, PREC_BITWISE_OR     },
    [TOKEN_PIPE_EQUAL]            = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_CARET]                 = { NULL,     binary, PREC_BITWISE_XOR    },
    [TOKEN_CARET_EQUAL]           = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_BANG]                  = { unary,    NULL,   PREC_NONE           },
    [TOKEN_BANG_EQUAL]            = { NULL,     binary, PREC_EQUALITY       },
    [TOKEN_EQUAL]                 = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_EQUAL_EQUAL]           = { NULL,     binary, PREC_EQUALITY       },
    [TOKEN_GREATER]               = { NULL,     binary, PREC_RELATIONAL     },
    [TOKEN_GREATER_EQUAL]         = { NULL,     binary, PREC_RELATIONAL     },
    [TOKEN_GREATER_GREATER]       = { NULL,     binary, PREC_SHIFT          },
    [TOKEN_GREATER_GREATER_EQUAL] = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_LESS]                  = { NULL,     binary, PREC_RELATIONAL     },
    [TOKEN_LESS_EQUAL]            = { NULL,     binary, PREC_RELATIONAL     },
    [TOKEN_LESS_LESS]             = { NULL,     binary, PREC_SHIFT          },
    [TOKEN_LESS_LESS_EQUAL]       = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_AND]                   = { NULL,     and,    PREC_LOGICAL_AND    },
    [TOKEN_BREAK]                 = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_CLASS]                 = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_CONTINUE]              = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_DEFAULT]               = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_ELSE]                  = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_FALSE]                 = { literal,  NULL,   PREC_NONE           },
    [TOKEN_FUN]                   = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_FOR]                   = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_IF]                    = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_NIL]                   = { literal,  NULL,   PREC_NONE           },
    [TOKEN_OR]                    = { NULL,     or,     PREC_LOGICAL_OR     },
    [TOKEN_PRINT]                 = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_RETURN]                = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_STATIC]                = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_SUPER]                 = { super_,   NULL,   PREC_NONE           },
    [TOKEN_THIS]                  = { this_,    NULL,   PREC_NONE           },
    [TOKEN_TRUE]                  = { literal,  NULL,   PREC_NONE           },
    [TOKEN_VAR]                   = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_VAR]                   = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_WHILE]                 = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_IDENTIFIER]            = { variable, NULL,   PREC_NONE           },
    [TOKEN_STRING]                = { string,   NULL,   PREC_NONE           },
    [TOKEN_NUMBER]                = { number,   NULL,   PREC_NONE           },
    [TOKEN_ERROR]                 = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_NONE]                  = { NULL,     NULL,   PREC_NONE           },
    [TOKEN_EOF]                   = { NULL,     NULL,   PREC_NONE           },
};

static void parse_precedence(Precedence precedence)
{
    advance();

    ParseFn prefixRule = get_rule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expected an expression.");
        return;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= get_rule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = get_rule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

static uint8_t identifier_constant(Token* identifier)
{
    return make_constant(OBJ_VAL(copy_string(identifier->start, identifier->length)));
}

static uint8_t parse_variable(const char* message)
{
    consume(TOKEN_IDENTIFIER, message);

    declare_variable();
    if (current->scopeDepth > 0) {
        return 0;
    }

    return identifier_constant(&parser.previous);
}

static ParseRule* get_rule(TokenType type)
{
    return &rules[type];
}

ObjFunction* compile(const char* source)
{
    scanner_init(source);
    parser.error = false;
    parser.panic = false;

    Compiler compiler;
    compiler_init(&compiler, TYPE_SCRIPT);

    advance();

    while (!match(TOKEN_EOF)) {
        declaration();
    }

    ObjFunction* function = finish_compilation();
    return parser.error ? NULL : function;
}

void mark_compiler_roots()
{
    for (Compiler* compiler = current; compiler != NULL; compiler = compiler->enclosing) {
        mark_object((Obj*)compiler->function);
    }
}
