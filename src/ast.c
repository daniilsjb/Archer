#include <stdint.h>

#include "ast.h"
#include "memlib.h"

AST* ast_new_tree(DeclarationList* body)
{
    AST* ast = raw_allocate(sizeof(AST));
    if (!ast) {
        return NULL;
    }

    ast->body = body;
    return ast;
}

void ast_delete_tree(AST* ast)
{
    if (!ast) {
        return;
    }

    ast_delete_declaration_list(ast->body);
    raw_deallocate(ast);
}

void ast_delete_declaration(Declaration* declaration)
{
    if (!declaration) {
        return;
    }

    switch (declaration->type) {
        case DECL_CLASS: ast_delete_class_decl(declaration); return;
        case DECL_FUNCTION: ast_delete_function_decl(declaration); return;
        case DECL_VARIABLE: ast_delete_variable_decl(declaration); return;
        case DECL_STATEMENT: ast_delete_statement_decl(declaration); return;
    }
}

Declaration* ast_new_class_decl(Token identifier, Token superclass, MethodList* body)
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
    ast_delete_method_list(declaration->as.classDecl.body);
    raw_deallocate(declaration);
}

Declaration* ast_new_function_decl(NamedFunction* function)
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
    ast_delete_named_function(declaration->as.functionDecl.function);
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
    ast_delete_expression(declaration->as.variableDecl.value);
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
    if (!statement) {
        return;
    }

    switch (statement->type) {
        case STMT_FOR: ast_delete_for_stmt(statement); return;
        case STMT_WHILE: ast_delete_while_stmt(statement); return;
        case STMT_BREAK: ast_delete_break_stmt(statement); return;
        case STMT_CONTINUE: ast_delete_continue_stmt(statement); return;
        case STMT_WHEN: ast_delete_when_stmt(statement); return;
        case STMT_IF: ast_delete_if_stmt(statement); return;
        case STMT_RETURN: ast_delete_return_stmt(statement); return;
        case STMT_PRINT: ast_delete_print_stmt(statement); return;
        case STMT_BLOCK: ast_delete_block_stmt(statement); return;
        case STMT_EXPRESSION: ast_delete_expression_stmt(statement); return;
    }
}

Statement* ast_new_for_stmt(Declaration* initializer, Expression* condition, Expression* increment, Statement* body)
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
    ast_delete_declaration(statement->as.forStmt.initializer);
    ast_delete_expression(statement->as.forStmt.condition);
    ast_delete_expression(statement->as.forStmt.increment);
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

