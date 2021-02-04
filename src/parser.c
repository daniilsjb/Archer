#include <stdio.h>

#include "parser.h"
#include "scanner.h"
#include "token.h"
#include "common.h"

typedef struct {
    Scanner scanner;

    Token current;
    Token previous;

    bool error;
    bool panic;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_RANGE,
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
    PREC_EXPONENTIATION,
    PREC_UNARY,
    PREC_POSTFIX,
    PREC_PRIMARY
} Precedence;

typedef enum {
    ASSOC_NONE,
    ASSOC_LEFT,
    ASSOC_RIGHT
} Associativity;

typedef Expression* (*PrefixParselet)(Parser* parser);
typedef Expression* (*InfixParselet)(Parser* parser, Expression* prefix);

static Declaration* declaration(Parser* parser);
static Declaration* import_decl(Parser* parser);
static Declaration* class_decl(Parser* parser);
static Declaration* function_decl(Parser* parser, bool coroutine);
static Declaration* variable_decl(Parser* parser);
static Declaration* begin_variable_decl(Parser* parser);
static Declaration* end_variable_decl(Parser* parser, Declaration* declaration);
static Declaration* statement_decl(Parser* parser);

static Statement* statement(Parser* parser);
static Statement* for_stmt(Parser* parser);
static Statement* for_in_stmt(Parser* parser, Declaration* declaration);
static Statement* while_stmt(Parser* parser);
static Statement* do_while_stmt(Parser* parser);
static Statement* break_stmt(Parser* parser);
static Statement* continue_stmt(Parser* parser);
static Statement* when_stmt(Parser* parser);
static Statement* if_stmt(Parser* parser);
static Statement* return_stmt(Parser* parser);
static Statement* print_stmt(Parser* parser);
static Statement* block_stmt(Parser* parser);
static Statement* expression_stmt(Parser* parser);

static Expression* expression(Parser* parser);
static Expression* literal_expr(Parser* parser);
static Expression* string_interp_expr(Parser* parser);
static Expression* lambda_expr(Parser* parser);
static Expression* list_expr(Parser* parser);
static Expression* map_expr(Parser* parser);
static Expression* identifier_expr(Parser* parser);
static Expression* prefix_inc_expr(Parser* parser);
static Expression* unary_expr(Parser* parser);
static Expression* grouping_expr(Parser* parser);
static Expression* super_expr(Parser* parser);
static Expression* coroutine_expr(Parser* parser);
static Expression* yield_expr(Parser* parser);
static Expression* unpack_assignment_expr(Parser* parser);
static Expression* call_expr(Parser* parser, Expression* prefix);
static Expression* range_expr(Parser* parser, Expression* prefix);
static Expression* property_expr(Parser* parser, Expression* prefix);
static Expression* subscript_expr(Parser* parser, Expression* prefix);
static Expression* postfix_inc_expr(Parser* parser, Expression* prefix);
static Expression* assignment_expr(Parser* parser, Expression* prefix);
static Expression* compound_assignment_expr(Parser* parser, Expression* prefix);
static Expression* logical_expr(Parser* parser, Expression* prefix);
static Expression* conditional_expr(Parser* parser, Expression* prefix);
static Expression* elvis_expr(Parser* parser, Expression* prefix);
static Expression* binary_expr(Parser* parser, Expression* prefix);

static WhenEntry* when_entry_rule(Parser* parser);
static WhenEntryList* when_entries_rule(Parser* parser);
static Block* block_rule(Parser* parser);
static ParameterList* parameters_rule(Parser* parser);
static NamedFunction* named_function_rule(Parser* parser, bool coroutine);
static Method* method_rule(Parser* parser);
static ArgumentList* arguments_rule(Parser* parser);

typedef struct {
    PrefixParselet prefix;
    InfixParselet infix;
    Precedence precedence;
    Associativity associativity;
} ParseRule;

