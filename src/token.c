#include <string.h>

#include "token.h"

bool Token_LexemesEqual(Token* a, Token* b)
{
    return a->length == b->length && memcmp(a->start, b->start, a->length) == 0;
}

Token Token_Synthetic(const char* lexeme)
{
    return (Token) { .start = lexeme, .length = strlen(lexeme) };
}

Token Token_Empty()
{
    return (Token) { .type = TOKEN_NONE, .start = NULL, .length = 0, .line = 0 };
}
