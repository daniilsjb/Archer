#include "ast.h"
#include "memlib.h"

Program* ast_new_program(DeclarationList* body)
{
    Program* program = raw_allocate(sizeof(Program));
    if (!program) {
        return NULL;
    }

    program->body = body;
    return program;
}

void ast_delete_program(Program* program)
{
    ast_delete_declaration_list(program->body);
    raw_deallocate(program);
}

void ast_delete_declaration(Declaration* declaration)
{
    switch (declaration->type) {
        case DECL_CLASS: ast_delete_class_decl(declaration); return;
        case DECL_FUNCTION: ast_delete_function_decl(declaration); return;
        case DECL_VARIABLE: ast_delete_variable_decl(declaration); return;
        case DECL_STATEMENT: ast_delete_statement_decl(declaration); return;
    }
}

Declaration* ast_new_class_decl(Token identifier, Token superclass, FunctionList* body)
{
    Declaration* decl = raw_allocate(sizeof(Declaration));
    if (!decl) {
        return NULL;
    }

    decl->type = DECL_CLASS;
    decl->as.classDecl.identifier = identifier;
    decl->as.classDecl.superclass = superclass;
    decl->as.classDecl.body = body;
    return decl;
}

void ast_delete_class_decl(Declaration* declaration)
{
    ast_delete_function_list(declaration->as.classDecl.body);
    raw_deallocate(declaration);
}

Declaration* ast_new_function_decl(Function* function)
{
    Declaration* decl = raw_allocate(sizeof(Declaration));
    if (!decl) {
        return NULL;
    }

    decl->type = DECL_FUNCTION;
    decl->as.functionDecl.function = function;
    return decl;
}

void ast_delete_function_decl(Declaration* declaration)
{
    ast_delete_function(declaration->as.functionDecl.function);
    raw_deallocate(declaration);
}

Declaration* ast_new_variable_decl(Token identifier, Expression* value)
{
    Declaration* decl = raw_allocate(sizeof(Declaration));
    if (!decl) {
        return NULL;
    }

    decl->type = DECL_VARIABLE;
    decl->as.variableDecl.identifier = identifier;
    decl->as.variableDecl.value = value;
    return decl;
}

void ast_delete_variable_decl(Declaration* declaration)
{
    if (declaration->as.variableDecl.value) {
        ast_delete_expression(declaration->as.variableDecl.value);
    }

    raw_deallocate(declaration);
}

Declaration* ast_new_statement_decl(Statement* statement)
{
    Declaration* decl = raw_allocate(sizeof(Declaration));
    if (!decl) {
        return NULL;
    }

    decl->type = DECL_STATEMENT;
    decl->as.statement = statement;
    return decl;
}

void ast_delete_statement_decl(Declaration* declaration)
{
    ast_delete_statement(declaration->as.statement);
    raw_deallocate(declaration);
}

void ast_delete_statement(Statement* statement)
{
    switch (statement->type) {
        case STMT_FOR: ast_delete_for_stmt(statement); return;
        case STMT_WHILE: ast_delete_while_stmt(statement); return;
        case STMT_IF: ast_delete_if_stmt(statement); return;
        case STMT_RETURN: ast_delete_return_stmt(statement); return;
        case STMT_PRINT: ast_delete_print_stmt(statement); return;
        case STMT_BLOCK: ast_delete_block_stmt(statement); return;
        case STMT_EXPRESSION: ast_delete_expression_stmt(statement); return;
    }
}

