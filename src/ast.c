#include <stdint.h>
#include <stdlib.h>

#include "ast.h"
#include "memory.h"

AST* Ast_NewTree(DeclarationList* body)
{
    AST* ast = xmalloc(sizeof(AST));
    if (!ast) {
        return NULL;
    }

    ast->body = body;
    return ast;
}

void Ast_DeleteTree(AST* ast)
{
    if (!ast) {
        return;
    }

    Ast_DeleteDeclarationList(ast->body);
    free(ast);
}

void Ast_DeleteDeclaration(Declaration* declaration)
{
    if (!declaration) {
        return;
    }

    switch (declaration->type) {
        case DECL_IMPORT: Ast_DeleteImportDecl(declaration); return;
        case DECL_CLASS: Ast_DeleteClassDecl(declaration); return;
        case DECL_FUNCTION: Ast_DeleteFunctionDecl(declaration); return;
        case DECL_VARIABLE: Ast_DeleteVariableDecl(declaration); return;
        case DECL_STATEMENT: Ast_DeleteStatementDecl(declaration); return;
    }
}

Declaration* Ast_NewImportAllDecl(Expression* moduleName)
{
    Declaration* decl = xmalloc(sizeof(Declaration));
    if (!decl) {
        return NULL;
    }

    decl->type = DECL_IMPORT;
    decl->as.importDecl.moduleName = moduleName;
    decl->as.importDecl.type = IMPORT_ALL;
    return decl;
}

Declaration* Ast_NewImportAsDecl(Expression* moduleName, Token alias)
{
    Declaration* decl = xmalloc(sizeof(Declaration));
    if (!decl) {
        return NULL;
    }

    decl->type = DECL_IMPORT;
    decl->as.importDecl.moduleName = moduleName;
    decl->as.importDecl.type = IMPORT_AS;
    decl->as.importDecl.with.alias = alias;
    return decl;
}

Declaration* Ast_NewImportForDecl(Expression* moduleName, ParameterList* names)
{
    Declaration* decl = xmalloc(sizeof(Declaration));
    if (!decl) {
        return NULL;
    }

    decl->type = DECL_IMPORT;
    decl->as.importDecl.moduleName = moduleName;
    decl->as.importDecl.type = IMPORT_FOR;
    decl->as.importDecl.with.names = names;
    return decl;
}

void Ast_DeleteImportDecl(Declaration* declaration)
{
    Ast_DeleteExpression(declaration->as.importDecl.moduleName);

    switch (declaration->as.importDecl.type) {
        case IMPORT_ALL: break;
        case IMPORT_AS: break;
        case IMPORT_FOR: Ast_DeleteParameterList(declaration->as.importDecl.with.names);
    }

    free(declaration);
}

Declaration* Ast_NewClassDecl(Token identifier, Token superclass, MethodList* body)
{
    Declaration* decl = xmalloc(sizeof(Declaration));
    if (!decl) {
        return NULL;
    }

    decl->type = DECL_CLASS;
    decl->as.classDecl.identifier = identifier;
    decl->as.classDecl.superclass = superclass;
    decl->as.classDecl.body = body;
    return decl;
}

void Ast_DeleteClassDecl(Declaration* declaration)
{
    Ast_DeleteMethodList(declaration->as.classDecl.body);
    free(declaration);
}

Declaration* Ast_NewFunctionDecl(NamedFunction* function)
{
    Declaration* decl = xmalloc(sizeof(Declaration));
    if (!decl) {
        return NULL;
    }

    decl->type = DECL_FUNCTION;
    decl->as.functionDecl.function = function;
    return decl;
}

void Ast_DeleteFunctionDecl(Declaration* declaration)
{
    Ast_DeleteNamedFunction(declaration->as.functionDecl.function);
    free(declaration);
}

Declaration* Ast_NewVariableDecl(VariableTarget* target, Expression* value)
{
    Declaration* decl = xmalloc(sizeof(Declaration));
    if (!decl) {
        return NULL;
    }

    decl->type = DECL_VARIABLE;
    decl->as.variableDecl.target = target;
    decl->as.variableDecl.value = value;
    return decl;
}

void Ast_DeleteVariableDecl(Declaration* declaration)
{
    Ast_DeleteVariableTarget(declaration->as.variableDecl.target);
    Ast_DeleteExpression(declaration->as.variableDecl.value);
    free(declaration);
}

Declaration* Ast_NewStatementDecl(Statement* statement)
{
    Declaration* decl = xmalloc(sizeof(Declaration));
    if (!decl) {
        return NULL;
    }

    decl->type = DECL_STATEMENT;
    decl->as.statement = statement;
    return decl;
}

void Ast_DeleteStatementDecl(Declaration* declaration)
{
    Ast_DeleteStatement(declaration->as.statement);
    free(declaration);
}

