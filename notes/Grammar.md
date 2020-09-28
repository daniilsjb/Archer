# Grammar

## Description

### Notation

The notation used is a modified version of Extended Backus-Naur Form. It uses syntax that is more resemblant to regular expressions for brevity and clarity.

### Operators

Operators and their meanings:

- operator `|` denotes *alternative* (only a single rule must match)
- operator `~` denotes *negation* (matches anything other than the rule)
- operator `?` denotes *option* (zero or one match)
- operator `*` denotes *iteration* (zero or more matches)
- operator `+` denotes *iteration* (one or more matches)
- operator `()` denotes *grouping* (a sequence of symbols is treated as a single rule)
- operator `..` denotes *range* (match anything between two iterative values)
- operator `→` denotes *binding* (rule is bound to a name)

### Symbols

Symbols and their meanings:

- *terminal symbols* are written in `"quotes"`
- *non-terminal symbols* are written in `PascalCase`
- *special values* are written in `UPPERCASE`

## Syntax Grammar

### Top-Level

```
Script → Declaration* EOF
```

### Declarations

```
Declaration → ImportDeclaration
            | ClassDeclaration
            | FunctionDeclaration
            | VariableDeclaration
            | Statement
```

#### Import

```
ImportDeclaration → "import" Expression (ImportAs | ImportFor)? ";"

ImportAs → "as" Identifier

ImportFor → "for" Parameters
```

#### Class

```
ClassDeclaration → "class" Identifier ("<" Identifier)? "{" Method* "}"

Method → "static"? "coroutine"? NamedFunction
```

#### Function

```
FunctionDeclaration → "coroutine"? "fun" NamedFunction
```

#### Variable

```
VariableDeclaration → "var" Identifier ("=" Expression)? ";"
```

### Statements

```
Statement → ForStatement
          | WhileStatement
          | DoWhileStatement
          | BreakStatement
          | ContinueStatement
          | WhenStatement
          | IfStatement
          | ReturnStatement
          | PrintStatement
          | BlockStatement
          | ExpressionStatement
```

#### For

```
ForStatement → "for" "(" (VariableDeclaration | ExpressionStatement | ";") Expression? ";" Expression? ")" Statement
```

#### While

```
WhileStatement → "while" "(" Expression ")" Statement
```

#### Do While

```
DoWhileStatement → "do" Statement "while" "(" Expression ")"
```

#### Break

```
BreakStatement → "break" ";"
```

#### Continue

```
ContinueStatement → "continue" ";"
```

#### When

```
WhenStatement → "when" "(" Expression ")" "{" WhenEntry* WhenElse? "}"

WhenEntry → Expression ("," Expression)* "->" Statement

WhenElse → "else" "->" Statement
```

#### If

```
IfStatement → "if" "(" Expression ")" Statement ("else" Statement)?
```

#### Return

```
ReturnStatement → "return" Expression? ";"
```

#### Print

```
PrintStatement → "print" Expression ";"
```

#### Block

```
BlockStatement → "{" Declaration* "}"
```

#### Expression

```
ExpressionStatement → Expression ";"
```

### Expressions

| Precedence | Title           | Operators                                                                          | Associativity |
|:----------:| --------------- | ---------------------------------------------------------------------------------- | ------------- |
| 1          | Postfix         | `++`, `--`,  `.`, `?.`, `[]`, `?[]`, `()`                                          | Left-to-right |
| 2          | Prefix          | `++`, `--`, `-`, `~`, `!`                                                          | Right-to-left |
| 3          | Exponentiation  | `**`                                                                               | Left-to-right |
| 4          | Multiplicative  | `*`, `/`, `%`                                                                      | Left-to-right |
| 5          | Additive        | `+`, `-`                                                                           | Left-to-right |
| 6          | Bitwise Shift   | `>>`, `<<`                                                                         | Left-to-right |
| 7          | Relational      | `>`, `>=`, `<`, `<=`                                                               | Left-to-right |
| 8          | Equality        | `==`, `!=`                                                                         | Left-to-right |
| 9          | Bitwise AND     | `&`                                                                                | Left-to-right |
| 10         | Bitwise XOR     | `^`                                                                                | Left-to-right |
| 11         | Bitwise OR      | `\|`                                                                               | Left-to-right |
| 12         | Logical AND     | `&&`                                                                               | Left-to-right |
| 13         | Logical OR      | `\|\|`                                                                             | Left-to-right |
| 14         | Conditional     | `? :`, `?:`                                                                        | Right-to-left |
| 15         | Assignment      | `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `**=`, `&=`, `^=`, `\|=`, `>>=`, `<<=`, `yield` | Right-to-left |

```
Expression → AssignmentExpression
```

#### Assignment

```
AssignmentExpression → PostfixExpression AssignmentOperator AssignmentExpression
                     | YieldExpression
                     | ConditionalExpression