ParseRule rules[] = {
    [TOKEN_L_PAREN]             = { grouping_expr,          call_expr,                PREC_POSTFIX,        ASSOC_LEFT   },
    [TOKEN_R_PAREN]             = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_L_BRACE]             = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_R_BRACE]             = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_AT_L_BRACE]          = { map_expr,               NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_L_BRACKET]           = { list_expr,              subscript_expr,           PREC_POSTFIX,        ASSOC_LEFT   },
    [TOKEN_R_BRACKET]           = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_COMMA]               = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_DOT]                 = { NULL,                   property_expr,            PREC_POSTFIX,        ASSOC_LEFT   },
    [TOKEN_DOT_DOT]             = { NULL,                   range_expr,               PREC_RANGE,          ASSOC_LEFT   },
    [TOKEN_SEMICOLON]           = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_QUESTION]            = { NULL,                   conditional_expr,         PREC_CONDITIONAL,    ASSOC_RIGHT  },
    [TOKEN_COLON]               = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_R_ARROW]             = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_BACKSLASH]           = { lambda_expr,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_QUESTION_DOT]        = { NULL,                   property_expr,            PREC_POSTFIX,        ASSOC_LEFT   },
    [TOKEN_QUESTION_COLON]      = { NULL,                   elvis_expr,               PREC_CONDITIONAL,    ASSOC_RIGHT  },
    [TOKEN_QUESTION_L_BRACKET]  = { NULL,                   subscript_expr,           PREC_POSTFIX,        ASSOC_LEFT   },
    [TOKEN_TILDE]               = { unary_expr,             NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_MINUS]               = { unary_expr,             binary_expr,              PREC_ADDITIVE,       ASSOC_LEFT   },
    [TOKEN_MINUS_EQUAL]         = { NULL,                   compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_DOUBLE_MINUS]        = { prefix_inc_expr,        postfix_inc_expr,         PREC_POSTFIX,        ASSOC_LEFT   },
    [TOKEN_PLUS]                = { NULL,                   binary_expr,              PREC_ADDITIVE,       ASSOC_LEFT   },
    [TOKEN_PLUS_EQUAL]          = { NULL,                   compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_DOUBLE_PLUS]         = { prefix_inc_expr,        postfix_inc_expr,         PREC_POSTFIX,        ASSOC_LEFT   },
    [TOKEN_STAR]                = { NULL,                   binary_expr,              PREC_MULTIPLICATIVE, ASSOC_LEFT   },
    [TOKEN_STAR_EQUAL]          = { NULL,                   compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_DOUBLE_STAR]         = { NULL,                   binary_expr,              PREC_EXPONENTIATION, ASSOC_RIGHT  },
    [TOKEN_DOUBLE_STAR_EQUAL]   = { NULL,                   compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_SLASH]               = { NULL,                   binary_expr,              PREC_MULTIPLICATIVE, ASSOC_LEFT   },
    [TOKEN_SLASH_EQUAL]         = { NULL,                   compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_PERCENT]             = { NULL,                   binary_expr,              PREC_MULTIPLICATIVE, ASSOC_LEFT   },
    [TOKEN_PERCENT_EQUAL]       = { NULL,                   compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_AMPERSAND]           = { NULL,                   binary_expr,              PREC_BITWISE_AND,    ASSOC_LEFT   },
    [TOKEN_AMPERSAND_EQUAL]     = { NULL,                   compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_PIPE]                = { unpack_assignment_expr, binary_expr,              PREC_BITWISE_OR,     ASSOC_LEFT   },
    [TOKEN_PIPE_EQUAL]          = { NULL,                   compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_CARET]               = { NULL,                   binary_expr,              PREC_BITWISE_XOR,    ASSOC_LEFT   },
    [TOKEN_CARET_EQUAL]         = { NULL,                   compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_BANG]                = { unary_expr,             NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_BANG_EQUAL]          = { NULL,                   binary_expr,              PREC_EQUALITY,       ASSOC_LEFT   },
    [TOKEN_EQUAL]               = { NULL,                   assignment_expr,          PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_EQUAL_EQUAL]         = { NULL,                   binary_expr,              PREC_EQUALITY,       ASSOC_LEFT   },
    [TOKEN_GREATER]             = { NULL,                   binary_expr,              PREC_RELATIONAL,     ASSOC_LEFT   },
    [TOKEN_GREATER_EQUAL]       = { NULL,                   binary_expr,              PREC_RELATIONAL,     ASSOC_LEFT   },
    [TOKEN_R_SHIFT]             = { NULL,                   binary_expr,              PREC_SHIFT,          ASSOC_LEFT   },
    [TOKEN_R_SHIFT_EQUAL]       = { NULL,                   compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_LESS]                = { NULL,                   binary_expr,              PREC_RELATIONAL,     ASSOC_LEFT   },
    [TOKEN_LESS_EQUAL]          = { NULL,                   binary_expr,              PREC_RELATIONAL,     ASSOC_LEFT   },
    [TOKEN_L_SHIFT]             = { NULL,                   binary_expr,              PREC_SHIFT,          ASSOC_LEFT   },
    [TOKEN_L_SHIFT_EQUAL]       = { NULL,                   compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_AND]                 = { NULL,                   logical_expr,             PREC_LOGICAL_AND,    ASSOC_LEFT   },
    [TOKEN_BREAK]               = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_CLASS]               = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_CONTINUE]            = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_COROUTINE]           = { coroutine_expr,         NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_DEFAULT]             = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_DO]                  = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_ELSE]                = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_FALSE]               = { literal_expr,           NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_FUN]                 = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_FOR]                 = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_IF]                  = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_IN]                  = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_NIL]                 = { literal_expr,           NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_OR]                  = { NULL,                   logical_expr,             PREC_LOGICAL_OR,     ASSOC_LEFT   },
    [TOKEN_PRINT]               = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_RETURN]              = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_STATIC]              = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_SUPER]               = { super_expr,             NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_THIS]                = { literal_expr,           NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_TRUE]                = { literal_expr,           NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_VAR]                 = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_VAR]                 = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_WHILE]               = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_YIELD]               = { yield_expr,             NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_IDENTIFIER]          = { identifier_expr,        NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_STRING]              = { literal_expr,           NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_STRING_INTERP_BEGIN] = { string_interp_expr,     NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_STRING_INTERP]       = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_STRING_INTERP_END]   = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_NUMBER]              = { literal_expr,           NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_ERROR]               = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_NONE]                = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_EOF]                 = { NULL,                   NULL,                     PREC_NONE,           ASSOC_NONE   },
};

