# Grammar

## Description

### Notation

The notation used is a modified version of Extended Backus-Naur Form. It uses syntax that is more resemblant to regular expressions for brevity and clarity.

### Operators

Operators and their meanings:

- operator `|` denotes *alternative* (only a single rule must match)
- operator `?` denotes *option* (zero or one match)
- operator `*` denotes *iteration* (zero or more matches)
- operator `+` denotes *iteration* (one or more matches)
- operator `()` denotes *grouping* (a sequence of symbols is treated as a single rule)
- operator `..` denotes *range* (match anything between two iterative values)
- operator `→` denotes *binding* (name is bound to a rule)

### Symbols

Symbols and their meanings:

- *terminal symbols* are written in `"quotes"`
- *non-terminal symbols* are written in `PascalCase`
- *special values* are written in `UPPERCASE`

## Syntax Grammar

### Top-Level

```ebnf
Script → Declaration* EOF
```

### Declarations

```ebnf
Declaration → ClassDeclaration
            | FunctionDeclaration
            | VariableDeclaration
            | Statement
```

```ebnf
ClassDeclaration → "class" Identifier ("<" Identifier)? "{" Method* "}"
```

```ebnf
FunctionDeclaration → "fun" NamedFunction
```

```ebnf
VariableDeclaration → "var" Identifier ("=" Expression)? ";"
```

### Statements

```ebnf
Statement → ForStatement
          | WhileStatement
          | BreakStatement
          | ContinueStatement
          | WhenStatement
          | IfStatement
          | ReturnStatement
          | PrintStatement
          | BlockStatement
          | ExpressionStatement
```

```ebnf
ForStatement → "for" "(" (VariableDeclaration | ExpressionStatement | ";")? Expression? ";" Expression? ")" Statement
```

```ebnf
WhileStatement → "while" "(" Expression ")" Statement
```

```ebnf
BreakStatement → "break" ";"
```

```ebnf
ContinueStatement → "continue" ";"
```

```ebnf
WhenStatement → "when" "(" Expression ")" "{" WhenEntry* WhenElse? "}"
```

```ebnf
IfStatement → "if" "(" Expression ")" Statement ("else" Statement)?
```

```ebnf
ReturnStatement → "return" Expression? ";"
```

```ebnf
PrintStatement → "print" Expression ";"
```

```ebnf
BlockStatement → "{" Declaration* "}"
```

```ebnf
ExpressionStatement → Expression ";"
```

### Expressions

| Precedence | Title           | Operators                                                                | Associativity |
|:----------:| --------------- | ------------------------------------------------------------------------ | ------------- |
| 1          | Postfix         | `++`, `--`,  `.`, `?.`, `()`                                             | Left-to-right |
| 2          | Prefix          | `++`, `--`, `-`, `~`, `!`                                                | Right-to-left |
| 3          | Exponentiation  | `**`                                                                     | Left-to-right |
| 4          | Multiplicative  | `*`, `/`, `%`                                                            | Left-to-right |
| 5          | Additive        | `+`, `-`                                                                 | Left-to-right |
| 6          | Bitwise Shift   | `>>`, `<<`                                                               | Left-to-right |
| 7          | Relational      | `>`, `>=`, `<`, `<=`                                                     | Left-to-right |
| 8          | Equality        | `==`, `!=`                                                               | Left-to-right |
| 9          | Bitwise AND     | `&`                                                                      | Left-to-right |
| 10         | Bitwise XOR     | `^`                                                                      | Left-to-right |
| 11         | Bitwise OR      | `|`                                                                      | Left-to-right |
| 12         | Logical AND     | `&&`                                                                     | Left-to-right |
| 13         | Logical OR      | `||`                                                                     | Left-to-right |
| 14         | Conditional     | `? :`, `?:`                                                              | Right-to-left |
| 15         | Assignment      | `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `**=`, `&=`, `^=`, `|=`, `>>=`, `<<=` | Right-to-left |

```ebnf
Expression → AssignmentExpression
```

```ebnf
AssignmentExpression → (PostfixExpression MemberAccessOperator)? Identifier AssignmentOperator AssignmentExpression
                     | ConditionalExpression
