#ifndef AST_H
#define AST_H

#include "token.h"

typedef struct Declaration Declaration;
typedef struct DeclarationList DeclarationList;

typedef struct Statement Statement;

typedef struct Expression Expression;
typedef struct ExpressionList ExpressionList;
typedef struct ArgumentList ArgumentList;

typedef struct WhenEntry WhenEntry;
typedef struct WhenEntryList WhenEntryList;

typedef struct MapEntry MapEntry;
typedef struct MapEntryList MapEntryList;

typedef struct Block Block;
typedef struct FunctionBody FunctionBody;
typedef struct ParameterList ParameterList;
typedef struct Function Function;
typedef struct NamedFunction NamedFunction;
typedef struct NamedFunctionList NamedFunctionList;
typedef struct Method Method;
typedef struct MethodList MethodList;

typedef struct VariableTarget VariableTarget;
typedef struct AssignmentTarget AssignmentTarget;
typedef enum AssignmentType { VAR_SINGLE, VAR_UNPACK } AssignmentType;

typedef enum ExprContext { LOAD, STORE } ExprContext;

typedef enum FunctionNotation { FUNC_EXPRESSION, FUNC_BLOCK } FunctionNotation;

typedef enum ImportType { IMPORT_ALL, IMPORT_AS, IMPORT_FOR } ImportType;

typedef struct AST {
    DeclarationList* body;
} AST;

typedef struct Declaration {
    enum {
        DECL_IMPORT,
        DECL_CLASS,
        DECL_FUNCTION,
        DECL_VARIABLE,
        DECL_STATEMENT
    } type;

    union {
        struct {
            Expression* moduleName;
            ImportType type;
            union {
                Token alias;
                ParameterList* names;
            } with;
        } importDecl;

        struct {
            Token identifier;
            Token superclass;
            MethodList* body;
        } classDecl;

        struct {
            NamedFunction* function;
        } functionDecl;

        struct {
            VariableTarget* target;
            Expression* value;
        } variableDecl;

        Statement* statement;
    } as;
} Declaration;

typedef struct Statement {
    enum {
        STMT_FOR,
        STMT_FOR_IN,
        STMT_WHILE,
        STMT_DO_WHILE,
        STMT_BREAK,
        STMT_CONTINUE,
        STMT_WHEN,
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
            Declaration* element;
            Expression* collection;
            Statement* body;
        } forInStmt;

        struct {
            Expression* condition;
            Statement* body;
        } whileStmt;

        struct {
            Statement* body;
            Expression* condition;
        } doWhileStmt;

        struct {
            Token keyword;
        } breakStmt;

        struct {
            Token keyword;
        } continueStmt;

        struct {
            Expression* control;
            WhenEntryList* entries;
            Statement* elseBranch;
        } whenStmt;
        
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
            Block* block;
        } blockStmt;

        Expression* expression;
    } as;
} Statement;

typedef struct Expression {
    enum {
        EXPR_CALL,
        EXPR_PROPERTY,
        EXPR_SUBSCRIPT,
        EXPR_SUPER,
        EXPR_ASSIGNMENT,
        EXPR_COMPOUND_ASSIGNMNET,
        EXPR_COROUTINE,
        EXPR_YIELD,
        EXPR_LOGICAL,
        EXPR_CONDITIONAL,
        EXPR_ELVIS,
        EXPR_BINARY,
        EXPR_UNARY,
        EXPR_PREFIX_INC,
        EXPR_POSTFIX_INC,
        EXPR_LITERAL,
        EXPR_STRING_INTERP,
        EXPR_RANGE,
        EXPR_LAMBDA,
        EXPR_LIST,
        EXPR_MAP,
        EXPR_TUPLE,
        EXPR_IDENTIFIER,
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
            bool safe;
        } propertyExpr;

        struct {
            Expression* object;
            Expression* index;
            ExprContext context;
            bool safe;
        } subscriptExpr;

        struct {
            Token keyword;
            Token method;
        } superExpr;

        struct {
            AssignmentTarget* target;
            Expression* value;
        } assignmentExpr;

        struct {
            AssignmentTarget* target;
            Token op;
            Expression* value;
        } compoundAssignmentExpr;

        struct {
            Token keyword;
            Expression* expression;
        } coroutineExpr;

        struct {
            Token keyword;
            Expression* expression;
        } yieldExpr;

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
            Expression* right;
        } elvisExpr;

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
            ExpressionList* values;
        } stringInterpExpr;

        struct {
            Expression* begin;
            Expression* end;
            Expression* step;
        } rangeExpr;

        struct {
            Function* function;
        } lambdaExpr;

        struct {
            ExpressionList* elements;
        } listExpr;

        struct {
            MapEntryList* entries;
        } mapExpr;

        struct {
            ExpressionList* elements;
        } tupleExpr;

        struct {
            Token identifier;
            ExprContext context;
        } identifierExpr;
    } as;
} Expression;