static void parser_init(Parser* parser, const char* source)
{
    Scanner_Init(&parser->scanner, source);
    parser->error = false;
    parser->panic = false;
}

static void enter_error_mode(Parser* parser)
{
    parser->error = true;
    parser->panic = true;
}

static void error_at(Parser* parser, Token* token, const char* message)
{
    if (parser->panic) {
        return;
    }

    fprintf(stderr, "[Line %d] Error", token->line);

    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at the end");
    } else if (token->type != TOKEN_ERROR) {
        fprintf(stderr, " at '%.*s'", (int)token->length, token->start);
    }

    fprintf(stderr, ": %s\n", message);
    enter_error_mode(parser);
}

static void error_at_current(Parser* parser, const char* message)
{
    error_at(parser, &parser->current, message);
}

static void error(Parser* parser, const char* message)
{
    error_at(parser, &parser->previous, message);
}

static bool check(Parser* parser, TokenType type)
{
    return parser->current.type == type;
}

static bool next_token(Parser* parser)
{
    parser->current = Scanner_ScanToken(&parser->scanner);
    return !check(parser, TOKEN_ERROR);
}

static void advance(Parser* parser)
{
    parser->previous = parser->current;

    while (!next_token(parser)) {
        error_at_current(parser, parser->current.start);
    }
}

static bool match(Parser* parser, TokenType type)
{
    if (!check(parser, type)) {
        return false;
    }

    advance(parser);
    return true;
}

static void consume(Parser* parser, TokenType type, const char* message)
{
    if (!check(parser, type)) {
        error_at_current(parser, message);
    } else {
        advance(parser);
    }
}

static bool reached_end(Parser* parser)
{
    return check(parser, TOKEN_EOF);
}

static void synchronize(Parser* parser)
{
    parser->panic = false;
    Scanner_Clear(&parser->scanner);

    while (!check(parser, TOKEN_EOF)) {
        switch (parser->previous.type) {
            case TOKEN_SEMICOLON: return;
        }

        switch (parser->current.type) {
            case TOKEN_IMPORT: return;
            case TOKEN_CLASS: return;
            case TOKEN_STATIC: return;
            case TOKEN_FUN: return;
            case TOKEN_VAR: return;
            case TOKEN_FOR: return;
            case TOKEN_WHEN: return;
            case TOKEN_IF: return;
            case TOKEN_WHILE: return;
            case TOKEN_DO: return;
            case TOKEN_PRINT: return;
            case TOKEN_BREAK: return;
            case TOKEN_CONTINUE: return;
            case TOKEN_RETURN: return;
            case TOKEN_YIELD: return;
        }

        next_token(parser);
    }
}

static ParseRule* get_rule(TokenType type)
{
    return &rules[type];
}

static Expression* parse_precedence(Parser* parser, Precedence precedence)
{
    advance(parser);

    PrefixParselet prefixRule = get_rule(parser->previous.type)->prefix;
    if (!prefixRule) {
        error(parser, "Expected an expression.");
        return NULL;
    }

    Expression* expr = prefixRule(parser);

    while (precedence <= get_rule(parser->current.type)->precedence) {
        advance(parser);
        InfixParselet infixRule = get_rule(parser->previous.type)->infix;
        expr = infixRule(parser, expr);
    }

    return expr;
}

static Declaration* finish_coroutine(Parser* parser)
{
    if (match(parser, TOKEN_FUN)) {
        return function_decl(parser, true);
    }

    Declaration* coroutine = Ast_NewStatementDecl(Ast_NewExpressionStmt(coroutine_expr(parser)));
    consume(parser, TOKEN_SEMICOLON, "Expected ';' at the end of statement.");
    return coroutine;
}