Statement* ast_new_break_stmt(Token keyword)
{
    Statement* stmt = raw_allocate(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_BREAK;
    stmt->as.breakStmt.keyword = keyword;
    return stmt;
}

void ast_delete_break_stmt(Statement* statement)
{
    raw_deallocate(statement);
}

Statement* ast_new_continue_stmt(Token keyword)
{
    Statement* stmt = raw_allocate(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_CONTINUE;
    stmt->as.continueStmt.keyword = keyword;
    return stmt;
}

void ast_delete_continue_stmt(Statement* statement)
{
    raw_deallocate(statement);
}

Statement* ast_new_when_stmt(Expression* control, WhenEntryList* entries, Statement* elseBranch)
{
    Statement* stmt = raw_allocate(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_WHEN;
    stmt->as.whenStmt.control = control;
    stmt->as.whenStmt.entries = entries;
    stmt->as.whenStmt.elseBranch = elseBranch;
    return stmt;
}

void ast_delete_when_stmt(Statement* statement)
{
    ast_delete_expression(statement->as.whenStmt.control);
    ast_delete_when_entry_list(statement->as.whenStmt.entries);
    ast_delete_statement(statement->as.whenStmt.elseBranch);
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
    ast_delete_statement(statement->as.ifStmt.elseBranch);
    raw_deallocate(statement);
}

Statement* ast_new_return_stmt(Token keyword, Expression* expression)
{
    Statement* stmt = raw_allocate(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_RETURN;
    stmt->as.returnStmt.keyword = keyword;
    stmt->as.returnStmt.expression = expression;
    return stmt;
}

void ast_delete_return_stmt(Statement* statement)
{
    ast_delete_expression(statement->as.returnStmt.expression);
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

Statement* ast_new_block_stmt(Block* block)
{
    Statement* stmt = raw_allocate(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_BLOCK;
    stmt->as.blockStmt.block = block;
    return stmt;
}

void ast_delete_block_stmt(Statement* statement)
{
    ast_delete_block(statement->as.blockStmt.block);
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
    if (!expression) {
        return;
    }

    switch (expression->type) {
        case EXPR_CALL: ast_delete_call_expr(expression); return;
        case EXPR_PROPERTY: ast_delete_property_expr(expression); return;
        case EXPR_SUBSCRIPT: ast_delete_subscript_expr(expression); return;
        case EXPR_SUPER: ast_delete_super_expr(expression); return;
        case EXPR_ASSIGNMENT: ast_delete_assignment_expr(expression); return;
        case EXPR_COMPOUND_ASSIGNMNET: ast_delete_compound_assignment_expr(expression); return;
        case EXPR_POSTFIX_INC: ast_delete_postfix_inc_expr(expression); return;
        case EXPR_PREFIX_INC: ast_delete_prefix_inc_expr(expression); return;
        case EXPR_LOGICAL: ast_delete_logical_expr(expression); return;
        case EXPR_CONDITIONAL: ast_delete_conditional_expr(expression); return;
        case EXPR_ELVIS: ast_delete_elvis_expr(expression); return;
        case EXPR_BINARY: ast_delete_binary_expr(expression); return;
        case EXPR_UNARY: ast_delete_unary_expr(expression); return;
        case EXPR_LITERAL: ast_delete_literal_expr(expression); return;
        case EXPR_STRING_INTERP: ast_delete_string_interp_expr(expression); return;
        case EXPR_LAMBDA: ast_delete_lambda_expr(expression); return;
        case EXPR_LIST: ast_delete_list_expr(expression); return;
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

Expression* ast_new_property_expr(Expression* object, Token property, ExprContext context, bool safe)
{
    Expression* expr = raw_allocate(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_PROPERTY;
    expr->as.propertyExpr.object = object;
    expr->as.propertyExpr.property = property;
    expr->as.propertyExpr.context = context;
    expr->as.propertyExpr.safe = safe;
    return expr;
}

void ast_delete_property_expr(Expression* expression)
{
    ast_delete_expression(expression->as.propertyExpr.object);
    raw_deallocate(expression);
}

Expression* ast_new_subscript_expr(Expression* object, Expression* index, ExprContext context, bool safe)
{
    Expression* expr = raw_allocate(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_SUBSCRIPT;
    expr->as.subscriptExpr.object = object;
    expr->as.subscriptExpr.index = index;
    expr->as.subscriptExpr.context = context;
    expr->as.subscriptExpr.safe = safe;
    return expr;
}

void ast_delete_subscript_expr(Expression* expression)
{
    ast_delete_expression(expression->as.subscriptExpr.object);
    ast_delete_expression(expression->as.subscriptExpr.index);
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

Expression* ast_new_assignment_expr(Expression* target, Expression* value)
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
    ast_delete_expression(expression->as.assignmentExpr.target);
    ast_delete_expression(expression->as.assignmentExpr.value);
    raw_deallocate(expression);
}

Expression* ast_new_compound_assignment_expr(Expression* target, Token op, Expression* value)
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
    ast_delete_expression(expression->as.compoundAssignmentExpr.target);
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

Expression* ast_new_conditional_expr(Expression* condition, Expression* thenBranch, Expression* elseBranch)
{
    Expression* expr = raw_allocate(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_CONDITIONAL;
    expr->as.conditionalExpr.condition = condition;
    expr->as.conditionalExpr.thenBranch = thenBranch;
    expr->as.conditionalExpr.elseBranch = elseBranch;
    return expr;
}

void ast_delete_conditional_expr(Expression* expression)
{
    ast_delete_expression(expression->as.conditionalExpr.condition);
    ast_delete_expression(expression->as.conditionalExpr.thenBranch);
    ast_delete_expression(expression->as.conditionalExpr.elseBranch);
    raw_deallocate(expression);
}

Expression* ast_new_elvis_expr(Expression* left, Expression* right)
{
    Expression* expr = raw_allocate(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_ELVIS;
    expr->as.elvisExpr.left = left;
    expr->as.elvisExpr.right = right;
    return expr;
}

void ast_delete_elvis_expr(Expression* expression)
{
    ast_delete_expression(expression->as.elvisExpr.left);
    ast_delete_expression(expression->as.elvisExpr.right);
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

Expression* ast_new_postfix_inc_expr(Token op, Expression* expression)
{
    Expression* expr = raw_allocate(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_POSTFIX_INC;
    expr->as.postfixIncExpr.op = op;
    expr->as.postfixIncExpr.target = expression;
    return expr;
}

void ast_delete_postfix_inc_expr(Expression* expression)
{
    ast_delete_expression(expression->as.postfixIncExpr.target);
    raw_deallocate(expression);
}

Expression* ast_new_prefix_inc_expr(Token op, Expression* expression)
{
    Expression* expr = raw_allocate(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_PREFIX_INC;
    expr->as.prefixIncExpr.op = op;
    expr->as.prefixIncExpr.target = expression;
    return expr;
}

void ast_delete_prefix_inc_expr(Expression* expression)
{
    ast_delete_expression(expression->as.prefixIncExpr.target);
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

Expression* ast_new_string_interp_expr(ExpressionList* values)
{
    Expression* expr = raw_allocate(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_STRING_INTERP;
    expr->as.stringInterpExpr.values = values;
    return expr;
}

void ast_delete_string_interp_expr(Expression* expression)
{
    ast_delete_expression_list(expression->as.stringInterpExpr.values);
    raw_deallocate(expression);
}

Expression* ast_new_lambda_expr(Function* function)
{
    Expression* expr = raw_allocate(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_LAMBDA;
    expr->as.lambdaExpr.function = function;
    return expr;
}

void ast_delete_lambda_expr(Expression* expression)
{
    ast_delete_function(expression->as.lambdaExpr.function);
    raw_deallocate(expression);
}

Expression* ast_new_list_expr(ExpressionList* elements)
{
    Expression* expr = raw_allocate(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_LIST;
    expr->as.listExpr.elements = elements;
    return expr;
}

void ast_delete_list_expr(Expression* expression)
{
    ast_delete_expression_list(expression->as.listExpr.elements);
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

ExpressionList* ast_new_expression_node(Expression* expression)
{
    ExpressionList* list = raw_allocate(sizeof(ExpressionList));
    if (!list) {
        return NULL;
    }

    list->expression = expression;
    list->next = NULL;
    return list;
}

void ast_expression_list_append(ExpressionList** list, Expression* expression)
{
    if (!(*list)) {
        *list = ast_new_expression_node(expression);
        return;
    }

    ExpressionList* current = *list;
    while (current->next) {
        current = current->next;
    }

    current->next = ast_new_expression_node(expression);
}

void ast_delete_expression_list(ExpressionList* list)
{
    ExpressionList* current = list;
    while (current != NULL) {
        ExpressionList* next = current->next;
        ast_delete_expression(current->expression);
        raw_deallocate(current);
        current = next;
    }
}

size_t ast_expression_list_length(ExpressionList* list)
{
    size_t length = 0;
    ExpressionList* current = list;

    while (current) {
        length++;
        current = current->next;
    }

    return length;
}

ArgumentList* ast_new_argument_node(Expression* expression)
{
    ArgumentList* list = raw_allocate(sizeof(ArgumentList));
    if (!list) {
        return NULL;
    }

    list->expression = expression;
    list->next = NULL;
    return list;
}

void ast_argument_list_append(ArgumentList** list, Expression* expression)
{
    if (!(*list)) {
        *list = ast_new_argument_node(expression);
        return;
    }

    ArgumentList* current = *list;
    while (current->next) {
        current = current->next;
    }

    current->next = ast_new_argument_node(expression);
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

size_t ast_argument_list_length(ArgumentList* list)
{
    size_t length = 0;
    ArgumentList* current = list;

    while (current) {
        length++;
        current = current->next;
    }

    return length;
}

ParameterList* ast_new_parameter_node(Token parameter)
{
    ParameterList* list = raw_allocate(sizeof(ParameterList));
    if (!list) {
        return NULL;
    }

    list->parameter = parameter;
    list->next = NULL;
    return list;
}

void ast_parameter_list_append(ParameterList** list, Token parameter)
{
    if (!(*list)) {
        *list = ast_new_parameter_node(parameter);
        return;
    }

    ParameterList* current = *list;
    while (current->next) {
        current = current->next;
    }

    current->next = ast_new_parameter_node(parameter);
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

size_t ast_parameter_list_length(ParameterList* list)
{
    size_t length = 0;
    ParameterList* current = list;

    while (current) {
        length++;
        current = current->next;
    }

    return length;
}

WhenEntry* ast_new_when_entry(ExpressionList* cases, Statement* body)
{
    WhenEntry* entry = raw_allocate(sizeof(WhenEntry));
    if (!entry) {
        return NULL;
    }

    entry->cases = cases;
    entry->body = body;
    return entry;
}

void ast_delete_when_entry(WhenEntry* entry)
{
    ast_delete_expression_list(entry->cases);
    ast_delete_statement(entry->body);
    raw_deallocate(entry);
}

WhenEntryList* ast_new_when_entry_node(WhenEntry* entry)
{
    WhenEntryList* list = raw_allocate(sizeof(WhenEntryList));
    if (!list) {
        return NULL;
    }

    list->entry = entry;
    list->next = NULL;
    return list;
}

void ast_when_entry_list_append(WhenEntryList** list, WhenEntry* entry)
{
    if (!(*list)) {
        *list = ast_new_when_entry_node(entry);
        return;
    }

    WhenEntryList* current = *list;
    while (current->next) {
        current = current->next;
    }

    current->next = ast_new_when_entry_node(entry);
}

void ast_delete_when_entry_list(WhenEntryList* list)
{
    WhenEntryList* current = list;
    while (current != NULL) {
        WhenEntryList* next = current->next;
        ast_delete_when_entry(current->entry);
        raw_deallocate(current);
        current = next;
    }
}

size_t ast_when_entry_list_length(WhenEntryList* list)
{
    size_t length = 0;
    WhenEntryList* current = list;

    while (current) {
        length++;
        current = current->next;
    }

    return length;
}

Block* ast_new_block(DeclarationList* body)
{
    Block* block = raw_allocate(sizeof(Block));
    if (!block) {
        return NULL;
    }

    block->body = body;
    return block;
}

void ast_delete_block(Block* block)
{
    ast_delete_declaration_list(block->body);
    raw_deallocate(block);
}

void ast_delete_function_body(FunctionBody* body)
{
    switch (body->notation) {
        case FUNC_EXPRESSION: ast_delete_expression_function_body(body); return;
        case FUNC_BLOCK: ast_delete_block_function_body(body); return;
    }
}

FunctionBody* ast_new_expression_function_body(Expression* expression)
{
    FunctionBody* body = raw_allocate(sizeof(FunctionBody));
    if (!body) {
        return NULL;
    }

    body->notation = FUNC_EXPRESSION;
    body->as.expression = expression;
    return body;
}

void ast_delete_expression_function_body(FunctionBody* body)
{
    ast_delete_expression(body->as.expression);
    raw_deallocate(body);
}

FunctionBody* ast_new_block_function_body(Block* block)
{
    FunctionBody* body = raw_allocate(sizeof(FunctionBody));
    if (!body) {
        return NULL;
    }

    body->notation = FUNC_BLOCK;
    body->as.block = block;
    return body;
}

void ast_delete_block_function_body(FunctionBody* body)
{
    ast_delete_block(body->as.block);
    raw_deallocate(body);
}

Function* ast_new_function(ParameterList* parameters, FunctionBody* body)
{
    Function* function = raw_allocate(sizeof(Function));
    if (!function) {
        return NULL;
    }

    function->parameters = parameters;
    function->body = body;
    return function;
}

void ast_delete_function(Function* function)
{
    ast_delete_parameter_list(function->parameters);
    ast_delete_function_body(function->body);
    raw_deallocate(function);
}

NamedFunction* ast_new_named_function(Token identifier, Function* function)
{
    NamedFunction* namedFunction = raw_allocate(sizeof(NamedFunction));
    if (!function) {
        return NULL;
    }

    namedFunction->identifier = identifier;
    namedFunction->function = function;
    return namedFunction;
}

void ast_delete_named_function(NamedFunction* namedFunction)
{
    ast_delete_function(namedFunction->function);
    raw_deallocate(namedFunction);
}

NamedFunctionList* ast_new_named_function_node(NamedFunction* function)
{
    NamedFunctionList* list = raw_allocate(sizeof(NamedFunctionList));
    if (!list) {
        return NULL;
    }

    list->function = function;
    list->next = NULL;
    return list;
}

void ast_named_function_list_append(NamedFunctionList** list, NamedFunction* function)
{
    if (!(*list)) {
        *list = ast_new_named_function_node(function);
        return;
    }

    NamedFunctionList* current = *list;
    while (current->next) {
        current = current->next;
    }

    current->next = ast_new_named_function_node(function);
}

void ast_delete_named_function_list(NamedFunctionList* list)
{
    NamedFunctionList* current = list;
    while (current != NULL) {
        NamedFunctionList* next = current->next;
        ast_delete_named_function(current->function);
        raw_deallocate(current);
        current = next;
    }
}

size_t ast_named_function_list_length(NamedFunctionList* list)
{
    size_t length = 0;
    NamedFunctionList* current = list;

    while (current) {
        length++;
        current = current->next;
    }

    return length;
}

Method* ast_new_method(bool isStatic, NamedFunction* namedFunction)
{
    Method* method = raw_allocate(sizeof(Method));
    if (!method) {
        return NULL;
    }

    method->isStatic = isStatic;
    method->namedFunction = namedFunction;
    return method;
}

void ast_delete_method(Method* method)
{
    ast_delete_named_function(method->namedFunction);
    raw_deallocate(method);
}

MethodList* ast_new_method_node(Method* method)
{
    MethodList* list = raw_allocate(sizeof(MethodList));
    if (!list) {
        return NULL;
    }

    list->method = method;
    list->next = NULL;
    return list;
}

void ast_method_list_append(MethodList** list, Method* method)
{
    if (!(*list)) {
        *list = ast_new_method_node(method);
        return;
    }

    MethodList* current = *list;
    while (current->next) {
        current = current->next;
    }

    current->next = ast_new_method_node(method);
}

void ast_delete_method_list(MethodList* list)
{
    MethodList* current = list;
    while (current != NULL) {
        MethodList* next = current->next;
        ast_delete_method(current->method);
        raw_deallocate(current);
        current = next;
    }
}

size_t ast_method_list_length(MethodList* list)
{
    size_t length = 0;
    MethodList* current = list;

    while (current) {
        length++;
        current = current->next;
    }

    return length;
}

DeclarationList* ast_new_declaration_node(Declaration* declaration)
{
    DeclarationList* list = raw_allocate(sizeof(DeclarationList));
    if (!list) {
        return NULL;
    }

    list->declaration = declaration;
    list->next = NULL;
    return list;
}

void ast_declaration_list_append(DeclarationList** list, Declaration* declaration)
{
    if (!(*list)) {
        *list = ast_new_declaration_node(declaration);
        return;
    }

    DeclarationList* current = *list;
    while (current->next) {
        current = current->next;
    }

    current->next = ast_new_declaration_node(declaration);
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

size_t ast_declaration_list_length(DeclarationList* list)
{
    size_t length = 0;
    DeclarationList* current = list;

    while (current) {
        length++;
        current = current->next;
    }

    return length;
}