typedef struct ExpressionList {
    Expression* expression;
    ExpressionList* prev;
    ExpressionList* next;
} ExpressionList;

typedef struct ArgumentList {
    Expression* expression;
    ArgumentList* next;
} ArgumentList;

typedef struct WhenEntry {
    ExpressionList* cases;
    Statement* body;
} WhenEntry;

typedef struct WhenEntryList {
    WhenEntry* entry;
    WhenEntryList* next;
} WhenEntryList;

typedef struct MapEntry {
    Expression* key;
    Expression* value;
} MapEntry;

typedef struct MapEntryList {
    MapEntry* entry;
    MapEntryList* next;
} MapEntryList;

typedef struct Block {
    DeclarationList* body;
} Block;

typedef struct FunctionBody {
    FunctionNotation notation;
    union {
        Expression* expression;
        Block* block;
    } as;
} FunctionBody;

typedef struct ParameterList {
    Token parameter;
    ParameterList* prev;
    ParameterList* next;
} ParameterList;

typedef struct Function {
    ParameterList* parameters;
    FunctionBody* body;
} Function;

typedef struct NamedFunction {
    Token identifier;
    Function* function;
    bool coroutine;
} NamedFunction;

typedef struct NamedFunctionList {
    NamedFunction* function;
    NamedFunctionList* next;
} NamedFunctionList;

typedef struct Method {
    bool isStatic;
    NamedFunction* namedFunction;
} Method;

typedef struct MethodList {
    Method* method;
    MethodList* next;
} MethodList;

typedef struct DeclarationList {
    Declaration* declaration;
    DeclarationList* next;
} DeclarationList;

typedef struct VariableTarget {
    AssignmentType type;
    union {
        Token single;
        ParameterList* unpack;
    } as;
} VariableTarget;

typedef struct AssignmentTarget {
    AssignmentType type;
    union {
        Expression* single;
        ExpressionList* unpack;
    } as;
} AssignmentTarget;

AST* Ast_NewTree(DeclarationList* body);
void Ast_DeleteTree(AST* ast);

void Ast_DeleteDeclaration(Declaration* declaration);
Declaration* Ast_NewImportAllDecl(Expression* moduleName);
Declaration* Ast_NewImportAsDecl(Expression* moduleName, Token alias);
Declaration* Ast_NewImportForDecl(Expression* moduleName, ParameterList* names);
void Ast_DeleteImportDecl(Declaration* declaration);
Declaration* Ast_NewClassDecl(Token identifier, Token superclass, MethodList* body);
void Ast_DeleteClassDecl(Declaration* declaration);
Declaration* Ast_NewFunctionDecl(NamedFunction* function);
void Ast_DeleteFunctionDecl(Declaration* declaration);
Declaration* Ast_NewVariableDecl(VariableTarget* target, Expression* value);
void Ast_DeleteVariableDecl(Declaration* declaration);
Declaration* Ast_NewStatementDecl(Statement* statement);
void Ast_DeleteStatementDecl(Declaration* declaration);

