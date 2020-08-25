#ifndef AST_H
#define AST_H

#include "token.h"

typedef struct Declaration Declaration;
typedef struct DeclarationList DeclarationList;

typedef struct Statement Statement;

typedef struct Expression Expression;
typedef struct ArgumentList ArgumentList;

typedef struct Function Function;
typedef struct FunctionList FunctionList;

typedef enum ExprContext { LOAD, STORE } ExprContext;

typedef struct AST {
    DeclarationList* body;
} AST;

typedef struct Declaration {
    enum {
        DECL_CLASS,
        DECL_FUNCTION,
        DECL_VARIABLE,
        DECL_STATEMENT
    } type;

    union {
        struct {
            Token identifier;
            Token superclass;
            FunctionList* body;
        } classDecl;

        struct {
            Function* function;
        } functionDecl;

        struct {
            Token identifier;
            Expression* value;
        } variableDecl;

        Statement* statement;
    } as;
} Declaration;

typedef struct Statement {
    enum {
        STMT_FOR,
        STMT_WHILE,
        STMT_IF,
        STMT_RETURN,
        STMT_PRINT,
        STMT_BLOCK,
        STMT_EXPRESSION
    } type;

    union {
        struct {
            Declaration* initializer;
            Expression* condition;
            Expression* increment;
            Statement* body;
        } forStmt;

        struct {
            Expression* condition;
            Statement* body;
        } whileStmt;

        struct {
            Expression* condition;
            Statement* thenBranch;
            Statement* elseBranch;
        } ifStmt;

        struct {
            Token keyword;
            Expression* expression;
        } returnStmt;

        struct {
            Expression* expression;
        } printStmt;

        struct {
            DeclarationList* body;
        } blockStmt;

        Expression* expression;
    } as;
} Statement;

typedef struct Expression {
    enum {
        EXPR_CALL,
        EXPR_PROPERTY,
        EXPR_SUPER,
        EXPR_ASSIGNMENT,
        EXPR_COMPOUND_ASSIGNMNET,
        EXPR_LOGICAL,
        EXPR_CONDITIONAL,
        EXPR_BINARY,
        EXPR_UNARY,
        EXPR_PREFIX_INC,
        EXPR_POSTFIX_INC,
        EXPR_LITERAL,
        EXPR_IDENTIFIER
    } type;

    union {
        struct {
            Expression* callee;
            ArgumentList* arguments;
        } callExpr;

        struct {
            Expression* object;
            Token property;
            ExprContext context;
        } propertyExpr;

        struct {
            Token keyword;
            Token method;
        } superExpr;

        struct {
            Expression* target;
            Expression* value;
        } assignmentExpr;

        struct {
            Expression* target;
            Token op;
            Expression* value;
        } compoundAssignmentExpr;

        struct {
            Expression* left;
            Token op;
            Expression* right;
        } logicalExpr;

        struct {
            Expression* condition;
            Expression* thenBranch;
            Expression* elseBranch;
        } conditionalExpr;

        struct {
            Expression* left;
            Token op;
            Expression* right;
        } binaryExpr;

        struct {
            Token op;
            Expression* target;
        } prefixIncExpr;

        struct {
            Token op;
            Expression* target;
        } postfixIncExpr;

        struct {
            Token op;
            Expression* expression;
        } unaryExpr;

        struct {
            Token value;
        } literalExpr;

        struct {
            Token identifier;
            ExprContext context;
        } identifierExpr;
    } as;
} Expression;

typedef struct ArgumentList {
    Expression* expression;
    ArgumentList* next;
} ArgumentList;

typedef struct ParameterList {
    Token parameter;
    struct ParameterList* next;
} ParameterList;

typedef struct Function {
    Token identifier;
    ParameterList* parameters;
    DeclarationList* body;
} Function;

typedef struct FunctionList {
    Function* function;
    FunctionList* next;
} FunctionList;

typedef struct DeclarationList {
    Declaration* declaration;
    DeclarationList* next;
} DeclarationList;

AST* ast_new_tree(DeclarationList* body);
void ast_delete_tree(AST* ast);

