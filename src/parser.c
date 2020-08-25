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
static Declaration* class_decl(Parser* parser);
static Declaration* function_decl(Parser* parser);
static Declaration* variable_decl(Parser* parser);
static Declaration* statement_decl(Parser* parser);

static Statement* statement(Parser* parser);
static Statement* for_stmt(Parser* parser);
static Statement* while_stmt(Parser* parser);
static Statement* if_stmt(Parser* parser);
static Statement* return_stmt(Parser* parser);
static Statement* print_stmt(Parser* parser);
static Statement* block_stmt(Parser* parser);
static Statement* expression_stmt(Parser* parser);

static Expression* expression(Parser* parser);
static Expression* literal_expr(Parser* parser);
static Expression* identifier_expr(Parser* parser);
static Expression* prefix_inc_expr(Parser* parser);
static Expression* unary_expr(Parser* parser);
static Expression* grouping_expr(Parser* parser);
static Expression* super_expr(Parser* parser);
static Expression* call_expr(Parser* parser, Expression* prefix);
static Expression* property_expr(Parser* parser, Expression* prefix);
static Expression* postfix_inc_expr(Parser* parser, Expression* prefix);
static Expression* assignment_expr(Parser* parser, Expression* prefix);
static Expression* compound_assignment_expr(Parser* parser, Expression* prefix);
static Expression* logical_expr(Parser* parser, Expression* prefix);
static Expression* conditional_expr(Parser* parser, Expression* prefix);
static Expression* binary_expr(Parser* parser, Expression* prefix);

static ArgumentList* arguments_rule(Parser* parser);
static Function* function_rule(Parser* parser);
static ParameterList* parameters_rule(Parser* parser);

typedef struct {
    PrefixParselet prefix;
    InfixParselet infix;
    Precedence precedence;
    Associativity associativity;
} ParseRule;