void Ast_DeleteStatement(Statement* statement);
Statement* Ast_NewForStmt(Declaration* initializer, Expression* condition, Expression* increment, Statement* body);
void Ast_DeleteForStmt(Statement* statement);
Statement* Ast_NewForInStmt(Declaration* element, Expression* collection, Statement* body);
void Ast_DeleteForInStmt(Statement* statement);
Statement* Ast_NewWhileStmt(Expression* condition, Statement* body);
void ast_DeleteWhileStmt(Statement* statement);
Statement* Ast_NewDoWhileStmt(Statement* body, Expression* condition);
void Ast_DeleteDoWhileStmt(Statement* statement);
Statement* Ast_NewBreakStmt(Token keyword);
void Ast_DeleteBreakStmt(Statement* statement);
Statement* Ast_NewContinueStmt(Token keyword);
void Ast_DeleteContinueStmt(Statement* statement);
Statement* Ast_NewWhenStmt(Expression* control, WhenEntryList* entries, Statement* elseBranch);
void Ast_DeleteWhenStmt(Statement* statement);
Statement* Ast_NewIfStmt(Expression* condition, Statement* thenBranch, Statement* elseBranch);
void Ast_DeleteIfStmt(Statement* statement);
Statement* Ast_NewReturnStmt(Token keyword, Expression* expression);
void Ast_DeleteReturnStmt(Statement* statement);
Statement* Ast_NewPrintStmt(Expression* expression);
void Ast_DeletePrintStmt(Statement* statement);
Statement* Ast_NewBlockStmt(Block* block);
void Ast_DeleteBlockStmt(Statement* statement);
Statement* Ast_NewExpressionStmt(Expression* expression);
void Ast_DeleteExpressionStmt(Statement* statement);

void Ast_DeleteExpression(Expression* expression);
Expression* Ast_NewCallExpr(Expression* callee, ArgumentList* arguments);
void Ast_DeleteCallExpr(Expression* expression);
Expression* Ast_NewPropertyExpr(Expression* object, Token property, ExprContext context, bool safe);
void Ast_DeletePropertyExpr(Expression* expression);
Expression* Ast_NewSubscriptExpr(Expression* object, Expression* index, ExprContext context, bool safe);
void Ast_DeleteSubscriptExpr(Expression* expression);
Expression* Ast_NewSuperExpr(Token keyword, Token method);
void Ast_DeleteSuperExpr(Expression* expression);
Expression* Ast_NewAssignmentExpr(AssignmentTarget* target, Expression* value);
void Ast_DeleteAssignmentExpr(Expression* expression);
Expression* Ast_NewCompoundAssignmentExpr(AssignmentTarget* target, Token op, Expression* value);
void Ast_DeleteCompoundAssignmentExpr(Expression* expression);
Expression* Ast_NewCoroutineExpr(Token keyword, Expression* expression);
void Ast_DeleteCoroutineExpr(Expression* expression);
Expression* Ast_NewYieldExpr(Token keyword, Expression* expression);
void Ast_DeleteYieldExpr(Expression* expression);
Expression* Ast_NewLogicalExpr(Expression* left, Token op, Expression* right);
void Ast_DeleteLogicalExpr(Expression* expression);
Expression* Ast_NewConditionalExpr(Expression* condition, Expression* thenBranch, Expression* elseBranch);
void Ast_DeleteConditionalExpr(Expression* expression);
Expression* Ast_NewElvisExpr(Expression* left, Expression* right);
void Ast_DeleteElvisExpr(Expression* expression);
Expression* Ast_NewBinaryExpr(Expression* left, Token op, Expression* right);
void Ast_DeleteBinaryExpr(Expression* expression);
Expression* Ast_NewUnaryExpr(Token op, Expression* expression);
void Ast_DeleteUnaryExpr(Expression* expression);
Expression* Ast_NewPostfixIncExpr(Token op, Expression* expression);
void Ast_DeletePostifxIncExpr(Expression* expression);
Expression* Ast_NewPrefixIncExpr(Token op, Expression* expression);
void Ast_DeletePrefixIncExpr(Expression* expression);
Expression* Ast_NewLiteralExpr(Token value);
void Ast_DeleteLiteralExpr(Expression* expression);
Expression* Ast_NewStringInterpExpr(ExpressionList* values);
void Ast_DeleteStringInterpExpr(Expression* expression);
Expression* Ast_NewRangeExpr(Expression* begin, Expression* end, Expression* step);
void Ast_DeleteRangeExpr(Expression* expression);
Expression* Ast_NewLambdaExpr(Function* function);
void Ast_DeleteLambdaExpr(Expression* expression);
Expression* Ast_NewListExpr(ExpressionList* elements);
void Ast_DeleteListExpr(Expression* expression);
Expression* Ast_NewMapExpr(MapEntryList* entries);
void Ast_DeleteMapExpr(Expression* expression);
Expression* Ast_NewTupleExpr(ExpressionList* elements);
void Ast_DeleteTupleExpr(Expression* expression);
Expression* Ast_NewIdentifierExpr(Token identifier, ExprContext context);
void Ast_DeleteIdentifierExpr(Expression* expression);