Declaration* declaration(Parser* parser)
{
    if (parser->panic) {
        synchronize(parser);
    }

    switch (parser->current.type) {
        case TOKEN_COROUTINE: advance(parser); return finish_coroutine(parser);
        case TOKEN_IMPORT: advance(parser); return import_decl(parser);
        case TOKEN_CLASS: advance(parser); return class_decl(parser);
        case TOKEN_FUN: advance(parser); return function_decl(parser, false);
        case TOKEN_VAR: advance(parser); return variable_decl(parser);
        default: return statement_decl(parser);
    }
}

Declaration* import_decl(Parser* parser)
{
    Expression* moduleName = expression(parser);

    if (match(parser, TOKEN_AS)) {
        consume(parser, TOKEN_IDENTIFIER, "Expected alias in import.");
        Token alias = parser->previous;

        consume(parser, TOKEN_SEMICOLON, "Expected ';' after import.");
        return Ast_NewImportAsDecl(moduleName, alias);
    } else if (match(parser, TOKEN_FOR)) {
        ParameterList* names = parameters_rule(parser);

        consume(parser, TOKEN_SEMICOLON, "Expected ';' after import.");
        return Ast_NewImportForDecl(moduleName, names);
    }

    consume(parser, TOKEN_SEMICOLON, "Expected ';' after import.");
    return Ast_NewImportAllDecl(moduleName);
}

Declaration* class_decl(Parser* parser)
{
    consume(parser, TOKEN_IDENTIFIER, "Expected class name in declaration.");
    Token identifier = parser->previous;

    Token superclass = Token_Empty();
    if (match(parser, TOKEN_LESS)) {
        consume(parser, TOKEN_IDENTIFIER, "Expected superclass name in declaration.");
        superclass = parser->previous;
    }

    MethodList* body = NULL;
    consume(parser, TOKEN_L_BRACE, "Expected '{' before class body in declaration.");
    while (!check(parser, TOKEN_R_BRACE) && !check(parser, TOKEN_EOF)) {
        Ast_MethodListAppend(&body, method_rule(parser));
    }
    consume(parser, TOKEN_R_BRACE, "Expected '}' after class body in declaration.");

    return Ast_NewClassDecl(identifier, superclass, body);
}

Declaration* function_decl(Parser* parser, bool coroutine)
{
    return Ast_NewFunctionDecl(named_function_rule(parser, coroutine));
}

Declaration* variable_decl(Parser* parser)
{
    Declaration* decl = begin_variable_decl(parser);
    return end_variable_decl(parser, decl);
}

Declaration* begin_variable_decl(Parser* parser)
{
    if (match(parser, TOKEN_PIPE)) {
        ParameterList* identifiers = NULL;
        do {
            consume(parser, TOKEN_IDENTIFIER, "Expected variable name in declaration.");
            Ast_ParameterListAppend(&identifiers, parser->previous);
        } while (match(parser, TOKEN_COMMA));

        consume(parser, TOKEN_PIPE, "Expected '|' at the end of unpacking declaration.");

        VariableTarget* target = Ast_NewUnpackVariableTarget(identifiers);
        return Ast_NewVariableDecl(target, NULL);
    } else {
        consume(parser, TOKEN_IDENTIFIER, "Expected variable name in declaration.");

        VariableTarget* target = Ast_NewSingleVariableTarget(parser->previous);
        return Ast_NewVariableDecl(target, NULL);
    }

    return NULL;
}

Declaration* end_variable_decl(Parser* parser, Declaration* declaration)
{
    if (match(parser, TOKEN_EQUAL)) {
        declaration->as.variableDecl.value = expression(parser);
    }

    consume(parser, TOKEN_SEMICOLON, "Expected ';' after variable declaration.");
    return declaration;
}

Declaration* statement_decl(Parser* parser)
{
    return Ast_NewStatementDecl(statement(parser));
}

Statement* statement(Parser* parser)
{
    switch (parser->current.type) {
        case TOKEN_FOR: advance(parser); return for_stmt(parser);
        case TOKEN_WHILE: advance(parser); return while_stmt(parser);
        case TOKEN_DO: advance(parser); return do_while_stmt(parser);
        case TOKEN_BREAK: advance(parser); return break_stmt(parser);
        case TOKEN_CONTINUE: advance(parser); return continue_stmt(parser);
        case TOKEN_WHEN: advance(parser); return when_stmt(parser);
        case TOKEN_IF: advance(parser); return if_stmt(parser);
        case TOKEN_RETURN: advance(parser); return return_stmt(parser);
        case TOKEN_PRINT: advance(parser);  return print_stmt(parser);
        case TOKEN_L_BRACE: advance(parser); return block_stmt(parser);
        default: return expression_stmt(parser);
    }
}