ParseRule rules[] = {
    [TOKEN_L_PAREN]           = { grouping_expr,   call_expr,                PREC_POSTFIX,        ASSOC_LEFT   },
    [TOKEN_R_PAREN]           = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_L_BRACE]           = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_R_BRACE]           = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_L_BRACKET]         = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_R_BRACKET]         = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_COMMA]             = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_DOT]               = { NULL,            property_expr,            PREC_POSTFIX,        ASSOC_LEFT   },
    [TOKEN_SEMICOLON]         = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_QUESTION]          = { NULL,            conditional_expr,         PREC_CONDITIONAL,    ASSOC_RIGHT  },
    [TOKEN_COLON]             = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_TILDE]             = { unary_expr,      NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_MINUS]             = { unary_expr,      binary_expr,              PREC_ADDITIVE,       ASSOC_LEFT   },
    [TOKEN_MINUS_EQUAL]       = { NULL,            compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_DOUBLE_MINUS]      = { prefix_inc_expr, postfix_inc_expr,         PREC_POSTFIX,        ASSOC_LEFT   },
    [TOKEN_PLUS]              = { NULL,            binary_expr,              PREC_ADDITIVE,       ASSOC_LEFT   },
    [TOKEN_PLUS_EQUAL]        = { NULL,            compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_DOUBLE_PLUS]       = { prefix_inc_expr, postfix_inc_expr,         PREC_POSTFIX,        ASSOC_LEFT   },
    [TOKEN_STAR]              = { NULL,            binary_expr,              PREC_MULTIPLICATIVE, ASSOC_LEFT   },
    [TOKEN_STAR_EQUAL]        = { NULL,            compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_DOUBLE_STAR]       = { NULL,            binary_expr,              PREC_EXPONENTIATION, ASSOC_RIGHT  },
    [TOKEN_DOUBLE_STAR_EQUAL] = { NULL,            compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_SLASH]             = { NULL,            binary_expr,              PREC_MULTIPLICATIVE, ASSOC_LEFT   },
    [TOKEN_SLASH_EQUAL]       = { NULL,            compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_PERCENT]           = { NULL,            binary_expr,              PREC_MULTIPLICATIVE, ASSOC_LEFT   },
    [TOKEN_PERCENT_EQUAL]     = { NULL,            compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_AMPERSAND]         = { NULL,            binary_expr,              PREC_BITWISE_AND,    ASSOC_LEFT   },
    [TOKEN_AMPERSAND_EQUAL]   = { NULL,            compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_PIPE]              = { NULL,            binary_expr,              PREC_BITWISE_OR,     ASSOC_LEFT   },
    [TOKEN_PIPE_EQUAL]        = { NULL,            compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_CARET]             = { NULL,            binary_expr,              PREC_BITWISE_XOR,    ASSOC_LEFT   },
    [TOKEN_CARET_EQUAL]       = { NULL,            compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_BANG]              = { unary_expr,      NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_BANG_EQUAL]        = { NULL,            binary_expr,              PREC_EQUALITY,       ASSOC_LEFT   },
    [TOKEN_EQUAL]             = { NULL,            assignment_expr,          PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_EQUAL_EQUAL]       = { NULL,            binary_expr,              PREC_EQUALITY,       ASSOC_LEFT   },
    [TOKEN_GREATER]           = { NULL,            binary_expr,              PREC_RELATIONAL,     ASSOC_LEFT   },
    [TOKEN_GREATER_EQUAL]     = { NULL,            binary_expr,              PREC_RELATIONAL,     ASSOC_LEFT   },
    [TOKEN_R_SHIFT]           = { NULL,            binary_expr,              PREC_SHIFT,          ASSOC_LEFT   },
    [TOKEN_R_SHIFT_EQUAL]     = { NULL,            compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_LESS]              = { NULL,            binary_expr,              PREC_RELATIONAL,     ASSOC_LEFT   },
    [TOKEN_LESS_EQUAL]        = { NULL,            binary_expr,              PREC_RELATIONAL,     ASSOC_LEFT   },
    [TOKEN_L_SHIFT]           = { NULL,            binary_expr,              PREC_SHIFT,          ASSOC_LEFT   },
    [TOKEN_L_SHIFT_EQUAL]     = { NULL,            compound_assignment_expr, PREC_ASSIGNMENT,     ASSOC_RIGHT  },
    [TOKEN_AND]               = { NULL,            logical_expr,             PREC_LOGICAL_AND,    ASSOC_LEFT   },
    [TOKEN_BREAK]             = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_CLASS]             = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_CONTINUE]          = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_DEFAULT]           = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_ELSE]              = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_FALSE]             = { literal_expr,    NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_FUN]               = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_FOR]               = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_IF]                = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_NIL]               = { literal_expr,    NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_OR]                = { NULL,            logical_expr,             PREC_LOGICAL_OR,     ASSOC_LEFT   },
    [TOKEN_PRINT]             = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_RETURN]            = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_STATIC]            = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_SUPER]             = { super_expr,      NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_THIS]              = { literal_expr,    NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_TRUE]              = { literal_expr,    NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_VAR]               = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_VAR]               = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_WHILE]             = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_IDENTIFIER]        = { identifier_expr, NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_STRING]            = { literal_expr,    NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_NUMBER]            = { literal_expr,    NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_ERROR]             = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_NONE]              = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
    [TOKEN_EOF]               = { NULL,            NULL,                     PREC_NONE,           ASSOC_NONE   },
};