ExpressionList* Ast_NewExpressionNode(Expression* expression);
void Ast_ExpressionListAppend(ExpressionList** list, Expression* expression);
void Ast_DeleteExpressionList(ExpressionList* list);
size_t Ast_ExpressionListLength(ExpressionList* list);
ExpressionList* Ast_ExpressionListEnd(ExpressionList* list);

ArgumentList* Ast_NewArgumentNode(Expression* expression);
void Ast_ArgumentListAppend(ArgumentList** list, Expression* expression);
void Ast_DeleteArgumentList(ArgumentList* list);
size_t Ast_ArgumentListLength(ArgumentList* list);

ParameterList* Ast_NewParameterNode(Token parameter);
void Ast_ParameterListAppend(ParameterList** list, Token parameter);
void Ast_DeleteParameterList(ParameterList* list);
size_t Ast_ParameterListLength(ParameterList* list);
ParameterList* Ast_ParameterListEnd(ParameterList* list);

WhenEntry* Ast_NewWhenEntry(ExpressionList* cases, Statement* body);
void Ast_DeleteWhenEntry(WhenEntry* entry);

WhenEntryList* Ast_NewWhenEntryNode(WhenEntry* entry);
void Ast_WhenEntryListAppend(WhenEntryList** list, WhenEntry* entry);
void Ast_DeleteWhenEntryList(WhenEntryList* list);
size_t Ast_WhenEntryListLength(WhenEntryList* list);

MapEntry* Ast_NewMapEntry(Expression* key, Expression* value);
void Ast_DeleteMapEntry(MapEntry* entry);

MapEntryList* Ast_NewMapEntryNode(MapEntry* entry);
void Ast_MapEntryListAppend(MapEntryList** list, MapEntry* entry);
void Ast_DeleteMapEntryList(MapEntryList* list);
size_t Ast_MapEntryListLength(MapEntryList* list);

Block* Ast_NewBlock(DeclarationList* body);
void Ast_DeleteBlock(Block* block);

void Ast_DeleteFunctionBody(FunctionBody* body);
FunctionBody* Ast_NewExpressionFunctionBody(Expression* expression);
void Ast_DeleteExpressionFunctionBody(FunctionBody* body);
FunctionBody* Ast_NewBlockFunctionBody(Block* block);
void Ast_DeleteBlockFunctionBody(FunctionBody* body);

Function* Ast_NewFunction(ParameterList* parameters, FunctionBody* body);
void Ast_DeleteFunction(Function* function);

NamedFunction* Ast_NewNamedFunction(Token identifier, Function* function, bool coroutine);
void Ast_DeleteNamedFunction(NamedFunction* function);

NamedFunctionList* Ast_NewNamedFunctionNode(NamedFunction* function);
void Ast_NamedFunctionListAppend(NamedFunctionList** list, NamedFunction* function);
void Ast_DeleteNamedFunctionList(NamedFunctionList* list);
size_t Ast_NamedFunctionListLength(NamedFunctionList* list);

Method* Ast_NewMethod(bool isStatic, NamedFunction* namedFunction);
void Ast_DeleteMethod(Method* method);

MethodList* Ast_NewMethodNode(Method* method);
void Ast_MethodListAppend(MethodList** list, Method* method);
void Ast_DeleteMethodList(MethodList* list);
size_t Ast_MethodListLength(MethodList* list);

DeclarationList* Ast_NewDeclarationNode(Declaration* declaration);
void Ast_DeclarationListAppend(DeclarationList** list, Declaration* declaration);
void Ast_DeleteDeclarationList(DeclarationList* list);
size_t Ast_DeclarationListLength(DeclarationList* list);

VariableTarget* Ast_NewSingleVariableTarget(Token single);
VariableTarget* Ast_NewUnpackVariableTarget(ParameterList* unpack);
void Ast_DeleteVariableTarget(VariableTarget* target);

AssignmentTarget* Ast_NewSingleAssignmentTarget(Expression* single);
AssignmentTarget* Ast_NewUnpackAssignmentTarget(ExpressionList* unpack);
void Ast_DeleteAssignmentTarget(AssignmentTarget* target);

#endif
