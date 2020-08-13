#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
    /* Brackets */
    TOKEN_L_PAREN,
    TOKEN_R_PAREN,
    TOKEN_L_BRACE,
    TOKEN_R_BRACE,
    TOKEN_L_BRACKET,
    TOKEN_R_BRACKET,

    /* Punctuation */
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_SEMICOLON,
    TOKEN_QUESTION,
    TOKEN_COLON,
    TOKEN_TILDE,

    /* Arithmetic */
    TOKEN_PLUS,
    TOKEN_PLUS_EQUAL,
    TOKEN_DOUBLE_PLUS,
    TOKEN_MINUS,
    TOKEN_MINUS_EQUAL,
    TOKEN_DOUBLE_MINUS,
    TOKEN_STAR,
    TOKEN_STAR_EQUAL,
    TOKEN_SLASH,
    TOKEN_SLASH_EQUAL,
    TOKEN_PERCENT,
    TOKEN_PERCENT_EQUAL,

    /* Bitwise */
    TOKEN_AMPERSAND,
    TOKEN_AMPERSAND_EQUAL,
    TOKEN_PIPE,
    TOKEN_PIPE_EQUAL,
    TOKEN_CARET,
    TOKEN_CARET_EQUAL,
    TOKEN_L_SHIFT,
    TOKEN_L_SHIFT_EQUAL,
    TOKEN_R_SHIFT,
    TOKEN_R_SHIFT_EQUAL,

    /* Comparison and Equality */
    TOKEN_BANG,
    TOKEN_BANG_EQUAL,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,

    /* Keywords */
    TOKEN_AND,
    TOKEN_BREAK,
    TOKEN_CASE,
    TOKEN_CLASS,
    TOKEN_CONTINUE,
    TOKEN_DEFAULT,
    TOKEN_ELSE,
    TOKEN_FALSE,
    TOKEN_FUN,
    TOKEN_FOR,
    TOKEN_IF,
    TOKEN_NIL,
    TOKEN_OR,
    TOKEN_PRINT,
    TOKEN_RETURN,
    TOKEN_SWITCH,
    TOKEN_STATIC,
    TOKEN_SUPER,
    TOKEN_THIS,
    TOKEN_TRUE,
    TOKEN_VAR,
    TOKEN_WHILE,

    /* Literals */
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,

    /* Flags */
    TOKEN_ERROR,
    TOKEN_NONE,
    TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    size_t length;
    int line;
} Token;

#endif
