#include "parser.h"

void parser_init(Parser* parser, const char* source)
{
    Scanner scanner;
    scanner_init(&scanner, source);

    parser->scanner = scanner;
    parser->error = false;
    parser->panic = false;
}

void parser_move_previous(Parser* parser)
{
    parser->previous = parser->current;
}

bool parser_check(Parser* parser, TokenType type)
{
    return parser->current.type == type;
}

bool parser_advance(Parser* parser)
{
    parser->current = scanner_scan_token(&parser->scanner);
    return !parser_check(parser, TOKEN_ERROR);
}

void parser_synchronize(Parser* parser)
{
    parser->panic = false;

    while (!parser_check(parser, TOKEN_EOF)) {
        switch (parser->previous.type) {
            case TOKEN_SEMICOLON: return;
        }

        switch (parser->current.type) {
            case TOKEN_CLASS: return;
            case TOKEN_STATIC: return;
            case TOKEN_FUN: return;
            case TOKEN_VAR: return;
            case TOKEN_FOR: return;
            case TOKEN_IF: return;
            case TOKEN_WHILE: return;
            case TOKEN_PRINT: return;
            case TOKEN_BREAK: return;
            case TOKEN_CONTINUE: return;
            case TOKEN_RETURN: return;
        }

        parser_advance(parser);
    }
}

void parser_enter_error_mode(Parser* parser)
{
    parser->error = true;
    parser->panic = true;
}