```

```ebnf
ConditionalExpression → LogicOrExpression (TernaryExpressionFinish | ElvisExpressionFinish)?
```

```ebnf
TernaryExpressionFinish → "?" Expression ":" ConditionalExpression
```

```ebnf
ElvisExpressionFinish → "?:" ConditionalExpression
```

```ebnf
LogicOrExpression → LogicAndExpression ("or" LogicAndExpression)*
```

```ebnf
LogicAndExpression → BitwiseOrExpression ("and" BitwiseOrExpression)*
```

```ebnf
BitwiseOrExpression → BitwiseXorExpression ("|" BitwiseXorExpression)*
```

```ebnf
BitwiseXorExpression → BitwiseAndExpression ("^" BitwiseAndExpression)*
```

```ebnf
BitwiseAndExpression → EqualityExpression ("&" EqualityExpression)*
```

```ebnf
EqualityExpression → ComparisonExpression (EqualityOperator ComparisonExpression)*
```

```ebnf
ComparisonExpression → BitwiseShiftExpression (ComparisonOperator BitwiseShiftExpression)*
```

```ebnf
BitwiseShiftExpression → AdditiveExpression (BitwiseShiftOperator AdditiveExpression)*
```

```ebnf
AdditiveExpression → MultiplicativeExpression (AdditiveOperator MultiplicativeExpression)*
```

```ebnf
MultiplicativeExpression → ExponentiationExpression (MultiplicativeOperator ExponentiationExpression)*
```

```ebnf
ExponentiationExpression → UnaryExpression ("**" UnaryExpression)*
```

```ebnf
UnaryExpression → PrefixUnaryOperator UnaryExpression
                | PostfixExpression
```

```ebnf
PostfixExpression → PrimaryExpression PostfixUnarySuffix*
```

```ebnf
PostfixUnarySuffix → PostfixUnaryOperator
                   | CallSuffix
                   | NavigationSuffix
```

```ebnf
CallSuffix → "(" Arguments? ")"
```

```ebnf
NavigationSuffix → MemberAccessOperator Identifier
```

```ebnf
PrimaryExpression → Literal
                  | Identifier
                  | GroupingExpression
                  | SuperExpression
```

```ebnf
GroupingExpression → "(" Expression ")"
```

```ebnf
SuperExpression → "super" "." Identifier
```

### Literals

```ebnf
Literal → BooleanLiteral
        | NumberLiteral
        | StringLiteral
        | LambdaLiteral
        | "nil"
        | "this"

```

```ebnf
BooleanLiteral → "true"
               | "false"
```

```ebnf
NumberLiteral → Digit* ("." Digit*)?
```

```ebnf
Digit → "0".."9"
```

```ebnf
StringLiteral → '"' TEXT '"'
```

```ebnf
LambdaLiteral → "\" Parameters? "->" (Expression | BlockStatement)
```

### Identifiers

```ebnf
Identifier → Alpha (Alpha | Digit)*
```

```ebnf
Alpha → "a".."z"
      | "A".."Z"
      | "_"
```

### Helpers

```ebnf
Method → "static"? NamedFunction
```

```ebnf
NamedFunction → Identifier "(" Parameters? ")" ("=" Expression | BlockStatement)
```

```ebnf
WhenEntry → Expression ("," Expression)* "->" Statement
```

```ebnf
WhenElse → "else" "->" Statement
```

```ebnf
Parameters → Identifier ("," Identifier)*
```

```ebnf
Arguments → Expression ("," Expression)*
```

### Operator Terminals

```ebnf
MemberAccessOperator → "."
                     | "?."
```

```ebnf
PostfixUnaryOperator → "++"
                     | "--"
```

```ebnf
PrefixUnaryOperator → "-"
                    | "~"
                    | "!"
                    | "++"
                    | "--"
```

```ebnf
MultiplicativeOperator → "*"
                       | "/"
                       | "%"
```

```ebnf
AdditiveOperator → "+"
                 | "-"
```

```ebnf
BitwiseShiftOperator → ">>"
                     | "<<"
```

```ebnf
ComparisonOperator → ">"
                   | ">="
                   | "<"
                   | "<="
```

```ebnf
EqualityOperator → "=="
                 | "!="
```

```ebnf
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
