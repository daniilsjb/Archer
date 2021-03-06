#include <stdio.h>
#include <stdarg.h>

#include "astprinter.h"

static void print_tree(int indent, AST* ast);

static void print_declaration(int indent, Declaration* decl);
static void print_import_decl(int indent, Declaration* decl);
static void print_class_decl(int indent, Declaration* decl);
static void print_function_decl(int indent, Declaration* decl);
static void print_variable_decl(int indent, Declaration* decl);
static void print_statement_decl(int indent, Declaration* decl);

static void print_statement(int indent, Statement* stmt);
static void print_for_stmt(int indent, Statement* stmt);
static void print_for_in_stmt(int indent, Statement* stmt);
static void print_while_stmt(int indent, Statement* stmt);
static void print_do_while_stmt(int indent, Statement* stmt);
static void print_break_stmt(int indent, Statement* stmt);
static void print_continue_stmt(int indent, Statement* stmt);
static void print_when_stmt(int indent, Statement* stmt);
static void print_if_stmt(int indent, Statement* stmt);
static void print_return_stmt(int indent, Statement* stmt);
static void print_print_stmt(int indent, Statement* stmt);
static void print_block_stmt(int indent, Statement* stmt);
static void print_expression_stmt(int indent, Statement* stmt);

static void print_expression(int indent, Expression* expr);
static void print_call_expr(int indent, Expression* expr);
static void print_property_expr(int indent, Expression* expr);
static void print_subscript_expr(int indent, Expression* expr);
static void print_super_expr(int indent, Expression* expr);
static void print_assignment_expr(int indent, Expression* expr);
static void print_compound_assignment_expr(int indent, Expression* expr);
static void print_coroutine_expr(int indent, Expression* expr);
static void print_yield_expr(int indent, Expression* expr);
static void print_postfix_inc_expr(int indent, Expression* expr);
static void print_prefix_inc_expr(int indent, Expression* expr);
static void print_logical_expr(int indent, Expression* expr);
static void print_conditional_expr(int indent, Expression* expr);
static void print_elvis_expr(int indent, Expression* expr);
static void print_binary_expr(int indent, Expression* expr);
static void print_unary_expr(int indent, Expression* expr);
static void print_literal_expr(int indent, Expression* expr);
static void print_string_interp_expr(int indent, Expression* expr);
static void print_range_expr(int indent, Expression* expr);
static void print_lambda_expr(int indent, Expression* expr);
static void print_list_expr(int indent, Expression* expr);
static void print_map_expr(int indent, Expression* expr);
static void print_tuple_expr(int indent, Expression* expr);
static void print_identifier_expr(int indent, Expression* expr);

static void print_when_entry(int indent, WhenEntry* entry);
static void print_when_entry_list(int indent, WhenEntryList* list);
static void print_map_entry(int indent, MapEntry* entry);
static void print_map_entry_list(int indent, MapEntryList* list);
static void print_expression_list(int indent, ExpressionList* list);
static void print_argument_list(int indent, ArgumentList* list);
static void print_block(int indent, Block* block);
static void print_function_body(int indent, FunctionBody* body);
static void print_parameter_list_inline(ParameterList* list);
static void print_function(int indent, Function* function);
static void print_named_function(int indent, NamedFunction* function);
static void print_named_function_list(int indent, NamedFunctionList* list);
static void print_method(int indent, Method* method);
static void print_method_list(int indent, MethodList* list);
static void print_declaration_list(int indent, DeclarationList* list);
static void print_variable_target(int indent, VariableTarget* target);
static void print_assignment_target(int indent, AssignmentTarget* target);