Statement* for_stmt(Parser* parser)
{
    consume(parser, TOKEN_L_PAREN, "Expected '(' after 'for'.");
    Declaration* initializer = NULL;
    if (match(parser, TOKEN_VAR)) {
        initializer = begin_variable_decl(parser);

        if (match(parser, TOKEN_IN)) {
            return for_in_stmt(parser, initializer);
        } else {
            initializer = end_variable_decl(parser, initializer);
        }
    } else if (!match(parser, TOKEN_SEMICOLON)) {
        initializer = Ast_NewStatementDecl(expression_stmt(parser));
    }

    Expression* condition = NULL;
    if (!match(parser, TOKEN_SEMICOLON)) {
        condition = expression(parser);
        consume(parser, TOKEN_SEMICOLON, "Expected ';' after condition in 'for'.");
    }

    Expression* increment = NULL;
    if (!match(parser, TOKEN_R_PAREN)) {
        increment = expression(parser);
        consume(parser, TOKEN_R_PAREN, "Expected ')' after increment in 'for'.");
    }

    Statement* body = statement(parser);
    return Ast_NewForStmt(initializer, condition, increment, body);
}

Statement* for_in_stmt(Parser* parser, Declaration* declaration)
{
    if (declaration->as.variableDecl.value != NULL) {
        error(parser, "Variable in 'for-in' cannot be assigned.");
    }

    Expression* collection = expression(parser);
    consume(parser, TOKEN_R_PAREN, "Expected ')' after collection in 'for-in'.");
    Statement* body = statement(parser);
    return Ast_NewForInStmt(declaration, collection, body);
}

Statement* while_stmt(Parser* parser)
{
    consume(parser, TOKEN_L_PAREN, "Expected '(' before condition in 'while'.");
    Expression* condition = expression(parser);
    consume(parser, TOKEN_R_PAREN, "Expected ')' after condition in 'while'.");

    Statement* body = statement(parser);
    return Ast_NewWhileStmt(condition, body);
}

Statement* do_while_stmt(Parser* parser)
{
    Statement* body = statement(parser);
    consume(parser, TOKEN_WHILE, "Expected 'while' after 'do' body.");

    consume(parser, TOKEN_L_PAREN, "Expected '(' before condition in 'while'.");
    Expression* condition = expression(parser);
    consume(parser, TOKEN_R_PAREN, "Expected ')' after condition in 'while'.");
    consume(parser, TOKEN_SEMICOLON, "Expected ';' after 'do-while' statement.");

    return Ast_NewDoWhileStmt(body, condition);
}

Statement* break_stmt(Parser* parser)
{
    consume(parser, TOKEN_SEMICOLON, "Expected ';' at the end of statement.");
    return Ast_NewBreakStmt(parser->previous);
}

Statement* continue_stmt(Parser* parser)
{
    consume(parser, TOKEN_SEMICOLON, "Expected ';' at the end of statement.");
    return Ast_NewContinueStmt(parser->previous);
}

Statement* when_stmt(Parser* parser)
{
    consume(parser, TOKEN_L_PAREN, "Expected '(' before control expression in 'when'.");
    Expression* control = expression(parser);
    consume(parser, TOKEN_R_PAREN, "Expected ')' after control expression in 'when'.");

    consume(parser, TOKEN_L_BRACE, "Expected '{' before 'when' body.");

    WhenEntryList* entries = when_entries_rule(parser);

    Statement* elseBranch = NULL;
    if (match(parser, TOKEN_ELSE)) {
        consume(parser, TOKEN_R_ARROW, "Expected '->' after 'else' in 'when'.");
        elseBranch = statement(parser);
    }

    consume(parser, TOKEN_R_BRACE, "Expected '}' after 'when' body.");

    return Ast_NewWhenStmt(control, entries, elseBranch);
}

Statement* if_stmt(Parser* parser)
{
    consume(parser, TOKEN_L_PAREN, "Expected '(' before condition in 'if'.");
    Expression* condition = expression(parser);
    consume(parser, TOKEN_R_PAREN, "Expected ')' after condition in 'if'.");

    Statement* thenBranch = statement(parser);
    Statement* elseBranch = NULL;
    if (match(parser, TOKEN_ELSE)) {
        elseBranch = statement(parser);
    }

    return Ast_NewIfStmt(condition, thenBranch, elseBranch);
}

Statement* return_stmt(Parser* parser)
{
    Token keyword = parser->previous;
    Expression* expr = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        expr = expression(parser);
    }

    consume(parser, TOKEN_SEMICOLON, "Expected ';' at the end of 'return'.");
    return Ast_NewReturnStmt(keyword, expr);
}

