# Grammar

```
Program → Declaration* EOF

Declaration → ClassDecl
            | FunctionDecl
            | VariableDecl
            | Statement

ClassDecl → "class" IDENTIFIER ( "<" IDENTIFIER )? "{" Function* "}"
FunctionDecl → "fun" Function
VariableDecl → "var" IDENTIFIER ( "=" Expression )?

Statement → ForStmt
          | WhileStmt
          | BreakStmt
          | ContinueStmt
          | WhenStmt
          | IfStmt
          | ReturnStmt
          | PrintStmt
          | BlockStmt
          | ExprStmt

ForStmt → "for" "(" ( VariableDecl | ExprStmt | ";" ) Expression? ";" Expression? ")" Statement
WhileStmt → "while" "(" Expression ")" Statement
BreakStmt → "break" ";"
ContinueStmt → "continue" ";"
WhenStmt → "when" "(" Expression ")" "{" WhenEntry* ( "else" "->" Statement )? "}"
IfStmt → "if" "(" Expression ")" Statement ( "else" Statement )?
ReturnStmt → "return" Expression? ";"
PrintStmt → "print" Expression ";"
BlockStmt → "{" Declaration* "}"
ExprStmt → Expression ";"

Expression → AssignmentExpr
AssignmentExpr → ( ( PostfixExpr "." )? IDENTIFIER AssignOp AssignmentExpr ) | ConditionalExpr
ConditionalExpr → LogicOrExpr ( "?" Expression ":" ConditionlExpr )?
LogicOrExpr → LogicAndExpr ( "or" LogicAndExpr )*
LogicAndExpr → BitwiseOrExpr ( "and" BitwiseOrExpr )*
BitwiseOrExpr → BitwiseXorExpr ( "|" BitwiseXorExpr )*
BitwiseXorExpr → BitwiseAndExpr ( "^" EqualityExpr )*
BitwiseAndExpr → EqualityExpr ( "&" BitwiseAndExpr )*
EqualityExpr → ComparisonExpr ( ( "==" | "!=" ) ComparisonExpr )*
ComparisonExpr → BitwiseShiftExpr ( ( "<" | ">" | "<=" | ">=" ) BitwiseShiftExpr )*
BitwiseShiftExpr → AdditionExpr ( ( "<<" | ">>" ) AdditionExpr )*
AdditionExpr → MultiplicationExpr ( ( "+" | "-" ) MultiplicationExpr )*
MultiplicationExpr → ExponentiationExpr ( ( "*" | "/" | "%" ) ExponentiationExpr )*
ExponentiationExpr → UnaryExpr ( ( "**" UnaryExpr ) )*
UnaryExpr → ( "!" | "-" | "++" | "--" ) UnaryExpr | PostfixExpr
PostfixExpr → PrimaryExpr ( "(" Arguments? ")" | "." IDENTIFIER )* ( "++" | "--" )?
PrimaryExpr → "true" | "false" | "nil" | "this"
            | STRING | NUMBER | IDENTIFIER | "(" Expression ")"
            | "super" "." IDENTIFIER

WhenEntry → Expression ( "," Expression )* "->" Statement
Function → IDENTIFIER "(" Parameters? ")" BlockStmt
Parameters → IDENTIFIER ( "," IDENTIFIER )*
Arguments → Expression ( "," Expression )*
AssignOp → "=" | "+=" | "-=" | "*=" | "/=" | "%=" | "**=" | ">>=" | "<<=" | "&=" | "|=" | "^="
```
