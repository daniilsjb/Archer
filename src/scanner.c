#include <string.h>
#include <ctype.h>

#include "scanner.h"
#include "common.h"

void scanner_init(Scanner* scanner, const char* source)
{
    scanner->start = source;
    scanner->current = source;
    scanner->line = 1;
}

static bool reached_end(Scanner* scanner)
{
    return *scanner->current == '\0';
}

static char advance(Scanner* scanner)
{
    return *scanner->current++;
}

static char peek(Scanner* scanner)
{
    return *scanner->current;
}

static char peek_next(Scanner* scanner)
{
    if (reached_end(scanner)) {
        return '\0';
    }

    return *(scanner->current + 1);
}

static bool match(Scanner* scanner, char expected)
{
    if (reached_end(scanner)) {
        return false;
    }

    if (*scanner->current != expected) {
        return false;
    }

    scanner->current++;
    return true;
}

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}

static bool is_alpha_num(char c)
{
    return is_alpha(c) || is_digit(c);
}

static Token make_token(Scanner* scanner, TokenType type)
{
    return (Token) { .type = type, .start = scanner->start, .length = (int)(scanner->current - scanner->start), .line = scanner->line };
}

static Token error_token(const char* message, int line)
{
    return (Token) { .type = TOKEN_ERROR, .start = message, .length = (int)strlen(message), .line = line };
}

static Token skip_whitespace(Scanner* scanner)
{
    while (true) {
        char c = peek(scanner);

        if (c == '\n') {
            scanner->line++;
            advance(scanner);
            continue;
        }

        if (isspace(c)) {
            advance(scanner);
            continue;
        }

        if (c == '/') {
            if (peek_next(scanner) == '/') {
                while (peek(scanner) != '\n' && !reached_end(scanner)) {
                    advance(scanner);
                }
                continue;
            }

            if (peek_next(scanner) == '*') {
                advance(scanner);
                advance(scanner);

                while (!match(scanner, '*') || !match(scanner, '/')) {
                    if (peek(scanner) == '\n') {
                        scanner->line++;
                    }

                    advance(scanner);
                    if (reached_end(scanner)) {
                        return error_token("Unterminated block comment.", scanner->line);
                    }
                }
                continue;
            }

            return make_token(scanner, TOKEN_NONE);
        }

        return make_token(scanner, TOKEN_NONE);
    }
}

static Token string(Scanner* scanner)
{
    while (peek(scanner) != '"' && !reached_end(scanner)) {
        if (peek(scanner) == '\n') {
            scanner->line++;
        }
        advance(scanner);
    }

    if (reached_end(scanner)) {
        return error_token("Unterminated string.", scanner->line);
    }

    advance(scanner);
    return make_token(scanner, TOKEN_STRING);
}

static Token number(Scanner* scanner)
{
    while (is_digit(peek(scanner))) {
        advance(scanner);
    }

    if (match(scanner, '.')) {
        while (is_digit(peek(scanner))) {
            advance(scanner);
        }
    }

    return make_token(scanner, TOKEN_NUMBER);
}