Statement* print_stmt(Parser* parser)
{
    Expression* expr = expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expected ';' at the end of 'print'.");
    return Ast_NewPrintStmt(expr);
}

Statement* block_stmt(Parser* parser)
{
    return Ast_NewBlockStmt(block_rule(parser));
}

Statement* expression_stmt(Parser* parser)
{
    Expression* expr = expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expected ';' at the end of statement.");
    return Ast_NewExpressionStmt(expr);
}

Expression* expression(Parser* parser)
{
    return parse_precedence(parser, PREC_ASSIGNMENT);
}

Expression* literal_expr(Parser* parser)
{
    return Ast_NewLiteralExpr(parser->previous);
}

static void synchronize_interpolation(Parser* parser)
{
    int unmatched = 1;
    while (!reached_end(parser)) {
        if (parser->previous.type == TOKEN_STRING_INTERP_END && --unmatched == 0) {
            break;
        }

        if (parser->previous.type == TOKEN_STRING_INTERP && unmatched == 1) {
            break;
        }

        if (parser->previous.type == TOKEN_STRING_INTERP_BEGIN) {
            unmatched++;
        }

        error(parser, "Unexpected token in string interpolation.");
        advance(parser);
    }
}

Expression* string_interp_expr(Parser* parser)
{
    ExpressionList* values = NULL;
    if (parser->previous.length != 0) {
        Ast_ExpressionListAppend(&values, Ast_NewLiteralExpr(parser->previous));
    }

    while (parser->previous.type != TOKEN_STRING_INTERP_END && !reached_end(parser)) {
        Expression* expr = expression(parser);
        Ast_ExpressionListAppend(&values, expr);

        if (expr) {
            advance(parser);
        }

        synchronize_interpolation(parser);

        if (parser->previous.length != 0) {
            Ast_ExpressionListAppend(&values, Ast_NewLiteralExpr(parser->previous));
        }
    }

    return Ast_NewStringInterpExpr(values);
}

Expression* lambda_expr(Parser* parser)
{
    ParameterList* parameters = NULL;
    if (!check(parser, TOKEN_R_ARROW)) {
        parameters = parameters_rule(parser);
    }
    consume(parser, TOKEN_R_ARROW, "Expected '->' after lambda parameters.");

    FunctionBody* body = NULL;
    if (match(parser, TOKEN_L_BRACE)) {
        body = Ast_NewBlockFunctionBody(block_rule(parser));
    } else {
        body = Ast_NewExpressionFunctionBody(expression(parser));
    }

    Function* function = Ast_NewFunction(parameters, body);
    return Ast_NewLambdaExpr(function);
}

Expression* list_expr(Parser* parser)
{
    ExpressionList* elements = NULL;
    if (!check(parser, TOKEN_R_BRACKET)) {
        do {
            Ast_ExpressionListAppend(&elements, expression(parser));
        } while (match(parser, TOKEN_COMMA));
    }

    consume(parser, TOKEN_R_BRACKET, "Expected ']' after list expression.");

    return Ast_NewListExpr(elements);
}

Expression* map_expr(Parser* parser)
{
    MapEntryList* entries = NULL;
    if (!check(parser, TOKEN_R_BRACE)) {
        do {
            Expression* key = expression(parser);
            consume(parser, TOKEN_COLON, "Expected ':' after map key.");
            Expression* value = expression(parser);
            Ast_MapEntryListAppend(&entries, Ast_NewMapEntry(key, value));
        } while (match(parser, TOKEN_COMMA));
    }
    consume(parser, TOKEN_R_BRACE, "Expected '}' after map.");

    return Ast_NewMapExpr(entries);
}

Expression* identifier_expr(Parser* parser)
{
    return Ast_NewIdentifierExpr(parser->previous, LOAD);
}

static void set_assignment_context(Parser* parser, Expression* expr)
{
    if (expr->type == EXPR_IDENTIFIER) {
        expr->as.identifierExpr.context = STORE;
    } else if (expr->type == EXPR_PROPERTY) {
        expr->as.propertyExpr.context = STORE;
    } else if (expr->type == EXPR_SUBSCRIPT) {
        expr->as.subscriptExpr.context = STORE;
    } else {
        error(parser, "Invalid assignment target.");
    }
}

Expression* prefix_inc_expr(Parser* parser)
{
    Token op = parser->previous;
    Expression* expr = parse_precedence(parser, PREC_UNARY);
    set_assignment_context(parser, expr);
    return Ast_NewPrefixIncExpr(op, expr);
}

Expression* unary_expr(Parser* parser)
{
    Token op = parser->previous;
    Expression* expr = parse_precedence(parser, PREC_UNARY);
    return Ast_NewUnaryExpr(op, expr);
}