void ast_delete_declaration(Declaration* declaration);
Declaration* ast_new_class_decl(Token identifier, Token superclass, FunctionList* body);
void ast_delete_class_decl(Declaration* declaration);
Declaration* ast_new_function_decl(Function* function);
void ast_delete_function_decl(Declaration* declaration);
Declaration* ast_new_variable_decl(Token identifier, Expression* value);
void ast_delete_variable_decl(Declaration* declaration);
Declaration* ast_new_statement_decl(Statement* statement);
void ast_delete_statement_decl(Declaration* declaration);

void ast_delete_statement(Statement* statement);
Statement* ast_new_for_stmt(Declaration* initializer, Expression* condition, Expression* increment, Statement* body);
void ast_delete_for_stmt(Statement* statement);
Statement* ast_new_while_stmt(Expression* condition, Statement* body);
void ast_delete_while_stmt(Statement* statement);
Statement* ast_new_if_stmt(Expression* condition, Statement* thenBranch, Statement* elseBranch);
void ast_delete_if_stmt(Statement* statement);
Statement* ast_new_return_stmt(Token keyword, Expression* expression);
void ast_delete_return_stmt(Statement* statement);
Statement* ast_new_print_stmt(Expression* expression);
void ast_delete_print_stmt(Statement* statement);
Statement* ast_new_block_stmt(DeclarationList* body);
void ast_delete_block_stmt(Statement* statement);
Statement* ast_new_expression_stmt(Expression* expression);
void ast_delete_expression_stmt(Statement* statement);

void ast_delete_expression(Expression* expression);
Expression* ast_new_call_expr(Expression* callee, ArgumentList* arguments);
void ast_delete_call_expr(Expression* expression);
Expression* ast_new_property_expr(Expression* object, Token property, ExprContext context);
void ast_delete_property_expr(Expression* expression);
Expression* ast_new_super_expr(Token keyword, Token method);
void ast_delete_super_expr(Expression* expression);
Expression* ast_new_assignment_expr(Expression* target, Expression* value);
void ast_delete_assignment_expr(Expression* expression);
Expression* ast_new_compound_assignment_expr(Expression*, Token op, Expression* value);
void ast_delete_compound_assignment_expr(Expression* expression);
Expression* ast_new_logical_expr(Expression* left, Token op, Expression* right);
void ast_delete_logical_expr(Expression* expression);
Expression* ast_new_conditional_expr(Expression* condition, Expression* thenBranch, Expression* elseBranch);
void ast_delete_conditional_expr(Expression* expression);
Expression* ast_new_binary_expr(Expression* left, Token op, Expression* right);
void ast_delete_binary_expr(Expression* expression);
Expression* ast_new_unary_expr(Token op, Expression* expression);
void ast_delete_unary_expr(Expression* expression);
Expression* ast_new_postfix_inc_expr(Token op, Expression* expression);
void ast_delete_postfix_inc_expr(Expression* expression);
Expression* ast_new_prefix_inc_expr(Token op, Expression* expression);
void ast_delete_prefix_inc_expr(Expression* expression);
Expression* ast_new_literal_expr(Token value);
void ast_delete_literal_expr(Expression* expression);
Expression* ast_new_identifier_expr(Token identifier, ExprContext context);
void ast_delete_identifier_expr(Expression* expression);

ArgumentList* ast_new_argument_node(Expression* expression);
void ast_argument_list_append(ArgumentList** list, Expression* expression);
void ast_delete_argument_list(ArgumentList* list);
size_t ast_argument_list_length(ArgumentList* list);

ParameterList* ast_new_parameter_node(Token parameter);
void ast_parameter_list_append(ParameterList** list, Token parameter);
void ast_delete_parameter_list(ParameterList* list);
size_t ast_parameter_list_length(ParameterList* list);

Function* ast_new_function(Token identifier, ParameterList* parameters, DeclarationList* body);
void ast_delete_function(Function* function);

FunctionList* ast_new_function_node(Function* function);
void ast_function_list_append(FunctionList** list, Function* function);
void ast_delete_function_list(FunctionList* list);
size_t ast_function_list_length(FunctionList* list);

DeclarationList* ast_new_declaration_node(Declaration* declaration);
void ast_declaration_list_append(DeclarationList** list, Declaration* declaration);
void ast_delete_declaration_list(DeclarationList* list);
size_t ast_declaration_list_length(DeclarationList* list);

#endif