void Ast_DeleteStatement(Statement* statement)
{
    if (!statement) {
        return;
    }

    switch (statement->type) {
        case STMT_FOR: Ast_DeleteForStmt(statement); return;
        case STMT_FOR_IN: Ast_DeleteForInStmt(statement); return;
        case STMT_WHILE: ast_DeleteWhileStmt(statement); return;
        case STMT_DO_WHILE: Ast_DeleteDoWhileStmt(statement); return;
        case STMT_BREAK: Ast_DeleteBreakStmt(statement); return;
        case STMT_CONTINUE: Ast_DeleteContinueStmt(statement); return;
        case STMT_WHEN: Ast_DeleteWhenStmt(statement); return;
        case STMT_IF: Ast_DeleteIfStmt(statement); return;
        case STMT_RETURN: Ast_DeleteReturnStmt(statement); return;
        case STMT_PRINT: Ast_DeletePrintStmt(statement); return;
        case STMT_BLOCK: Ast_DeleteBlockStmt(statement); return;
        case STMT_EXPRESSION: Ast_DeleteExpressionStmt(statement); return;
    }
}

Statement* Ast_NewForStmt(Declaration* initializer, Expression* condition, Expression* increment, Statement* body)
{
    Statement* stmt = xmalloc(sizeof(Statement));
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

void Ast_DeleteForStmt(Statement* statement)
{
    Ast_DeleteDeclaration(statement->as.forStmt.initializer);
    Ast_DeleteExpression(statement->as.forStmt.condition);
    Ast_DeleteExpression(statement->as.forStmt.increment);
    Ast_DeleteStatement(statement->as.forStmt.body);
    free(statement);
}

Statement* Ast_NewForInStmt(Declaration* element, Expression* collection, Statement* body)
{
    Statement* stmt = xmalloc(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_FOR_IN;
    stmt->as.forInStmt.element = element;
    stmt->as.forInStmt.collection = collection;
    stmt->as.forInStmt.body = body;
    return stmt;
}

void Ast_DeleteForInStmt(Statement* statement)
{
    Ast_DeleteDeclaration(statement->as.forInStmt.element);
    Ast_DeleteExpression(statement->as.forInStmt.collection);
    Ast_DeleteStatement(statement->as.forInStmt.body);
    free(statement);
}

Statement* Ast_NewWhileStmt(Expression* condition, Statement* body)
{
    Statement* stmt = xmalloc(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_WHILE;
    stmt->as.whileStmt.condition = condition;
    stmt->as.whileStmt.body = body;
    return stmt;
}

void ast_DeleteWhileStmt(Statement* statement)
{
    Ast_DeleteExpression(statement->as.whileStmt.condition);
    Ast_DeleteStatement(statement->as.whileStmt.body);
    free(statement);
}

Statement* Ast_NewDoWhileStmt(Statement* body, Expression* condition)
{
    Statement* stmt = xmalloc(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_DO_WHILE;
    stmt->as.doWhileStmt.body = body;
    stmt->as.doWhileStmt.condition = condition;
    return stmt;
}

void Ast_DeleteDoWhileStmt(Statement* statement)
{
    Ast_DeleteStatement(statement->as.doWhileStmt.body);
    Ast_DeleteExpression(statement->as.doWhileStmt.condition);
    free(statement);
}

Statement* Ast_NewBreakStmt(Token keyword)
{
    Statement* stmt = xmalloc(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_BREAK;
    stmt->as.breakStmt.keyword = keyword;
    return stmt;
}

void Ast_DeleteBreakStmt(Statement* statement)
{
    free(statement);
}

Statement* Ast_NewContinueStmt(Token keyword)
{
    Statement* stmt = xmalloc(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_CONTINUE;
    stmt->as.continueStmt.keyword = keyword;
    return stmt;
}

void Ast_DeleteContinueStmt(Statement* statement)
{
    free(statement);
}

Statement* Ast_NewWhenStmt(Expression* control, WhenEntryList* entries, Statement* elseBranch)
{
    Statement* stmt = xmalloc(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_WHEN;
    stmt->as.whenStmt.control = control;
    stmt->as.whenStmt.entries = entries;
    stmt->as.whenStmt.elseBranch = elseBranch;
    return stmt;
}

void Ast_DeleteWhenStmt(Statement* statement)
{
    Ast_DeleteExpression(statement->as.whenStmt.control);
    Ast_DeleteWhenEntryList(statement->as.whenStmt.entries);
    Ast_DeleteStatement(statement->as.whenStmt.elseBranch);
    free(statement);
}

Statement* Ast_NewIfStmt(Expression* condition, Statement* thenBranch, Statement* elseBranch)
{
    Statement* stmt = xmalloc(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_IF;
    stmt->as.ifStmt.condition = condition;
    stmt->as.ifStmt.thenBranch = thenBranch;
    stmt->as.ifStmt.elseBranch = elseBranch;
    return stmt;
}

void Ast_DeleteIfStmt(Statement* statement)
{
    Ast_DeleteExpression(statement->as.ifStmt.condition);
    Ast_DeleteStatement(statement->as.ifStmt.thenBranch);
    Ast_DeleteStatement(statement->as.ifStmt.elseBranch);
    free(statement);
}

Statement* Ast_NewReturnStmt(Token keyword, Expression* expression)
{
    Statement* stmt = xmalloc(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_RETURN;
    stmt->as.returnStmt.keyword = keyword;
    stmt->as.returnStmt.expression = expression;
    return stmt;
}

void Ast_DeleteReturnStmt(Statement* statement)
{
    Ast_DeleteExpression(statement->as.returnStmt.expression);
    free(statement);
}

Statement* Ast_NewPrintStmt(Expression* expression)
{
    Statement* stmt = xmalloc(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_PRINT;
    stmt->as.printStmt.expression = expression;
    return stmt;
}

void Ast_DeletePrintStmt(Statement* statement)
{
    Ast_DeleteExpression(statement->as.printStmt.expression);
    free(statement);
}

Statement* Ast_NewBlockStmt(Block* block)
{
    Statement* stmt = xmalloc(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_BLOCK;
    stmt->as.blockStmt.block = block;
    return stmt;
}

void Ast_DeleteBlockStmt(Statement* statement)
{
    Ast_DeleteBlock(statement->as.blockStmt.block);
    free(statement);
}

Statement* Ast_NewExpressionStmt(Expression* expression)
{
    Statement* stmt = xmalloc(sizeof(Statement));
    if (!stmt) {
        return NULL;
    }

    stmt->type = STMT_EXPRESSION;
    stmt->as.expression = expression;
    return stmt;
}

void Ast_DeleteExpressionStmt(Statement* statement)
{
    Ast_DeleteExpression(statement->as.expression);
    free(statement);
}

void Ast_DeleteExpression(Expression* expression)
{
    if (!expression) {
        return;
    }

    switch (expression->type) {
        case EXPR_CALL: Ast_DeleteCallExpr(expression); return;
        case EXPR_PROPERTY: Ast_DeletePropertyExpr(expression); return;
        case EXPR_SUBSCRIPT: Ast_DeleteSubscriptExpr(expression); return;
        case EXPR_SUPER: Ast_DeleteSuperExpr(expression); return;
        case EXPR_ASSIGNMENT: Ast_DeleteAssignmentExpr(expression); return;
        case EXPR_COMPOUND_ASSIGNMNET: Ast_DeleteCompoundAssignmentExpr(expression); return;
        case EXPR_COROUTINE: Ast_DeleteCoroutineExpr(expression); return;
        case EXPR_YIELD: Ast_DeleteYieldExpr(expression); return;
        case EXPR_POSTFIX_INC: Ast_DeletePostifxIncExpr(expression); return;
        case EXPR_PREFIX_INC: Ast_DeletePrefixIncExpr(expression); return;
        case EXPR_LOGICAL: Ast_DeleteLogicalExpr(expression); return;
        case EXPR_CONDITIONAL: Ast_DeleteConditionalExpr(expression); return;
        case EXPR_ELVIS: Ast_DeleteElvisExpr(expression); return;
        case EXPR_BINARY: Ast_DeleteBinaryExpr(expression); return;
        case EXPR_UNARY: Ast_DeleteUnaryExpr(expression); return;
        case EXPR_LITERAL: Ast_DeleteLiteralExpr(expression); return;
        case EXPR_STRING_INTERP: Ast_DeleteStringInterpExpr(expression); return;
        case EXPR_RANGE: Ast_DeleteRangeExpr(expression); return;
        case EXPR_LAMBDA: Ast_DeleteLambdaExpr(expression); return;
        case EXPR_LIST: Ast_DeleteListExpr(expression); return;
        case EXPR_MAP: Ast_DeleteMapExpr(expression); return;
        case EXPR_TUPLE: Ast_DeleteTupleExpr(expression); return;
        case EXPR_IDENTIFIER: Ast_DeleteIdentifierExpr(expression); return;
    }
}

Expression* Ast_NewCallExpr(Expression* callee, ArgumentList* arguments)
{
    Expression* expr = xmalloc(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_CALL;
    expr->as.callExpr.callee = callee;
    expr->as.callExpr.arguments = arguments;
    return expr;
}

void Ast_DeleteCallExpr(Expression* expression)
{
    Ast_DeleteExpression(expression->as.callExpr.callee);
    Ast_DeleteArgumentList(expression->as.callExpr.arguments);
    free(expression);
}

Expression* Ast_NewPropertyExpr(Expression* object, Token property, ExprContext context, bool safe)
{
    Expression* expr = xmalloc(sizeof(Expression));
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

void Ast_DeletePropertyExpr(Expression* expression)
{
    Ast_DeleteExpression(expression->as.propertyExpr.object);
    free(expression);
}

Expression* Ast_NewSubscriptExpr(Expression* object, Expression* index, ExprContext context, bool safe)
{
    Expression* expr = xmalloc(sizeof(Expression));
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

void Ast_DeleteSubscriptExpr(Expression* expression)
{
    Ast_DeleteExpression(expression->as.subscriptExpr.object);
    Ast_DeleteExpression(expression->as.subscriptExpr.index);
    free(expression);
}

Expression* Ast_NewSuperExpr(Token keyword, Token method)
{
    Expression* expr = xmalloc(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_SUPER;
    expr->as.superExpr.keyword = keyword;
    expr->as.superExpr.method = method;
    return expr;
}

void Ast_DeleteSuperExpr(Expression* expression)
{
    free(expression);
}

Expression* Ast_NewAssignmentExpr(AssignmentTarget* target, Expression* value)
{
    Expression* expr = xmalloc(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_ASSIGNMENT;
    expr->as.assignmentExpr.target = target;
    expr->as.assignmentExpr.value = value;
    return expr;
}

void Ast_DeleteAssignmentExpr(Expression* expression)
{
    Ast_DeleteAssignmentTarget(expression->as.assignmentExpr.target);
    Ast_DeleteExpression(expression->as.assignmentExpr.value);
    free(expression);
}

Expression* Ast_NewCompoundAssignmentExpr(AssignmentTarget* target, Token op, Expression* value)
{
    Expression* expr = xmalloc(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_COMPOUND_ASSIGNMNET;
    expr->as.compoundAssignmentExpr.target = target;
    expr->as.compoundAssignmentExpr.op = op;
    expr->as.compoundAssignmentExpr.value = value;
    return expr;
}

void Ast_DeleteCompoundAssignmentExpr(Expression* expression)
{
    Ast_DeleteAssignmentTarget(expression->as.compoundAssignmentExpr.target);
    Ast_DeleteExpression(expression->as.compoundAssignmentExpr.value);
    free(expression);
}

Expression* Ast_NewCoroutineExpr(Token keyword, Expression* expression)
{
    Expression* expr = xmalloc(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_COROUTINE;
    expr->as.coroutineExpr.keyword = keyword;
    expr->as.coroutineExpr.expression = expression;
    return expr;
}

void Ast_DeleteCoroutineExpr(Expression* expression)
{
    Ast_DeleteExpression(expression->as.coroutineExpr.expression);
    free(expression);
}

Expression* Ast_NewYieldExpr(Token keyword, Expression* expression)
{
    Expression* expr = xmalloc(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_YIELD;
    expr->as.yieldExpr.keyword = keyword;
    expr->as.yieldExpr.expression = expression;
    return expr;
}

void Ast_DeleteYieldExpr(Expression* expression)
{
    Ast_DeleteExpression(expression->as.yieldExpr.expression);
    free(expression);
}

Expression* Ast_NewLogicalExpr(Expression* left, Token op, Expression* right)
{
    Expression* expr = xmalloc(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_LOGICAL;
    expr->as.logicalExpr.left = left;
    expr->as.logicalExpr.op = op;
    expr->as.logicalExpr.right = right;
    return expr;
}

void Ast_DeleteLogicalExpr(Expression* expression)
{
    Ast_DeleteExpression(expression->as.logicalExpr.left);
    Ast_DeleteExpression(expression->as.logicalExpr.right);
    free(expression);
}

Expression* Ast_NewConditionalExpr(Expression* condition, Expression* thenBranch, Expression* elseBranch)
{
    Expression* expr = xmalloc(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_CONDITIONAL;
    expr->as.conditionalExpr.condition = condition;
    expr->as.conditionalExpr.thenBranch = thenBranch;
    expr->as.conditionalExpr.elseBranch = elseBranch;
    return expr;
}

void Ast_DeleteConditionalExpr(Expression* expression)
{
    Ast_DeleteExpression(expression->as.conditionalExpr.condition);
    Ast_DeleteExpression(expression->as.conditionalExpr.thenBranch);
    Ast_DeleteExpression(expression->as.conditionalExpr.elseBranch);
    free(expression);
}

Expression* Ast_NewElvisExpr(Expression* left, Expression* right)
{
    Expression* expr = xmalloc(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_ELVIS;
    expr->as.elvisExpr.left = left;
    expr->as.elvisExpr.right = right;
    return expr;
}

void Ast_DeleteElvisExpr(Expression* expression)
{
    Ast_DeleteExpression(expression->as.elvisExpr.left);
    Ast_DeleteExpression(expression->as.elvisExpr.right);
    free(expression);
}

Expression* Ast_NewBinaryExpr(Expression* left, Token op, Expression* right)
{
    Expression* expr = xmalloc(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_BINARY;
    expr->as.logicalExpr.left = left;
    expr->as.logicalExpr.op = op;
    expr->as.logicalExpr.right = right;
    return expr;
}

void Ast_DeleteBinaryExpr(Expression* expression)
{
    Ast_DeleteExpression(expression->as.binaryExpr.left);
    Ast_DeleteExpression(expression->as.binaryExpr.right);
    free(expression);
}

Expression* Ast_NewUnaryExpr(Token op, Expression* expression)
{
    Expression* expr = xmalloc(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_UNARY;
    expr->as.unaryExpr.op = op;
    expr->as.unaryExpr.expression = expression;
    return expr;
}

void Ast_DeleteUnaryExpr(Expression* expression)
{
    Ast_DeleteExpression(expression->as.unaryExpr.expression);
    free(expression);
}

Expression* Ast_NewPostfixIncExpr(Token op, Expression* expression)
{
    Expression* expr = xmalloc(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_POSTFIX_INC;
    expr->as.postfixIncExpr.op = op;
    expr->as.postfixIncExpr.target = expression;
    return expr;
}

void Ast_DeletePostifxIncExpr(Expression* expression)
{
    Ast_DeleteExpression(expression->as.postfixIncExpr.target);
    free(expression);
}

Expression* Ast_NewPrefixIncExpr(Token op, Expression* expression)
{
    Expression* expr = xmalloc(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_PREFIX_INC;
    expr->as.prefixIncExpr.op = op;
    expr->as.prefixIncExpr.target = expression;
    return expr;
}

void Ast_DeletePrefixIncExpr(Expression* expression)
{
    Ast_DeleteExpression(expression->as.prefixIncExpr.target);
    free(expression);
}

Expression* Ast_NewLiteralExpr(Token value)
{
    Expression* expr = xmalloc(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_LITERAL;
    expr->as.literalExpr.value = value;
    return expr;
}

void Ast_DeleteLiteralExpr(Expression* expression)
{
    free(expression);
}

Expression* Ast_NewStringInterpExpr(ExpressionList* values)
{
    Expression* expr = xmalloc(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_STRING_INTERP;
    expr->as.stringInterpExpr.values = values;
    return expr;
}

void Ast_DeleteStringInterpExpr(Expression* expression)
{
    Ast_DeleteExpressionList(expression->as.stringInterpExpr.values);
    free(expression);
}

Expression* Ast_NewRangeExpr(Expression* begin, Expression* end, Expression* step)
{
    Expression* expr = xmalloc(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_RANGE;
    expr->as.rangeExpr.begin = begin;
    expr->as.rangeExpr.end = end;
    expr->as.rangeExpr.step = step;
    return expr;
}

void Ast_DeleteRangeExpr(Expression* expression)
{
    Ast_DeleteExpression(expression->as.rangeExpr.begin);
    Ast_DeleteExpression(expression->as.rangeExpr.end);
    Ast_DeleteExpression(expression->as.rangeExpr.step);
    free(expression);
}

Expression* Ast_NewLambdaExpr(Function* function)
{
    Expression* expr = xmalloc(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_LAMBDA;
    expr->as.lambdaExpr.function = function;
    return expr;
}

void Ast_DeleteLambdaExpr(Expression* expression)
{
    Ast_DeleteFunction(expression->as.lambdaExpr.function);
    free(expression);
}

Expression* Ast_NewListExpr(ExpressionList* elements)
{
    Expression* expr = xmalloc(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_LIST;
    expr->as.listExpr.elements = elements;
    return expr;
}

void Ast_DeleteListExpr(Expression* expression)
{
    Ast_DeleteExpressionList(expression->as.listExpr.elements);
    free(expression);
}

Expression* Ast_NewMapExpr(MapEntryList* entries)
{
    Expression* expr = xmalloc(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_MAP;
    expr->as.mapExpr.entries = entries;
    return expr;
}

void Ast_DeleteMapExpr(Expression* expression)
{
    Ast_DeleteMapEntryList(expression->as.mapExpr.entries);
    free(expression);
}

Expression* Ast_NewTupleExpr(ExpressionList* elements)
{
    Expression* expr = xmalloc(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_TUPLE;
    expr->as.tupleExpr.elements = elements;
    return expr;
}

void Ast_DeleteTupleExpr(Expression* expression)
{
    Ast_DeleteExpressionList(expression->as.tupleExpr.elements);
    free(expression);
}

Expression* Ast_NewIdentifierExpr(Token identifier, ExprContext context)
{
    Expression* expr = xmalloc(sizeof(Expression));
    if (!expr) {
        return NULL;
    }

    expr->type = EXPR_IDENTIFIER;
    expr->as.identifierExpr.identifier = identifier;
    expr->as.identifierExpr.context = context;
    return expr;
}

void Ast_DeleteIdentifierExpr(Expression* expression)
{
    free(expression);
}

ExpressionList* Ast_NewExpressionNode(Expression* expression)
{
    ExpressionList* list = xmalloc(sizeof(ExpressionList));
    if (!list) {
        return NULL;
    }

    list->expression = expression;
    list->prev = NULL;
    list->next = NULL;
    return list;
}

void Ast_ExpressionListAppend(ExpressionList** list, Expression* expression)
{
    if (!(*list)) {
        *list = Ast_NewExpressionNode(expression);
        return;
    }

    ExpressionList* current = *list;
    while (current->next) {
        current = current->next;
    }

    current->next = Ast_NewExpressionNode(expression);
    current->next->prev = current;
}

void Ast_DeleteExpressionList(ExpressionList* list)
{
    ExpressionList* current = list;
    while (current != NULL) {
        ExpressionList* next = current->next;
        Ast_DeleteExpression(current->expression);
        free(current);
        current = next;
    }
}

size_t Ast_ExpressionListLength(ExpressionList* list)
{
    size_t length = 0;
    ExpressionList* current = list;

    while (current) {
        length++;
        current = current->next;
    }

    return length;
}

ExpressionList* Ast_ExpressionListEnd(ExpressionList* list)
{
    if (list == NULL) {
        return NULL;
    }

    for (ExpressionList* current = list;; current = current->next) {
        if (current->next == NULL) {
            return current;
        }
    }
}

ArgumentList* Ast_NewArgumentNode(Expression* expression)
{
    ArgumentList* list = xmalloc(sizeof(ArgumentList));
    if (!list) {
        return NULL;
    }

    list->expression = expression;
    list->next = NULL;
    return list;
}

void Ast_ArgumentListAppend(ArgumentList** list, Expression* expression)
{
    if (!(*list)) {
        *list = Ast_NewArgumentNode(expression);
        return;
    }

    ArgumentList* current = *list;
    while (current->next) {
        current = current->next;
    }

    current->next = Ast_NewArgumentNode(expression);
}

void Ast_DeleteArgumentList(ArgumentList* list)
{
    ArgumentList* current = list;
    while (current != NULL) {
        ArgumentList* next = current->next;
        Ast_DeleteExpression(current->expression);
        free(current);
        current = next;
    }
}

size_t Ast_ArgumentListLength(ArgumentList* list)
{
    size_t length = 0;
    ArgumentList* current = list;

    while (current) {
        length++;
        current = current->next;
    }

    return length;
}

ParameterList* Ast_NewParameterNode(Token parameter)
{
    ParameterList* list = xmalloc(sizeof(ParameterList));
    if (!list) {
        return NULL;
    }

    list->parameter = parameter;
    list->prev = NULL;
    list->next = NULL;
    return list;
}

void Ast_ParameterListAppend(ParameterList** list, Token parameter)
{
    if (!(*list)) {
        *list = Ast_NewParameterNode(parameter);
        return;
    }

    ParameterList* current = *list;
    while (current->next) {
        current = current->next;
    }

    current->next = Ast_NewParameterNode(parameter);
    current->next->prev = current;
}

void Ast_DeleteParameterList(ParameterList* list)
{
    ParameterList* current = list;
    while (current != NULL) {
        ParameterList* next = current->next;
        free(current);
        current = next;
    }
}

size_t Ast_ParameterListLength(ParameterList* list)
{
    size_t length = 0;
    ParameterList* current = list;

    while (current) {
        length++;
        current = current->next;
    }

    return length;
}

ParameterList* Ast_ParameterListEnd(ParameterList* list)
{
    if (list == NULL) {
        return NULL;
    }

    for (ParameterList* current = list;; current = current->next) {
        if (current->next == NULL) {
            return current;
        }
    }
}

WhenEntry* Ast_NewWhenEntry(ExpressionList* cases, Statement* body)
{
    WhenEntry* entry = xmalloc(sizeof(WhenEntry));
    if (!entry) {
        return NULL;
    }

    entry->cases = cases;
    entry->body = body;
    return entry;
}

void Ast_DeleteWhenEntry(WhenEntry* entry)
{
    Ast_DeleteExpressionList(entry->cases);
    Ast_DeleteStatement(entry->body);
    free(entry);
}

WhenEntryList* Ast_NewWhenEntryNode(WhenEntry* entry)
{
    WhenEntryList* list = xmalloc(sizeof(WhenEntryList));
    if (!list) {
        return NULL;
    }

    list->entry = entry;
    list->next = NULL;
    return list;
}

void Ast_WhenEntryListAppend(WhenEntryList** list, WhenEntry* entry)
{
    if (!(*list)) {
        *list = Ast_NewWhenEntryNode(entry);
        return;
    }

    WhenEntryList* current = *list;
    while (current->next) {
        current = current->next;
    }

    current->next = Ast_NewWhenEntryNode(entry);
}

void Ast_DeleteWhenEntryList(WhenEntryList* list)
{
    WhenEntryList* current = list;
    while (current != NULL) {
        WhenEntryList* next = current->next;
        Ast_DeleteWhenEntry(current->entry);
        free(current);
        current = next;
    }
}

size_t Ast_WhenEntryListLength(WhenEntryList* list)
{
    size_t length = 0;
    WhenEntryList* current = list;

    while (current) {
        length++;
        current = current->next;
    }

    return length;
}

MapEntry* Ast_NewMapEntry(Expression* key, Expression* value)
{
    MapEntry* entry = xmalloc(sizeof(MapEntry));
    if (!entry) {
        return NULL;
    }

    entry->key = key;
    entry->value = value;
    return entry;
}

void Ast_DeleteMapEntry(MapEntry* entry)
{
    Ast_DeleteExpression(entry->key);
    Ast_DeleteExpression(entry->value);
    free(entry);
}

MapEntryList* Ast_NewMapEntryNode(MapEntry* entry)
{
    MapEntryList* list = xmalloc(sizeof(MapEntryList));
    if (!list) {
        return NULL;
    }

    list->entry = entry;
    list->next = NULL;
    return list;
}

void Ast_MapEntryListAppend(MapEntryList** list, MapEntry* entry)
{
    if (!(*list)) {
        *list = Ast_NewMapEntryNode(entry);
        return;
    }

    MapEntryList* current = *list;
    while (current->next) {
        current = current->next;
    }

    current->next = Ast_NewMapEntryNode(entry);
}

void Ast_DeleteMapEntryList(MapEntryList* list)
{
    MapEntryList* current = list;
    while (current != NULL) {
        MapEntryList* next = current->next;
        Ast_DeleteMapEntry(current->entry);
        free(current);
        current = next;
    }
}

size_t Ast_MapEntryListLength(MapEntryList* list)
{
    size_t length = 0;
    MapEntryList* current = list;

    while (current) {
        length++;
        current = current->next;
    }

    return length;
}

Block* Ast_NewBlock(DeclarationList* body)
{
    Block* block = xmalloc(sizeof(Block));
    if (!block) {
        return NULL;
    }

    block->body = body;
    return block;
}

void Ast_DeleteBlock(Block* block)
{
    Ast_DeleteDeclarationList(block->body);
    free(block);
}

void Ast_DeleteFunctionBody(FunctionBody* body)
{
    switch (body->notation) {
        case FUNC_EXPRESSION: Ast_DeleteExpressionFunctionBody(body); return;
        case FUNC_BLOCK: Ast_DeleteBlockFunctionBody(body); return;
    }
}

FunctionBody* Ast_NewExpressionFunctionBody(Expression* expression)
{
    FunctionBody* body = xmalloc(sizeof(FunctionBody));
    if (!body) {
        return NULL;
    }

    body->notation = FUNC_EXPRESSION;
    body->as.expression = expression;
    return body;
}

void Ast_DeleteExpressionFunctionBody(FunctionBody* body)
{
    Ast_DeleteExpression(body->as.expression);
    free(body);
}

FunctionBody* Ast_NewBlockFunctionBody(Block* block)
{
    FunctionBody* body = xmalloc(sizeof(FunctionBody));
    if (!body) {
        return NULL;
    }

    body->notation = FUNC_BLOCK;
    body->as.block = block;
    return body;
}

void Ast_DeleteBlockFunctionBody(FunctionBody* body)
{
    Ast_DeleteBlock(body->as.block);
    free(body);
}

Function* Ast_NewFunction(ParameterList* parameters, FunctionBody* body)
{
    Function* function = xmalloc(sizeof(Function));
    if (!function) {
        return NULL;
    }

    function->parameters = parameters;
    function->body = body;
    return function;
}

void Ast_DeleteFunction(Function* function)
{
    Ast_DeleteParameterList(function->parameters);
    Ast_DeleteFunctionBody(function->body);
    free(function);
}

NamedFunction* Ast_NewNamedFunction(Token identifier, Function* function, bool coroutine)
{
    NamedFunction* namedFunction = xmalloc(sizeof(NamedFunction));
    if (!function) {
        return NULL;
    }

    namedFunction->identifier = identifier;
    namedFunction->function = function;
    namedFunction->coroutine = coroutine;
    return namedFunction;
}

void Ast_DeleteNamedFunction(NamedFunction* namedFunction)
{
    Ast_DeleteFunction(namedFunction->function);
    free(namedFunction);
}

NamedFunctionList* Ast_NewNamedFunctionNode(NamedFunction* function)
{
    NamedFunctionList* list = xmalloc(sizeof(NamedFunctionList));
    if (!list) {
        return NULL;
    }

    list->function = function;
    list->next = NULL;
    return list;
}

void Ast_NamedFunctionListAppend(NamedFunctionList** list, NamedFunction* function)
{
    if (!(*list)) {
        *list = Ast_NewNamedFunctionNode(function);
        return;
    }

    NamedFunctionList* current = *list;
    while (current->next) {
        current = current->next;
    }

    current->next = Ast_NewNamedFunctionNode(function);
}

void Ast_DeleteNamedFunctionList(NamedFunctionList* list)
{
    NamedFunctionList* current = list;
    while (current != NULL) {
        NamedFunctionList* next = current->next;
        Ast_DeleteNamedFunction(current->function);
        free(current);
        current = next;
    }
}

size_t Ast_NamedFunctionListLength(NamedFunctionList* list)
{
    size_t length = 0;
    NamedFunctionList* current = list;

    while (current) {
        length++;
        current = current->next;
    }

    return length;
}

Method* Ast_NewMethod(bool isStatic, NamedFunction* namedFunction)
{
    Method* method = xmalloc(sizeof(Method));
    if (!method) {
        return NULL;
    }

    method->isStatic = isStatic;
    method->namedFunction = namedFunction;
    return method;
}

void Ast_DeleteMethod(Method* method)
{
    Ast_DeleteNamedFunction(method->namedFunction);
    free(method);
}

MethodList* Ast_NewMethodNode(Method* method)
{
    MethodList* list = xmalloc(sizeof(MethodList));
    if (!list) {
        return NULL;
    }

    list->method = method;
    list->next = NULL;
    return list;
}

void Ast_MethodListAppend(MethodList** list, Method* method)
{
    if (!(*list)) {
        *list = Ast_NewMethodNode(method);
        return;
    }

    MethodList* current = *list;
    while (current->next) {
        current = current->next;
    }

    current->next = Ast_NewMethodNode(method);
}

void Ast_DeleteMethodList(MethodList* list)
{
    MethodList* current = list;
    while (current != NULL) {
        MethodList* next = current->next;
        Ast_DeleteMethod(current->method);
        free(current);
        current = next;
    }
}

size_t Ast_MethodListLength(MethodList* list)
{
    size_t length = 0;
    MethodList* current = list;

    while (current) {
        length++;
        current = current->next;
    }

    return length;
}

DeclarationList* Ast_NewDeclarationNode(Declaration* declaration)
{
    DeclarationList* list = xmalloc(sizeof(DeclarationList));
    if (!list) {
        return NULL;
    }

    list->declaration = declaration;
    list->next = NULL;
    return list;
}

void Ast_DeclarationListAppend(DeclarationList** list, Declaration* declaration)
{
    if (!(*list)) {
        *list = Ast_NewDeclarationNode(declaration);
        return;
    }

    DeclarationList* current = *list;
    while (current->next) {
        current = current->next;
    }

    current->next = Ast_NewDeclarationNode(declaration);
}

void Ast_DeleteDeclarationList(DeclarationList* list)
{
    DeclarationList* current = list;
    while (current != NULL) {
        DeclarationList* next = current->next;
        Ast_DeleteDeclaration(current->declaration);
        free(current);
        current = next;
    }
}

size_t Ast_DeclarationListLength(DeclarationList* list)
{
    size_t length = 0;
    DeclarationList* current = list;

    while (current) {
        length++;
        current = current->next;
    }

    return length;
}

VariableTarget* Ast_NewSingleVariableTarget(Token single)
{
    VariableTarget* target = xmalloc(sizeof(VariableTarget));
    if (!target) {
        return NULL;
    }

    target->type = VAR_SINGLE;
    target->as.single = single;
    return target;
}

VariableTarget* Ast_NewUnpackVariableTarget(ParameterList* unpack)
{
    VariableTarget* target = xmalloc(sizeof(VariableTarget));
    if (!target) {
        return NULL;
    }

    target->type = VAR_UNPACK;
    target->as.unpack = unpack;
    return target;
}

void Ast_DeleteVariableTarget(VariableTarget* target)
{
    switch (target->type) {
        case VAR_SINGLE: break;
        case VAR_UNPACK: Ast_DeleteParameterList(target->as.unpack); break;
    }

    free(target);
}

AssignmentTarget* Ast_NewSingleAssignmentTarget(Expression* single)
{
    AssignmentTarget* target = xmalloc(sizeof(AssignmentTarget));
    if (!target) {
        return NULL;
    }

    target->type = VAR_SINGLE;
    target->as.single = single;
    return target;
}

AssignmentTarget* Ast_NewUnpackAssignmentTarget(ExpressionList* unpack)
{
    AssignmentTarget* target = xmalloc(sizeof(AssignmentTarget));
    if (!target) {
        return NULL;
    }

    target->type = VAR_UNPACK;
    target->as.unpack = unpack;
    return target;
}

void Ast_DeleteAssignmentTarget(AssignmentTarget* target)
{
    switch (target->type) {
        case VAR_SINGLE: break;
        case VAR_UNPACK: Ast_DeleteExpressionList(target->as.unpack); break;
    }

    free(target);
}
