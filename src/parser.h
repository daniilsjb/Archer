#ifndef PARSER_H
#define PARSER_H

#include "common.h"
#include "scanner.h"
#include "token.h"

typedef struct {
    Scanner scanner;

    Token current;
    Token previous;

    bool error;
    bool panic;
} Parser;

void parser_init(Parser* parser, const char* source);

void parser_move_previous(Parser* parser);
TokenType parser_peek_type(Parser* parser);
bool parser_check(Parser* parser, TokenType type);
bool parser_advance(Parser* parser);

void parser_synchronize(Parser* parser);
void parser_enter_error_mode(Parser* parser);

#endif
