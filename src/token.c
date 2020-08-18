#include <string.h>

#include "token.h"

bool lexemes_equal(Token* a, Token* b)
{
    return a->length == b->length && memcmp(a->start, b->start, a->length) == 0;
}

Token synthetic_token(const char* lexeme)
{
    return (Token) { .start = lexeme, .length = strlen(lexeme) };
}