static TokenType check_keyword(Scanner* scanner, int start, int length, const char* rest, TokenType type)
{
    if ((int)(scanner->current - scanner->start) == start + length && memcmp(scanner->start + start, rest, length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

static TokenType identifier_type(Scanner* scanner)
{
    switch (*scanner->start) {
        case 'a': return check_keyword(scanner, 1, 2, "nd", TOKEN_AND);
        case 'b': return check_keyword(scanner, 1, 4, "reak", TOKEN_BREAK);
        case 'c': {
            if (scanner->current - scanner->start > 1) {
                switch (*(scanner->start + 1)) {
                    case 'a': return check_keyword(scanner, 2, 2, "se", TOKEN_CASE);
                    case 'l': return check_keyword(scanner, 2, 3, "ass", TOKEN_CLASS);
                    case 'o': return check_keyword(scanner, 2, 6, "ntinue", TOKEN_CONTINUE);
                }
            }
        }
        case 'd': return check_keyword(scanner, 1, 6, "efault", TOKEN_DEFAULT);
        case 'e': return check_keyword(scanner, 1, 3, "lse", TOKEN_ELSE);
        case 'f': {
            if (scanner->current - scanner->start > 1) {
                switch (*(scanner->start + 1)) {
                    case 'a': return check_keyword(scanner, 2, 3, "lse", TOKEN_FALSE);
                    case 'o': return check_keyword(scanner, 2, 1, "r", TOKEN_FOR);
                    case 'u': return check_keyword(scanner, 2, 1, "n", TOKEN_FUN);
                }
            }
        }
        case 'i': return check_keyword(scanner, 1, 1, "f", TOKEN_IF);
        case 'n': return check_keyword(scanner, 1, 2, "il", TOKEN_NIL);
        case 'o': return check_keyword(scanner, 1, 1, "r", TOKEN_OR);
        case 'p': return check_keyword(scanner, 1, 4, "rint", TOKEN_PRINT);
        case 'r': return check_keyword(scanner, 1, 5, "eturn", TOKEN_RETURN);
        case 's': {
            if (scanner->current - scanner->start > 1) {
                switch (*(scanner->start + 1)) {
                    case 'w': return check_keyword(scanner, 2, 4, "itch", TOKEN_SWITCH);
                    case 'u': return check_keyword(scanner, 2, 3, "per", TOKEN_SUPER);
                    case 't': return check_keyword(scanner, 2, 4, "atic", TOKEN_STATIC);
                }
            }
        }
        case 't': {
            if (scanner->current - scanner->start > 1) {
                switch (*(scanner->start + 1)) {
                    case 'h': return check_keyword(scanner, 2, 2, "is", TOKEN_THIS);
                    case 'r': return check_keyword(scanner, 2, 2, "ue", TOKEN_TRUE);
                }
            }
        }
        case 'v': return check_keyword(scanner, 1, 2, "ar", TOKEN_VAR);
        case 'w': return check_keyword(scanner, 1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

static Token identifier(Scanner* scanner)
{
    while (is_alpha_num(peek(scanner))) {
        advance(scanner);
    }

    return make_token(scanner, identifier_type(scanner));
}

Token scanner_scan_token(Scanner* scanner)
{
    Token whitespace = skip_whitespace(scanner);
    if (whitespace.type == TOKEN_ERROR) {
        return whitespace;
    }

    scanner->start = scanner->current;
    if (reached_end(scanner)) {
        return make_token(scanner, TOKEN_EOF);
    }

    char c = advance(scanner);
    switch (c) {
        case '(': return make_token(scanner, TOKEN_L_PAREN);
        case ')': return make_token(scanner, TOKEN_R_PAREN);
        case '{': return make_token(scanner, TOKEN_L_BRACE);
        case '}': return make_token(scanner, TOKEN_R_BRACE);
        case '[': return make_token(scanner, TOKEN_L_BRACKET);
        case ']': return make_token(scanner, TOKEN_R_BRACKET);
        case ';': return make_token(scanner, TOKEN_SEMICOLON);
        case ',': return make_token(scanner, TOKEN_COMMA);
        case '.': return make_token(scanner, TOKEN_DOT);
        case '?': return make_token(scanner, TOKEN_QUESTION);
        case ':': return make_token(scanner, TOKEN_COLON);
        case '-': {
            switch (peek(scanner)) {
                case '=': advance(scanner); return make_token(scanner, TOKEN_MINUS_EQUAL);
                case '-': advance(scanner); return make_token(scanner, TOKEN_DOUBLE_MINUS);
            }
            return make_token(scanner, TOKEN_MINUS);
        }
        case '+': {
            switch (peek(scanner)) {
                case '=': advance(scanner); return make_token(scanner, TOKEN_PLUS_EQUAL);
                case '+': advance(scanner); return make_token(scanner, TOKEN_DOUBLE_PLUS);
            }
            return make_token(scanner, TOKEN_PLUS);
        }
        case '/': {
            switch (peek(scanner)) {
                case '=': advance(scanner); return make_token(scanner, TOKEN_SLASH_EQUAL);
            }
            return make_token(scanner, TOKEN_SLASH);
        }
        case '*': {
            switch (peek(scanner)) {
                case '=': advance(scanner); return make_token(scanner, TOKEN_STAR_EQUAL);
                case '*': advance(scanner); return make_token(scanner, TOKEN_DOUBLE_STAR);
            }
            return make_token(scanner, TOKEN_STAR);
        }
        case '%': {
            switch (peek(scanner)) {
                case '=': advance(scanner); return make_token(scanner, TOKEN_PERCENT_EQUAL);
            }
            return make_token(scanner, TOKEN_PERCENT);
        }
        case '!': {
            switch (peek(scanner)) {
                case '=': advance(scanner); return make_token(scanner, TOKEN_BANG_EQUAL);
            }
            return make_token(scanner, TOKEN_BANG);
        }
        case '=': {
            switch (peek(scanner)) {
                case '=': advance(scanner); return make_token(scanner, TOKEN_EQUAL_EQUAL);
            }
            return make_token(scanner, TOKEN_EQUAL);
        }
        case '>': {
            switch (peek(scanner)) {
                case '=': advance(scanner); return make_token(scanner, TOKEN_GREATER_EQUAL);
                case '>': {
                    advance(scanner);
                    switch (peek(scanner)) {
                        case '=': advance(scanner); return make_token(scanner, TOKEN_R_SHIFT_EQUAL);
                    }
                    return make_token(scanner, TOKEN_R_SHIFT);
                }
            }

            return make_token(scanner, TOKEN_GREATER);
        }
        case '<': {
            switch (peek(scanner)) {
                case '=': advance(scanner); return make_token(scanner, TOKEN_LESS_EQUAL);
                case '<': {
                    advance(scanner);
                    switch (peek(scanner)) {
                        case '=': advance(scanner); return make_token(scanner, TOKEN_L_SHIFT_EQUAL);
                    }
                    return make_token(scanner, TOKEN_L_SHIFT);
                }
            }

            return make_token(scanner, TOKEN_LESS);
        }
        case '~': return make_token(scanner, TOKEN_TILDE);
        case '&': {
            switch (peek(scanner)) {
                case '=': advance(scanner); return make_token(scanner, TOKEN_AMPERSAND_EQUAL);
            }
            return make_token(scanner, TOKEN_AMPERSAND);
        }
        case '|': {
            switch (peek(scanner)) {
                case '=': advance(scanner); return make_token(scanner, TOKEN_PIPE_EQUAL);
            }
            return make_token(scanner, TOKEN_PIPE);
        }
        case '^': {
            switch (peek(scanner)) {
                case '=': advance(scanner); return make_token(scanner, TOKEN_CARET_EQUAL);
            }
            return make_token(scanner, TOKEN_CARET);
        }
        case '"': return string(scanner);
    }

    if (is_alpha(c)) {
        return identifier(scanner);
    }

    if (is_digit(c)) {
        return number(scanner);
    }

    return error_token("Unexpected character.", scanner->line);
}