Expression* grouping_expr(Parser* parser)
{
    Expression* expr = expression(parser);
    if (match(parser, TOKEN_COMMA)) {
        ExpressionList* elements = NULL;
        Ast_ExpressionListAppend(&elements, expr);

        do {
            Ast_ExpressionListAppend(&elements, expression(parser));
        } while (match(parser, TOKEN_COMMA));

        consume(parser, TOKEN_R_PAREN, "Expected ')' after tuple expression.");
        return Ast_NewTupleExpr(elements);
    }

    consume(parser, TOKEN_R_PAREN, "Expected ')' after grouping expression.");
    return expr;
}

Expression* super_expr(Parser* parser)
{
    Token keyword = parser->previous;
    consume(parser, TOKEN_DOT, "Expected '.' after 'super'.");
    consume(parser, TOKEN_IDENTIFIER, "Expected superclass method name in 'super'.");
    Token method = parser->previous;
    return Ast_NewSuperExpr(keyword, method);
}

Expression* coroutine_expr(Parser* parser)
{
    Token keyword = parser->previous;
    Expression* expr = expression(parser);
    return Ast_NewCoroutineExpr(keyword, expr);
}

Expression* yield_expr(Parser* parser)
{
    Token keyword = parser->previous;
    Expression* expr = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        expr = expression(parser);
    }

    return Ast_NewYieldExpr(keyword, expr);
}

Expression* unpack_assignment_expr(Parser* parser)
{
    ExpressionList* targets = NULL;
    do {
        Expression* target = parse_precedence(parser, PREC_POSTFIX);
        set_assignment_context(parser, target);

        Ast_ExpressionListAppend(&targets, target);
    } while (match(parser, TOKEN_COMMA));

    consume(parser, TOKEN_PIPE, "Expected '|' at the end of unpacking assignment.");
    AssignmentTarget* target = Ast_NewUnpackAssignmentTarget(targets);

    consume(parser, TOKEN_EQUAL, "Expected '=' in unpacking assignment.");
    Expression* value = expression(parser);
    return Ast_NewAssignmentExpr(target, value);
}

Expression* call_expr(Parser* parser, Expression* prefix)
{
    ArgumentList* arguments = arguments_rule(parser);
    consume(parser, TOKEN_R_PAREN, "Expected ')' after call arguments.");
    return Ast_NewCallExpr(prefix, arguments);
}

Expression* range_expr(Parser* parser, Expression* prefix)
{
    Expression* end = parse_precedence(parser, PREC_CONDITIONAL);
    Expression* step = NULL;

    if (match(parser, TOKEN_COLON)) {
        step = parse_precedence(parser, PREC_CONDITIONAL);
    }

    return Ast_NewRangeExpr(prefix, end, step);
}

Expression* property_expr(Parser* parser, Expression* prefix)
{
    bool safe = parser->previous.type == TOKEN_QUESTION_DOT;
    consume(parser, TOKEN_IDENTIFIER, "Expected property name.");
    return Ast_NewPropertyExpr(prefix, parser->previous, LOAD, safe);
}

Expression* subscript_expr(Parser* parser, Expression* prefix)
{
    bool safe = parser->previous.type == TOKEN_QUESTION_L_BRACKET;
    Expression* index = expression(parser);
    consume(parser, TOKEN_R_BRACKET, "Expected ']' after subscript.");
    return Ast_NewSubscriptExpr(prefix, index, LOAD, safe);
}

Expression* postfix_inc_expr(Parser* parser, Expression* prefix)
{
    set_assignment_context(parser, prefix);
    Token op = parser->previous;
    return Ast_NewPostfixIncExpr(op, prefix);
}

Expression* binary_expr(Parser* parser, Expression* prefix)
{
    Token op = parser->previous;

    ParseRule* rule = get_rule(op.type);
    Precedence precedence = (rule->associativity == ASSOC_RIGHT) ? rule->precedence : rule->precedence + 1;

    Expression* right = parse_precedence(parser, precedence);
    return Ast_NewBinaryExpr(prefix, op, right);
}

Expression* assignment_expr(Parser* parser, Expression* prefix)
{
    set_assignment_context(parser, prefix);

    AssignmentTarget* target = Ast_NewSingleAssignmentTarget(prefix);
    Expression* value = parse_precedence(parser, PREC_ASSIGNMENT);
    return Ast_NewAssignmentExpr(target, value);
}

Expression* compound_assignment_expr(Parser* parser, Expression* prefix)
{
    set_assignment_context(parser, prefix);

    AssignmentTarget* target = Ast_NewSingleAssignmentTarget(prefix);
    Token op = parser->previous;
    Expression* value = parse_precedence(parser, PREC_ASSIGNMENT);
    return Ast_NewCompoundAssignmentExpr(target, op, value);
}