```

#### Yield

```
YieldExpression → "yield" Expression?
```

#### Conditional

```
ConditionalExpression → LogicOrExpression (TernaryExpressionFinish | ElvisExpressionFinish)?

TernaryExpressionFinish → "?" Expression ":" ConditionalExpression

ElvisExpressionFinish → "?:" ConditionalExpression
```

#### Logic OR

```
LogicOrExpression → LogicAndExpression ("or" LogicAndExpression)*
```

#### Logic AND

```
LogicAndExpression → BitwiseOrExpression ("and" BitwiseOrExpression)*
```

#### Bitwise OR

```
BitwiseOrExpression → BitwiseXorExpression ("|" BitwiseXorExpression)*
```

#### Bitwise XOR

```
BitwiseXorExpression → BitwiseAndExpression ("^" BitwiseAndExpression)*
```

#### Bitwise AND

```
BitwiseAndExpression → EqualityExpression ("&" EqualityExpression)*
```

#### Equality

```
EqualityExpression → ComparisonExpression (EqualityOperator ComparisonExpression)*
```

#### Comparison

```
ComparisonExpression → BitwiseShiftExpression (ComparisonOperator BitwiseShiftExpression)*
```

#### Bitwise Shift

```
BitwiseShiftExpression → AdditiveExpression (BitwiseShiftOperator AdditiveExpression)*
```

#### Additive

```
AdditiveExpression → MultiplicativeExpression (AdditiveOperator MultiplicativeExpression)*
```

#### Multiplicative

```
MultiplicativeExpression → ExponentiationExpression (MultiplicativeOperator ExponentiationExpression)*
```

#### Exponentiation

```
ExponentiationExpression → UnaryExpression ("**" UnaryExpression)*
```

#### Unary

```
UnaryExpression → PrefixUnaryOperator UnaryExpression
                | PostfixExpression
```

#### Postfix

```
PostfixExpression → PrimaryExpression PostfixUnarySuffix*

PostfixUnarySuffix → PostfixUnaryOperator
                   | CallSuffix
                   | NavigationSuffix
                   | SubscriptSuffix

CallSuffix → "(" Arguments? ")"

Arguments → Expression ("," Expression)*

NavigationSuffix → MemberAccessOperator Identifier

SubscriptSuffix → SubscriptOperator Expression "]"
```

#### Primary

```
PrimaryExpression → Literal
                  | Identifier
                  | GroupingExpression
                  | SuperExpression
                  | CoroutineExpression

GroupingExpression → "(" Expression ")"

SuperExpression → "super" "." Identifier

CoroutineExpression → "coroutine" Expression
```

### Literals

```
Literal → BooleanLiteral
        | NumberLiteral
        | StringLiteral
        | ListLiteral
        | MapLiteral
        | LambdaLiteral
        | "nil"
        | "this"

BooleanLiteral → "true"
               | "false"

ListLiteral → "[" (Expression ("," Expression)*)? "]"

MapLiteral → "@{" (MapEntry ("," MapEntry)*)? "}"

MapEntry → Expression ":" Expression

LambdaLiteral → "\" Parameters? "->" (Expression | BlockStatement)
```

### Helpers

```
NamedFunction → Identifier "(" Parameters? ")" ("=" Expression | BlockStatement)

Parameters → Identifier ("," Identifier)*
```

### Operator Terminals

```
MemberAccessOperator → "."
                     | "?."

SubscriptOperator → "["
                  | "?["

PostfixUnaryOperator → "++"
                     | "--"

PrefixUnaryOperator → "-"
                    | "~"
                    | "!"
                    | "++"
                    | "--"

MultiplicativeOperator → "*"
                       | "/"
                       | "%"

AdditiveOperator → "+"
                 | "-"

BitwiseShiftOperator → ">>"
                     | "<<"

ComparisonOperator → ">"
                   | ">="
                   | "<"
                   | "<="

EqualityOperator → "=="
                 | "!="

AssignmentOperator → "="
                   | "+="
                   | "-="
                   | "*="
                   | "/="
                   | "%="
                   | "**="
                   | ">>="
                   | "<<="
                   | "&="
                   | "|="
                   | "^="
```

## Lexical Grammar

```
NumberLiteral → Digit* ("." Digit*)?

StringLiteral → '"' (StringInterpolation | EscapeSequence | ~'"')* '"'

StringInterpolation → "$" Identifier
                    | "${" Expression "}"

EscapeSequence → "\a"
               | "\b"
               | "\f"
               | "\n"
               | "\r"
               | "\t"
               | "\v"
               | "\"
               | "\'"
               | "\""
               | "\$"

Identifier → Alpha (Alpha | Digit)*

Alpha → "a".."z"
      | "A".."Z"
      | "_"

Digit → "0".."9"
```