static void print_indented(int indent, const char* format, ...)
{
    printf("%*.s", indent * 2, "");

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

static void print_header(int indent, const char* name)
{
    print_indented(indent, "[%s]\n", name);
}

static void print_token(Token token)
{
    printf("'%.*s'", (int)token.length, token.start);
}

static void print_token_field(int indent, const char* fieldName, Token token)
{
    print_indented(indent, "%s: ", fieldName);

    if (token.type == TOKEN_NONE) {
        printf("<None>");
    } else {
        print_token(token);
    }

    printf("\n");
}

static void print_expr_context(int indent, ExprContext context)
{
    print_indented(indent, "Context: %s\n", context == LOAD ? "Load" : "Store");
}

static void print_optional_declaration(int indent, Declaration* decl)
{
    if (!decl) {
        printf("<None>\n");
    } else {
        printf("\n");
        print_declaration(indent, decl);
    }
}

static void print_optional_statement(int indent, Statement* stmt)
{
    if (!stmt) {
        printf("<None>\n");
    } else {
        printf("\n");
        print_statement(indent, stmt);
    }
}

static void print_optional_expression(int indent, Expression* expr)
{
    if (!expr) {
        printf("<None>\n");
    } else {
        printf("\n");
        print_expression(indent, expr);
    }
}

void AstPrinter_Print(AST* ast)
{
    print_tree(0, ast);
}

void print_tree(int indent, AST* ast)
{
    print_header(indent, "Program");
    print_declaration_list(indent + 1, ast->body);
}

void print_declaration(int indent, Declaration* decl)
{
    switch (decl->type) {
        case DECL_IMPORT: print_import_decl(indent, decl); return;
        case DECL_CLASS: print_class_decl(indent, decl); return;
        case DECL_FUNCTION: print_function_decl(indent, decl); return;
        case DECL_VARIABLE: print_variable_decl(indent, decl); return;
        case DECL_STATEMENT: print_statement_decl(indent, decl); return;
    }
}

void print_import_decl(int indent, Declaration* decl)
{
    print_header(indent, "Import");
    indent++;

    print_indented(indent, "Module Name:\n");
    print_expression(indent + 1, decl->as.importDecl.moduleName);

    switch (decl->as.importDecl.type) {
        case IMPORT_ALL: {
            break;
        }
        case IMPORT_AS: {
            Token alias = decl->as.importDecl.with.alias;
            print_token_field(indent, "Alias", alias);
            break;
        }
        case IMPORT_FOR: {
            ParameterList* names = decl->as.importDecl.with.names;
            print_indented(indent, "Names: ");
            print_parameter_list_inline(names);
            printf("\n");
            break;
        }
    }
}

void print_class_decl(int indent, Declaration* decl)
{
    print_header(indent, "Class Declaration");
    indent++;

    Token identifier = decl->as.classDecl.identifier;
    print_token_field(indent, "Identifier", identifier);

    Token superclass = decl->as.classDecl.superclass;
    print_token_field(indent, "Superclass", superclass);

    print_indented(indent, "Methods:\n");
    print_method_list(indent + 1, decl->as.classDecl.body);
}

void print_function_decl(int indent, Declaration* decl)
{
    print_header(indent, "Function Declaration");
    print_named_function(indent + 1, decl->as.functionDecl.function);
}

void print_variable_decl(int indent, Declaration* decl)
{
    print_header(indent, "Variable Declaration");
    indent++;

    print_indented(indent, "Target: ");
    print_variable_target(indent, decl->as.variableDecl.target);
    printf("\n");

    print_indented(indent, "Value: ");
    Expression* value = decl->as.variableDecl.value;
    print_optional_expression(indent + 1, value);
}

void print_statement_decl(int indent, Declaration* decl)
{
    print_header(indent, "Statement");
    print_statement(indent + 1, decl->as.statement);
}

void print_statement(int indent, Statement* stmt)
{
    switch (stmt->type) {
        case STMT_FOR: print_for_stmt(indent, stmt); return;
        case STMT_FOR_IN: print_for_in_stmt(indent, stmt); return;
        case STMT_WHILE: print_while_stmt(indent, stmt); return;
        case STMT_DO_WHILE: print_do_while_stmt(indent, stmt); return;
        case STMT_BREAK: print_break_stmt(indent, stmt); return;
        case STMT_CONTINUE: print_continue_stmt(indent, stmt); return;
        case STMT_WHEN: print_when_stmt(indent, stmt); return;
        case STMT_IF: print_if_stmt(indent, stmt); return;
        case STMT_RETURN: print_return_stmt(indent, stmt); return;
        case STMT_PRINT: print_print_stmt(indent, stmt); return;
        case STMT_BLOCK: print_block_stmt(indent, stmt); return;
        case STMT_EXPRESSION: print_expression_stmt(indent, stmt); return;
    }
}

void print_for_stmt(int indent, Statement* stmt)
{
    print_header(indent, "For");
    indent++;

    Declaration* initializer = stmt->as.forStmt.initializer;
    print_indented(indent, "Initializer: ");
    print_optional_declaration(indent + 1, initializer);

    Expression* condition = stmt->as.forStmt.condition;
    print_indented(indent, "Condition: ");
    print_optional_expression(indent + 1, condition);

    Expression* increment = stmt->as.forStmt.increment;
    print_indented(indent, "Increment: ");
    print_optional_expression(indent + 1, increment);

    Statement* body = stmt->as.forStmt.body;
    print_indented(indent, "Body:\n");
    print_statement(indent + 1, body);
}

void print_for_in_stmt(int indent, Statement* stmt)
{
    print_header(indent, "For-In");
    indent++;

    Declaration* element = stmt->as.forInStmt.element;
    print_indented(indent, "Element:\n");
    print_declaration(indent + 1, element);

    Expression* collection = stmt->as.forInStmt.collection;
    print_indented(indent, "Collection:\n");
    print_expression(indent + 1, collection);

    Statement* body = stmt->as.forInStmt.body;
    print_indented(indent, "Body:\n");
    print_statement(indent + 1, body);
}

void print_while_stmt(int indent, Statement* stmt)
{
    print_header(indent, "While");
    indent++;

    Expression* condition = stmt->as.whileStmt.condition;
    print_indented(indent, "Condition:\n");
    print_expression(indent + 1, condition);

    Statement* body = stmt->as.whileStmt.body;
    print_indented(indent, "Body:\n");
    print_statement(indent + 1, body);
}

void print_do_while_stmt(int indent, Statement* stmt)
{
    print_header(indent, "Do While");
    indent++;

    Statement* body = stmt->as.doWhileStmt.body;
    print_indented(indent, "Body:\n");
    print_statement(indent + 1, body);

    Expression* condition = stmt->as.doWhileStmt.condition;
    print_indented(indent, "Condition:\n");
    print_expression(indent + 1, condition);
}

void print_break_stmt(int indent, Statement* stmt)
{
    print_header(indent, "Break");
}

void print_continue_stmt(int indent, Statement* stmt)
{
    print_header(indent, "Continue");
}

void print_when_stmt(int indent, Statement* stmt)
{
    print_header(indent, "When");
    indent++;

    Expression* control = stmt->as.whenStmt.control;
    print_indented(indent, "Control:\n");
    print_expression(indent + 1, control);

    WhenEntryList* entries = stmt->as.whenStmt.entries;
    print_indented(indent, "Entries:\n");
    print_when_entry_list(indent + 1, entries);

    Statement* elseBranch = stmt->as.whenStmt.elseBranch;
    print_indented(indent, "Else: ");
    print_optional_statement(indent + 1, elseBranch);
}

void print_if_stmt(int indent, Statement* stmt)
{
    print_header(indent, "If");
    indent++;

    Expression* condition = stmt->as.ifStmt.condition;
    print_indented(indent, "Condition:\n");
    print_expression(indent + 1, condition);

    Statement* thenBranch = stmt->as.ifStmt.thenBranch;
    print_indented(indent, "Then:\n");
    print_statement(indent + 1, thenBranch);

    Statement* elseBranch = stmt->as.ifStmt.elseBranch;
    print_indented(indent, "Else: ");
    print_optional_statement(indent + 1, elseBranch);
}

void print_return_stmt(int indent, Statement* stmt)
{
    print_header(indent, "Return");
    indent++;

    print_indented(indent, "Value: ");
    Expression* value = stmt->as.returnStmt.expression;
    print_optional_expression(indent + 1, value);
}

void print_print_stmt(int indent, Statement* stmt)
{
    print_header(indent, "Print");
    indent++;

    print_indented(indent, "Value:\n");
    Expression* expr = stmt->as.printStmt.expression;
    print_expression(indent + 1, expr);
}

void print_block_stmt(int indent, Statement* stmt)
{
    print_header(indent, "Block");
    print_block(indent + 1, stmt->as.blockStmt.block);
}

void print_expression_stmt(int indent, Statement* stmt)
{
    print_header(indent, "Expression");
    print_expression(indent + 1, stmt->as.expression);
}

void print_expression(int indent, Expression* expr)
{
    switch (expr->type) {
        case EXPR_CALL: print_call_expr(indent, expr); return;
        case EXPR_PROPERTY: print_property_expr(indent, expr); return;
        case EXPR_SUBSCRIPT: print_subscript_expr(indent, expr); return;
        case EXPR_SUPER: print_super_expr(indent, expr); return;
        case EXPR_ASSIGNMENT: print_assignment_expr(indent, expr); return;
        case EXPR_COMPOUND_ASSIGNMNET: print_compound_assignment_expr(indent, expr); return;
        case EXPR_COROUTINE: print_coroutine_expr(indent, expr); return;
        case EXPR_YIELD: print_yield_expr(indent, expr); return;
        case EXPR_POSTFIX_INC: print_postfix_inc_expr(indent, expr); return;
        case EXPR_PREFIX_INC: print_prefix_inc_expr(indent, expr); return;
        case EXPR_LOGICAL: print_logical_expr(indent, expr); return;
        case EXPR_CONDITIONAL: print_conditional_expr(indent, expr); return;
        case EXPR_ELVIS: print_elvis_expr(indent, expr); return;
        case EXPR_BINARY: print_binary_expr(indent, expr); return;
        case EXPR_UNARY: print_unary_expr(indent, expr); return;
        case EXPR_LITERAL: print_literal_expr(indent, expr); return;
        case EXPR_STRING_INTERP: print_string_interp_expr(indent, expr); return;
        case EXPR_RANGE: print_range_expr(indent, expr); return;
        case EXPR_LAMBDA: print_lambda_expr(indent, expr); return;
        case EXPR_LIST: print_list_expr(indent, expr); return;
        case EXPR_MAP: print_map_expr(indent, expr); return;
        case EXPR_TUPLE: print_tuple_expr(indent, expr); return;
        case EXPR_IDENTIFIER: print_identifier_expr(indent, expr); return;
    }
}

void print_call_expr(int indent, Expression* expr)
{
    print_header(indent, "Call");
    indent++;

    Expression* callee = expr->as.callExpr.callee;
    print_indented(indent, "Callee:\n");
    print_expression(indent + 1, callee);

    ArgumentList* arguments = expr->as.callExpr.arguments;
    print_indented(indent, "Arguments:\n");
    print_argument_list(indent + 1, arguments);
}

void print_property_expr(int indent, Expression* expr)
{
    print_header(indent, "Property");
    indent++;

    Expression* object = expr->as.propertyExpr.object;
    print_indented(indent, "Object:\n");
    print_expression(indent + 1, object);

    Token property = expr->as.propertyExpr.property;
    print_token_field(indent, "Property", property);

    ExprContext context = expr->as.propertyExpr.context;
    print_expr_context(indent, context);

    bool safe = expr->as.propertyExpr.safe;
    print_indented(indent, "Safe: %s\n", safe ? "true" : "false");
}

void print_subscript_expr(int indent, Expression* expr)
{
    print_header(indent, "Subscript");
    indent++;

    Expression* object = expr->as.subscriptExpr.object;
    print_indented(indent, "Object:\n");
    print_expression(indent + 1, object);

    Expression* index = expr->as.subscriptExpr.index;
    print_indented(indent, "Index:\n");
    print_expression(indent + 1, index);

    ExprContext context = expr->as.subscriptExpr.context;
    print_expr_context(indent, context);

    bool safe = expr->as.subscriptExpr.safe;
    print_indented(indent, "Safe: %s\n", safe ? "true" : "false");
}

void print_super_expr(int indent, Expression* expr)
{
    print_header(indent, "Super");
    indent++;

    Token method = expr->as.superExpr.method;
    print_token_field(indent, "Method", method);
}

void print_assignment_expr(int indent, Expression* expr)
{
    print_header(indent, "Assignment");
    indent++;

    print_indented(indent, "Target:\n");
    print_assignment_target(indent + 1, expr->as.assignmentExpr.target);

    Expression* value = expr->as.assignmentExpr.value;
    print_indented(indent, "Value:\n");
    print_expression(indent + 1, value);
}

void print_compound_assignment_expr(int indent, Expression* expr)
{
    print_header(indent, "Compound Assignment");
    indent++;

    print_indented(indent, "Target:\n");
    print_assignment_target(indent + 1, expr->as.compoundAssignmentExpr.target);

    Token op = expr->as.compoundAssignmentExpr.op;
    print_token_field(indent, "Operator", op);

    Expression* value = expr->as.compoundAssignmentExpr.value;
    print_indented(indent, "Value:\n");
    print_expression(indent + 1, value);
}

void print_coroutine_expr(int indent, Expression* expr)
{
    print_header(indent, "Coroutine");
    indent++;

    print_indented(indent, "Value: ");
    Expression* value = expr->as.coroutineExpr.expression;
    print_optional_expression(indent + 1, value);
}

void print_yield_expr(int indent, Expression* expr)
{
    print_header(indent, "Yield");
    indent++;

    print_indented(indent, "Value: ");
    Expression* value = expr->as.yieldExpr.expression;
    print_optional_expression(indent + 1, value);
}

void print_postfix_inc_expr(int indent, Expression* expr)
{
    print_header(indent, "Postfix Increment");
    indent++;

    Expression* target = expr->as.postfixIncExpr.target;
    print_indented(indent, "Target:\n");
    print_expression(indent + 1, target);

    Token op = expr->as.postfixIncExpr.op;
    print_token_field(indent, "Operator", op);
}

void print_prefix_inc_expr(int indent, Expression* expr)
{
    print_header(indent, "Prefix Increment");
    indent++;

    Expression* target = expr->as.prefixIncExpr.target;
    print_indented(indent, "Target:\n");
    print_expression(indent + 1, target);

    Token op = expr->as.prefixIncExpr.op;
    print_token_field(indent, "Operator", op);
}

void print_logical_expr(int indent, Expression* expr)
{
    print_header(indent, "Logical");
    indent++;

    Expression* left = expr->as.logicalExpr.left;
    print_indented(indent, "Left:\n");
    print_expression(indent + 1, left);

    Token op = expr->as.logicalExpr.op;
    print_token_field(indent, "Operator", op);

    Expression* right = expr->as.logicalExpr.right;
    print_indented(indent, "Right:\n");
    print_expression(indent + 1, right);
}

void print_conditional_expr(int indent, Expression* expr)
{
    print_header(indent, "Conditional");
    indent++;

    Expression* condition = expr->as.conditionalExpr.condition;
    print_indented(indent, "Condition:\n");
    print_expression(indent + 1, condition);

    Expression* thenBranch = expr->as.conditionalExpr.thenBranch;
    print_indented(indent, "Then:\n");
    print_expression(indent + 1, thenBranch);

    Expression* elseBranch = expr->as.conditionalExpr.elseBranch;
    print_indented(indent, "Else:\n");
    print_expression(indent + 1, elseBranch);
}

void print_elvis_expr(int indent, Expression* expr)
{
    print_header(indent, "Elvis");
    indent++;

    print_indented(indent, "Left:\n");
    print_expression(indent + 1, expr->as.elvisExpr.left);

    print_indented(indent, "Right:\n");
    print_expression(indent + 1, expr->as.elvisExpr.right);
}

void print_binary_expr(int indent, Expression* expr)
{
    print_header(indent, "Binary");
    indent++;

    Expression* left = expr->as.binaryExpr.left;
    print_indented(indent, "Left:\n");
    print_expression(indent + 1, left);

    Token op = expr->as.binaryExpr.op;
    print_token_field(indent, "Operator", op);

    Expression* right = expr->as.binaryExpr.right;
    print_indented(indent, "Right:\n");
    print_expression(indent + 1, right);
}

void print_unary_expr(int indent, Expression* expr)
{
    print_header(indent, "Unary");
    indent++;

    Token op = expr->as.unaryExpr.op;
    print_token_field(indent, "Operator", op);

    print_indented(indent, "Expression:\n");
    print_expression(indent + 1, expr->as.unaryExpr.expression);
}

void print_literal_expr(int indent, Expression* expr)
{
    print_header(indent, "Literal");
    indent++;

    Token value = expr->as.literalExpr.value;
    print_token_field(indent, "Value", value);
}

void print_string_interp_expr(int indent, Expression* expr)
{
    print_header(indent, "String Interpolation");
    indent++;

    print_indented(indent, "Values:\n");
    print_expression_list(indent + 1, expr->as.stringInterpExpr.values);
}

void print_range_expr(int indent, Expression* expr)
{
    print_header(indent, "Range");
    indent++;

    print_indented(indent, "Begin:\n");
    print_expression(indent + 1, expr->as.rangeExpr.begin);

    print_indented(indent, "End:\n");
    print_expression(indent + 1, expr->as.rangeExpr.end);

    print_indented(indent, "Step: ");
    print_optional_expression(indent + 1, expr->as.rangeExpr.step);
}

void print_lambda_expr(int indent, Expression* expr)
{
    print_header(indent, "Lambda");
    print_function(indent + 1, expr->as.lambdaExpr.function);
}

void print_list_expr(int indent, Expression* expr)
{
    print_header(indent, "List");
    indent++;

    print_indented(indent, "Elements:\n");
    print_expression_list(indent + 1, expr->as.listExpr.elements);
}

void print_map_expr(int indent, Expression* expr)
{
    print_header(indent, "Map");
    indent++;

    print_indented(indent, "Entries:\n");
    print_map_entry_list(indent + 1, expr->as.mapExpr.entries);
}

void print_tuple_expr(int indent, Expression* expr)
{
    print_header(indent, "Tuple");
    indent++;

    print_indented(indent, "Elements:\n");
    print_expression_list(indent + 1, expr->as.tupleExpr.elements);
}

void print_identifier_expr(int indent, Expression* expr)
{
    print_header(indent, "Identifier");
    indent++;

    Token identifier = expr->as.identifierExpr.identifier;
    print_token_field(indent, "Identifier", identifier);

    ExprContext context = expr->as.identifierExpr.context;
    print_expr_context(indent, context);
}

void print_when_entry(int indent, WhenEntry* entry)
{
    print_header(indent, "Entry");
    indent++;

    print_indented(indent, "Cases:\n");
    print_expression_list(indent + 1, entry->cases);

    print_indented(indent, "Body:\n");
    print_statement(indent + 1, entry->body);
}

void print_when_entry_list(int indent, WhenEntryList* list)
{
    if (Ast_WhenEntryListLength(list) == 0) {
        print_indented(indent, "<Empty>\n");
        return;
    }

    WhenEntryList* current = list;
    while (current) {

        print_when_entry(indent, current->entry);
        current = current->next;
    }
}

void print_map_entry(int indent, MapEntry* entry)
{
    print_header(indent, "Entry");
    indent++;

    print_indented(indent, "Key:\n");
    print_expression(indent + 1, entry->key);

    print_indented(indent, "Value:\n");
    print_expression(indent + 1, entry->value);
}

void print_map_entry_list(int indent, MapEntryList* list)
{
    if (Ast_MapEntryListLength(list) == 0) {
        print_indented(indent, "<Empty>\n");
        return;
    }

    MapEntryList* current = list;
    while (current) {
        print_map_entry(indent, current->entry);
        current = current->next;
    }
}

void print_expression_list(int indent, ExpressionList* list)
{
    if (Ast_ExpressionListLength(list) == 0) {
        print_indented(indent, "<Empty>\n");
        return;
    }

    ExpressionList* current = list;
    while (current) {
        print_expression(indent, current->expression);
        current = current->next;
    }
}

void print_argument_list(int indent, ArgumentList* list)
{
    if (Ast_ArgumentListLength(list) == 0) {
        print_indented(indent, "<Empty>\n");
        return;
    }

    ArgumentList* current = list;
    while (current) {
        print_expression(indent, current->expression);
        current = current->next;
    }
}

void print_block(int indent, Block* block)
{
    print_declaration_list(indent, block->body);
}

void print_function_body(int indent, FunctionBody* body)
{
    switch (body->notation) {
        case FUNC_EXPRESSION: {
            print_header(indent, "Expression");
            print_expression(indent + 1, body->as.expression);
            break;
        }
        case FUNC_BLOCK: {
            print_header(indent, "Block");
            print_block(indent + 1, body->as.block);
            break;
        }
    }
}

void print_parameter_list_inline(ParameterList* list)
{
    if (Ast_ParameterListLength(list) == 0) {
        printf("<Empty>");
        return;
    }

    ParameterList* current = list;
    while (current) {
        print_token(current->parameter);
        if (current = current->next) {
            printf(", ");
        }
    }
}

void print_function(int indent, Function* function)
{
    print_indented(indent, "Parameters: ");
    print_parameter_list_inline(function->parameters);
    printf("\n");

    print_indented(indent, "Body:\n");
    print_function_body(indent + 1, function->body);
}

void print_named_function(int indent, NamedFunction* namedFunction)
{
    print_header(indent, "Named Function");
    indent++;

    print_token_field(indent, "Identifier", namedFunction->identifier);

    print_indented(indent, "Function:\n");
    print_function(indent + 1, namedFunction->function);

    print_indented(indent, "Coroutine: %s\n", namedFunction->coroutine ? "true" : "false");
}

void print_named_function_list(int indent, NamedFunctionList* list)
{
    if (Ast_NamedFunctionListLength(list) == 0) {
        print_indented(indent, "<Empty>\n");
        return;
    }

    NamedFunctionList* current = list;
    while (current) {
        print_named_function(indent, current->function);
        current = current->next;
    }
}

void print_method(int indent, Method* method)
{
    print_header(indent, "Method");
    indent++;

    print_indented(indent, "Static: %s\n", method->isStatic ? "true" : "false");

    print_indented(indent, "Named Function:\n");
    print_named_function(indent + 1, method->namedFunction);
}

void print_method_list(int indent, MethodList* list)
{
    if (Ast_MethodListLength(list) == 0) {
        print_indented(indent, "<Empty>\n");
        return;
    }

    MethodList* current = list;
    while (current) {
        print_method(indent, current->method);
        current = current->next;
    }
}

void print_declaration_list(int indent, DeclarationList* list)
{
    if (Ast_DeclarationListLength(list) == 0) {
        print_indented(indent, "<Empty>\n");
        return;
    }

    DeclarationList* current = list;
    while (current) {
        print_declaration(indent, current->declaration);
        current = current->next;
    }
}

void print_variable_target(int indent, VariableTarget* target)
{
    switch (target->type) {
        case VAR_SINGLE: {
            print_token(target->as.single);
            break;
        }
        case VAR_UNPACK: {
            print_parameter_list_inline(target->as.unpack);
            break;
        }
    }
}

void print_assignment_target(int indent, AssignmentTarget* target)
{
    switch (target->type) {
        case VAR_SINGLE: {
            print_expression(indent, target->as.single);
            break;
        }
        case VAR_UNPACK: {
            print_expression_list(indent, target->as.unpack);
            break;
        }
    }
}