Expression* logical_expr(Parser* parser, Expression* prefix)
{
    Token op = parser->previous;
    ParseRule* rule = get_rule(op.type);
    Expression* right = parse_precedence(parser, rule->precedence);
    return Ast_NewLogicalExpr(prefix, op, right);
}

Expression* conditional_expr(Parser* parser, Expression* prefix)
{
    Expression* thenBranch = expression(parser);
    consume(parser, TOKEN_COLON, "Expected ':' in conditional expression.");
    Expression* elseBranch = parse_precedence(parser, PREC_CONDITIONAL);
    return Ast_NewConditionalExpr(prefix, thenBranch, elseBranch);
}

Expression* elvis_expr(Parser* parser, Expression* prefix)
{
    Expression* right = expression(parser);
    return Ast_NewElvisExpr(prefix, right);
}

WhenEntry* when_entry_rule(Parser* parser)
{
    ExpressionList* cases = NULL;
    do {
        Ast_ExpressionListAppend(&cases, expression(parser));
    } while (match(parser, TOKEN_COMMA));

    consume(parser, TOKEN_R_ARROW, "Expected '->' after 'when' cases.");

    Statement* body = statement(parser);

    return Ast_NewWhenEntry(cases, body);
}

WhenEntryList* when_entries_rule(Parser* parser)
{
    WhenEntryList* entries = NULL;
    while (!check(parser, TOKEN_ELSE) && !check(parser, TOKEN_R_BRACE) && !check(parser, TOKEN_EOF)) {
        Ast_WhenEntryListAppend(&entries, when_entry_rule(parser));
    }
    return entries;
}

Block* block_rule(Parser* parser)
{
    DeclarationList* body = NULL;
    while (!check(parser, TOKEN_R_BRACE) && !check(parser, TOKEN_EOF)) {
        Ast_DeclarationListAppend(&body, declaration(parser));
    }
    consume(parser, TOKEN_R_BRACE, "Expected '}' after block.");
    return Ast_NewBlock(body);
}

ArgumentList* arguments_rule(Parser* parser)
{
    ArgumentList* arguments = NULL;
    if (!check(parser, TOKEN_R_PAREN)) {
        do {
            if (Ast_ArgumentListLength(arguments) > 255) {
                error(parser, "Cannot have more than 255 arguments.");
            }
            Ast_ArgumentListAppend(&arguments, expression(parser));
        } while (match(parser, TOKEN_COMMA));
    }

    return arguments;
}

NamedFunction* named_function_rule(Parser* parser, bool coroutine)
{
    consume(parser, TOKEN_IDENTIFIER, "Expected function name in declaration.");
    Token identifier = parser->previous;

    consume(parser, TOKEN_L_PAREN, "Expected '(' after function name in declaration.");
    ParameterList* parameters = parameters_rule(parser);
    consume(parser, TOKEN_R_PAREN, "Expected ')' after function parameters in declaration.");

    FunctionBody* body = NULL;
    if (match(parser, TOKEN_EQUAL)) {
        body = Ast_NewExpressionFunctionBody(expression(parser));
        consume(parser, TOKEN_SEMICOLON, "Expected ';' after expression function.");
    } else {
        consume(parser, TOKEN_L_BRACE, "Expected '{' before function body in declaration.");
        body = Ast_NewBlockFunctionBody(block_rule(parser));
    }

    Function* function = Ast_NewFunction(parameters, body);
    return Ast_NewNamedFunction(identifier, function, coroutine);
}

Method* method_rule(Parser* parser)
{
    bool isStatic = match(parser, TOKEN_STATIC);
    bool isCoroutine = match(parser, TOKEN_COROUTINE);
    NamedFunction* namedFunction = named_function_rule(parser, isCoroutine);
    return Ast_NewMethod(isStatic, namedFunction);
}

ParameterList* parameters_rule(Parser* parser)
{
    ParameterList* parameters = NULL;
    if (!check(parser, TOKEN_R_PAREN)) {
        do {
            consume(parser, TOKEN_IDENTIFIER, "Expected parameter name.");
            Ast_ParameterListAppend(&parameters, parser->previous);
        } while (match(parser, TOKEN_COMMA));
    }
    return parameters;
}

AST* Parser_Parse(const char* source)
{
    Parser parser;
    parser_init(&parser, source);

    advance(&parser);

    DeclarationList* program = NULL;
    while (!match(&parser, TOKEN_EOF)) {
        Ast_DeclarationListAppend(&program, declaration(&parser));
    }

    if (parser.error) {
        Ast_DeleteDeclarationList(program);
        return NULL;
    }

    return Ast_NewTree(program);
}