Statement* ast_new_for_stmt(Statement* initializer, Expression* condition, Expression* increment, Statement* body)
{
    Statement* stmt = raw_allocate(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_FOR;
    stmt->as.forStmt.initializer = initializer;
    stmt->as.forStmt.condition = condition;
    stmt->as.forStmt.increment = increment;
    stmt->as.forStmt.body = body;
    return stmt;
}

void ast_delete_for_stmt(Statement* statement)
{
    if (statement->as.forStmt.initializer) {
        ast_delete_statement(statement->as.forStmt.initializer);
    }

    if (statement->as.forStmt.condition) {
        ast_delete_expression(statement->as.forStmt.condition);
    }

    if (statement->as.forStmt.increment) {
        ast_delete_expression(statement->as.forStmt.increment);
    }

    raw_deallocate(statement);
}

Statement* ast_new_while_stmt(Expression* condition, Statement* body)
{
    Statement* stmt = raw_allocate(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_WHILE;
    stmt->as.whileStmt.condition = condition;
    stmt->as.whileStmt.body = body;
    return stmt;
}

void ast_delete_while_stmt(Statement* statement)
{
    ast_delete_expression(statement->as.whileStmt.condition);
    ast_delete_statement(statement->as.whileStmt.body);
    raw_deallocate(statement);
}

Statement* ast_new_if_stmt(Expression* condition, Statement* thenBranch, Statement* elseBranch)
{
    Statement* stmt = raw_allocate(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_IF;
    stmt->as.ifStmt.condition = condition;
    stmt->as.ifStmt.thenBranch = thenBranch;
    stmt->as.ifStmt.elseBranch = elseBranch;
    return stmt;
}

void ast_delete_if_stmt(Statement* statement)
{
    ast_delete_expression(statement->as.ifStmt.condition);
    ast_delete_statement(statement->as.ifStmt.thenBranch);

    if (statement->as.ifStmt.elseBranch) {
        ast_delete_statement(statement->as.ifStmt.elseBranch);
    }

    raw_deallocate(statement);
}

Statement* ast_new_return_stmt(Expression* expression)
{
    Statement* stmt = raw_allocate(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_RETURN;
    stmt->as.returnStmt.expression = expression;
    return stmt;
}

void ast_delete_return_stmt(Statement* statement)
{
    if (statement->as.returnStmt.expression) {
        ast_delete_expression(statement->as.returnStmt.expression);
    }

    raw_deallocate(statement);
}

Statement* ast_new_print_stmt(Expression* expression)
{
    Statement* stmt = raw_allocate(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_PRINT;
    stmt->as.printStmt.expression = expression;
    return stmt;
}

void ast_delete_print_stmt(Statement* statement)
{
    ast_delete_expression(statement->as.printStmt.expression);
    raw_deallocate(statement);
}

Statement* ast_new_block_stmt(DeclarationList* body)
{
    Statement* stmt = raw_allocate(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_BLOCK;
    stmt->as.blockStmt.body = body;
    return stmt;
}

void ast_delete_block_stmt(Statement* statement)
{
    ast_delete_declaration_list(statement->as.blockStmt.body);
    raw_deallocate(statement);
}

Statement* ast_new_expression_stmt(Expression* expression)
{
    Statement* stmt = raw_allocate(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_EXPRESSION;
    stmt->as.expression = expression;
    return stmt;
}

void ast_delete_expression_stmt(Statement* statement)
{
    ast_delete_expression(statement->as.expression);
    raw_deallocate(statement);
}

void ast_delete_expression(Expression* expression)
{
    switch (expression->type) {
        case EXPR_CALL: ast_delete_call_expr(expression); return;
        case EXPR_PROPERTY: ast_delete_property_expr(expression); return;
        case EXPR_THIS: ast_delete_this_expr(expression); return;
        case EXPR_SUPER: ast_delete_super_expr(expression); return;
        case EXPR_ASSIGNMENT: ast_delete_assignment_expr(expression); return;
        case EXPR_COMPOUND_ASSIGNMNET: ast_delete_compound_assignment_expr(expression); return;
        case EXPR_LOGICAL: ast_delete_logical_expr(expression); return;
        case EXPR_BINARY: ast_delete_binary_expr(expression); return;
        case EXPR_UNARY: ast_delete_unary_expr(expression); return;
        case EXPR_LITERAL: ast_delete_literal_expr(expression); return;
        case EXPR_IDENTIFIER: ast_delete_identifier_expr(expression); return;
    }
}

Expression* ast_new_call_expr(Expression* callee, ArgumentList* arguments)
{
    Expression* expr = raw_allocate(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_CALL;
    expr->as.callExpr.callee = callee;
    expr->as.callExpr.arguments = arguments;
    return expr;
}

void ast_delete_call_expr(Expression* expression)
{
    ast_delete_expression(expression->as.callExpr.callee);
    ast_delete_argument_list(expression->as.callExpr.arguments);
    raw_deallocate(expression);
}

Expression* ast_new_property_expr(Expression* object, Token property, ExprContext context)
{
    Expression* expr = raw_allocate(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_PROPERTY;
    expr->as.propertyExpr.object = object;
    expr->as.propertyExpr.property = property;
    expr->as.propertyExpr.context = context;
    return expr;
}

void ast_delete_property_expr(Expression* expression)
{
    ast_delete_expression(expression->as.propertyExpr.object);
    raw_deallocate(expression);
}

Expression* ast_new_this_expr(Token keyword)
{
    Expression* expr = raw_allocate(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_SUPER;
    expr->as.thisExpr.keyword = keyword;
    return expr;
}

void ast_delete_this_expr(Expression* expression)
{
    raw_deallocate(expression);
}

Expression* ast_new_super_expr(Token keyword, Token method)
{
    Expression* expr = raw_allocate(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_SUPER;
    expr->as.superExpr.keyword = keyword;
    expr->as.superExpr.method = method;
    return expr;
}

void ast_delete_super_expr(Expression* expression)
{
    raw_deallocate(expression);
}

Expression* ast_new_assignment_expr(Token target, Expression* value)
{
    Expression* expr = raw_allocate(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_ASSIGNMENT;
    expr->as.assignmentExpr.target = target;
    expr->as.assignmentExpr.value = value;
    return expr;
}

void ast_delete_assignment_expr(Expression* expression)
{
    ast_delete_expression(expression->as.assignmentExpr.value);
    raw_deallocate(expression);
}

Expression* ast_new_compound_assignment_expr(Token target, Token op, Expression* value)
{
    Expression* expr = raw_allocate(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_COMPOUND_ASSIGNMNET;
    expr->as.compoundAssignmentExpr.target = target;
    expr->as.compoundAssignmentExpr.op = op;
    expr->as.compoundAssignmentExpr.value = value;
    return expr;
}

void ast_delete_compound_assignment_expr(Expression* expression)
{
    ast_delete_expression(expression->as.compoundAssignmentExpr.value);
    raw_deallocate(expression);
}

Expression* ast_new_logical_expr(Expression* left, Token op, Expression* right)
{
    Expression* expr = raw_allocate(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_LOGICAL;
    expr->as.logicalExpr.left = left;
    expr->as.logicalExpr.op = op;
    expr->as.logicalExpr.right = right;
    return expr;
}

void ast_delete_logical_expr(Expression* expression)
{
    ast_delete_expression(expression->as.logicalExpr.left);
    ast_delete_expression(expression->as.logicalExpr.right);
    raw_deallocate(expression);
}

Expression* ast_new_binary_expr(Expression* left, Token op, Expression* right)
{
    Expression* expr = raw_allocate(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_BINARY;
    expr->as.logicalExpr.left = left;
    expr->as.logicalExpr.op = op;
    expr->as.logicalExpr.right = right;
    return expr;
}

void ast_delete_binary_expr(Expression* expression)
{
    ast_delete_expression(expression->as.binaryExpr.left);
    ast_delete_expression(expression->as.binaryExpr.right);
    raw_deallocate(expression);
}

Expression* ast_new_unary_expr(Token op, Expression* expression)
{
    Expression* expr = raw_allocate(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_UNARY;
    expr->as.unaryExpr.op = op;
    expr->as.unaryExpr.expression = expression;
    return expr;
}

void ast_delete_unary_expr(Expression* expression)
{
    ast_delete_expression(expression->as.unaryExpr.expression);
    raw_deallocate(expression);
}

Expression* ast_new_literal_expr(Token value)
{
    Expression* expr = raw_allocate(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_LITERAL;
    expr->as.literalExpr.value = value;
    return expr;
}

void ast_delete_literal_expr(Expression* expression)
{
    raw_deallocate(expression);
}

Expression* ast_new_identifier_expr(Token identifier, ExprContext context)
{
    Expression* expr = raw_allocate(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_IDENTIFIER;
    expr->as.identifierExpr.identifier = identifier;
    expr->as.identifierExpr.context = context;
    return expr;
}

void ast_delete_identifier_expr(Expression* expression)
{
    raw_deallocate(expression);
}

ArgumentList* ast_new_argument_list(Expression* expression)
{
    ArgumentList* list = raw_allocate(sizeof(ArgumentList));
    if (!list) {
        return NULL;
    }

    list->expression = expression;
    list->next = NULL;
    return list;
}

void ast_argument_list_append(ArgumentList* list, Expression* expression)
{
    list->next = ast_new_argument_list(expression);
}

void ast_delete_argument_list(ArgumentList* list)
{
    ArgumentList* current = list;
    while (current != NULL) {
        ArgumentList* next = current->next;
        ast_delete_expression(current->expression);
        raw_deallocate(current);
        current = next;
    }
}

ParameterList* ast_new_parameter_list(Token parameter)
{
    ParameterList* list = raw_allocate(sizeof(ParameterList));
    if (!list) {
        return NULL;
    }

    list->parameter = parameter;
    list->next = NULL;
    return list;
}

void ast_parameter_list_append(ParameterList* list, Token parameter)
{
    list->next = ast_new_parameter_list(parameter);
}

void ast_delete_parameter_list(ParameterList* list)
{
    ParameterList* current = list;
    while (current != NULL) {
        ParameterList* next = current->next;
        raw_deallocate(current);
        current = next;
    }
}

Function* ast_new_function(Token identifier, ParameterList* parameters, DeclarationList* body)
{
    Function* function = raw_allocate(sizeof(Function));
    if (!function) {
        return NULL;
    }

    function->identifier = identifier;
    function->parameters = parameters;
    function->body = body;
    return function;
}

void ast_delete_function(Function* function)
{
    ast_delete_parameter_list(function->parameters);
    ast_delete_declaration_list(function->body);
    raw_deallocate(function);
}

FunctionList* ast_new_function_list(Function* function)
{
    FunctionList* list = raw_allocate(sizeof(FunctionList));
    if (!list) {
        return NULL;
    }

    list->function = function;
    list->next = NULL;
    return list;
}

void ast_function_list_append(FunctionList* list, Function* function)
{
    list->next = ast_new_function_list(function);
}

void ast_delete_function_list(FunctionList* list)
{
    FunctionList* current = list;
    while (current != NULL) {
        FunctionList* next = current->next;
        ast_delete_function(current->function);
        raw_deallocate(current);
        current = next;
    }
}

DeclarationList* ast_new_declaration_list(Declaration* declaration)
{
    DeclarationList* list = raw_allocate(sizeof(DeclarationList));
    if (!list) {
        return NULL;
    }

    list->declaration = declaration;
    list->next = NULL;
    return list;
}

void ast_declaration_list_append(DeclarationList* list, Declaration* declaration)
{
    list->next = ast_new_declaration_list(declaration);
}

void ast_delete_declaration_list(DeclarationList* list)
{
    DeclarationList* current = list;
    while (current != NULL) {
        DeclarationList* next = current->next;
        ast_delete_declaration(current->declaration);
        raw_deallocate(current);
        current = next;
    }
}