static void parser_init(Parser* parser, const char* source)
{
    scanner_init(&parser->scanner, source);
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
    parser->current = scanner_scan_token(&parser->scanner);
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

static void synchronize(Parser* parser)
{
    parser->panic = false;

    while (!check(parser, TOKEN_EOF)) {
        switch (parser->previous.type) {
            case TOKEN_SEMICOLON: return;
        }

        switch (parser->current.type) {
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

Declaration* declaration(Parser* parser)
{
    if (parser->panic) {
        synchronize(parser);
    }

    switch (parser->current.type) {
        case TOKEN_CLASS: advance(parser); return class_decl(parser);
        case TOKEN_FUN: advance(parser); return function_decl(parser);
        case TOKEN_VAR: advance(parser); return variable_decl(parser);
        default: return statement_decl(parser);
    }
}

Declaration* class_decl(Parser* parser)
{
    consume(parser, TOKEN_IDENTIFIER, "Expected class name in declaration.");
    Token identifier = parser->previous;

    Token superclass = empty_token();
    if (match(parser, TOKEN_LESS)) {
        consume(parser, TOKEN_IDENTIFIER, "Expected superclass name in declaration.");
        superclass = parser->previous;
    }

    FunctionList* body = NULL;
    consume(parser, TOKEN_L_BRACE, "Expected '{' before class body in declaration.");
    while (!check(parser, TOKEN_R_BRACE) && !check(parser, TOKEN_EOF)) {
        ast_function_list_append(&body, function_rule(parser));
    }
    consume(parser, TOKEN_R_BRACE, "Expected '}' after class body in declaration.");

    return ast_new_class_decl(identifier, superclass, body);
}

Declaration* function_decl(Parser* parser)
{
    return ast_new_function_decl(function_rule(parser));
}

Declaration* variable_decl(Parser* parser)
{
    consume(parser, TOKEN_IDENTIFIER, "Expected variable name in declaration.");
    Token identifier = parser->previous;

    Expression* value = NULL;
    if (match(parser, TOKEN_EQUAL)) {
        value = expression(parser);
    }

    consume(parser, TOKEN_SEMICOLON, "Expected ';' after variable declaration.");
    return ast_new_variable_decl(identifier, value);
}

Declaration* statement_decl(Parser* parser)
{
    return ast_new_statement_decl(statement(parser));
}

Statement* statement(Parser* parser)
{
    switch (parser->current.type) {
        case TOKEN_FOR: advance(parser); return for_stmt(parser);
        case TOKEN_WHILE: advance(parser); return while_stmt(parser);
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
        initializer = variable_decl(parser);
    } else if (!match(parser, TOKEN_SEMICOLON)) {
        initializer = ast_new_statement_decl(expression_stmt(parser));
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
    return ast_new_for_stmt(initializer, condition, increment, body);
}

Statement* while_stmt(Parser* parser)
{
    consume(parser, TOKEN_L_PAREN, "Expected '(' before condition in 'while'.");
    Expression* condition = expression(parser);
    consume(parser, TOKEN_R_PAREN, "Expected ')' after condition in 'while'.");

    Statement* body = statement(parser);
    return ast_new_while_stmt(condition, body);
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

    return ast_new_if_stmt(condition, thenBranch, elseBranch);
}

Statement* return_stmt(Parser* parser)
{
    Token keyword = parser->previous;
    Expression* expr = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        expr = expression(parser);
    }

    consume(parser, TOKEN_SEMICOLON, "Expected ';' at the end of 'return'.");
    return ast_new_return_stmt(keyword, expr);
}

Statement* print_stmt(Parser* parser)
{
    Expression* expr = expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expected ';' at the end of 'print'.");
    return ast_new_print_stmt(expr);
}

Statement* block_stmt(Parser* parser)
{
    DeclarationList* body = NULL;
    while (!check(parser, TOKEN_R_BRACE) && !check(parser, TOKEN_EOF)) {
        ast_declaration_list_append(&body, declaration(parser));
    }
    consume(parser, TOKEN_R_BRACE, "Expected '}' after block.");
    return ast_new_block_stmt(body);
}

Statement* expression_stmt(Parser* parser)
{
    Expression* expr = expression(parser);
    consume(parser, TOKEN_SEMICOLON, "Expected ';' at the end of statement.");
    return ast_new_expression_stmt(expr);
}

Expression* expression(Parser* parser)
{
    return parse_precedence(parser, PREC_ASSIGNMENT);
}

Expression* literal_expr(Parser* parser)
{
    return ast_new_literal_expr(parser->previous);
}

Expression* identifier_expr(Parser* parser)
{
    return ast_new_identifier_expr(parser->previous, LOAD);
}

static void set_assignment_context(Parser* parser, Expression* expr)
{
    if (expr->type == EXPR_IDENTIFIER) {
        expr->as.identifierExpr.context = STORE;
    } else if (expr->type == EXPR_PROPERTY) {
        expr->as.propertyExpr.context = STORE;
    } else {
        error(parser, "Invalid assignment target.");
    }
}

Expression* prefix_inc_expr(Parser* parser)
{
    Token op = parser->previous;
    Expression* expr = parse_precedence(parser, PREC_UNARY);
    set_assignment_context(parser, expr);
    return ast_new_prefix_inc_expr(op, expr);
}

Expression* unary_expr(Parser* parser)
{
    Token op = parser->previous;
    Expression* expr = parse_precedence(parser, PREC_UNARY);
    return ast_new_unary_expr(op, expr);
}

Expression* grouping_expr(Parser* parser)
{
    Expression* expr = expression(parser);
    consume(parser, TOKEN_R_PAREN, "Expected ')' after grouping expression.");
    return expr;
}

Expression* super_expr(Parser* parser)
{
    Token keyword = parser->previous;
    consume(parser, TOKEN_DOT, "Expected '.' after 'super'.");
    consume(parser, TOKEN_IDENTIFIER, "Expected superclass method name in 'super'.");
    Token method = parser->previous;
    return ast_new_super_expr(keyword, method);
}

Expression* call_expr(Parser* parser, Expression* prefix)
{
    ArgumentList* arguments = arguments_rule(parser);
    consume(parser, TOKEN_R_PAREN, "Expected ')' after call arguments.");
    return ast_new_call_expr(prefix, arguments);
}

Expression* property_expr(Parser* parser, Expression* prefix)
{
    consume(parser, TOKEN_IDENTIFIER, "Expected property name.");
    return ast_new_property_expr(prefix, parser->previous, LOAD);
}

Expression* postfix_inc_expr(Parser* parser, Expression* prefix)
{
    set_assignment_context(parser, prefix);
    Token op = parser->previous;
    return ast_new_postfix_inc_expr(op, prefix);
}

Expression* binary_expr(Parser* parser, Expression* prefix)
{
    Token op = parser->previous;

    ParseRule* rule = get_rule(op.type);
    Precedence precedence = (rule->associativity == ASSOC_RIGHT) ? rule->precedence : rule->precedence + 1;

    Expression* right = parse_precedence(parser, precedence);
    return ast_new_binary_expr(prefix, op, right);
}

Expression* assignment_expr(Parser* parser, Expression* prefix)
{
    set_assignment_context(parser, prefix);

    Expression* value = parse_precedence(parser, PREC_ASSIGNMENT);
    return ast_new_assignment_expr(prefix, value);
}

Expression* compound_assignment_expr(Parser* parser, Expression* prefix)
{
    set_assignment_context(parser, prefix);

    Token op = parser->previous;
    Expression* value = parse_precedence(parser, PREC_ASSIGNMENT);
    return ast_new_compound_assignment_expr(prefix, op, value);
}

Expression* logical_expr(Parser* parser, Expression* prefix)
{
    Token op = parser->previous;
    ParseRule* rule = get_rule(op.type);
    Expression* right = parse_precedence(parser, rule->precedence);
    return ast_new_logical_expr(prefix, op, right);
}

Expression* conditional_expr(Parser* parser, Expression* prefix)
{
    Expression* thenBranch = expression(parser);
    consume(parser, TOKEN_COLON, "Expected ':' in conditional expression.");
    Expression* elseBranch = parse_precedence(parser, PREC_CONDITIONAL);
    return ast_new_conditional_expr(prefix, thenBranch, elseBranch);
}

ArgumentList* arguments_rule(Parser* parser)
{
    ArgumentList* arguments = NULL;
    if (!check(parser, TOKEN_R_PAREN)) {
        do {
            if (ast_argument_list_length(arguments) > 255) {
                error(parser, "Cannot have more than 255 arguments.");
            }
            ast_argument_list_append(&arguments, expression(parser));
        } while (match(parser, TOKEN_COMMA));
    }

    return arguments;
}

Function* function_rule(Parser* parser)
{
    consume(parser, TOKEN_IDENTIFIER, "Expected function name in declaration.");
    Token identifier = parser->previous;
    
    consume(parser, TOKEN_L_PAREN, "Expected '(' after function name in declaration.");
    ParameterList* parameters = parameters_rule(parser);
    consume(parser, TOKEN_R_PAREN, "Expected ')' after function parameters in declaration.");

    DeclarationList* body = NULL;
    consume(parser, TOKEN_L_BRACE, "Expected '{' before function body in declaration.");
    body = block_stmt(parser)->as.blockStmt.body;

    return ast_new_function(identifier, parameters, body);
}

ParameterList* parameters_rule(Parser* parser)
{
    ParameterList* parameters = NULL;
    if (!check(parser, TOKEN_R_PAREN)) {
        do {
            consume(parser, TOKEN_IDENTIFIER, "Expected parameter name.");
            ast_parameter_list_append(&parameters, parser->previous);
        } while (match(parser, TOKEN_COMMA));
    }
    return parameters;
}

AST* parse(const char* source)
{
    Parser parser;
    parser_init(&parser, source);

    advance(&parser);

    DeclarationList* program = NULL;
    while (!match(&parser, TOKEN_EOF)) {
        ast_declaration_list_append(&program, declaration(&parser));
    }

    if (parser.error) {
        ast_delete_declaration_list(program);
        return NULL;
    }

    return ast_new_tree(program);
}
