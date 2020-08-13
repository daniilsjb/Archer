#include <stdio.h>
#include <string.h>

#include "scanner.h"
#include "common.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

Scanner scanner;

void scanner_init(char const* source)
{
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static bool reached_end()
{
    return *scanner.current == '\0';
}

static Token make_token(TokenType type)
{
    return (Token) { .type = type, .start = scanner.start, .length = (int)(scanner.current - scanner.start), .line = scanner.line };
}

static Token error_token(const char* message)
{
    return (Token) { .type = TOKEN_ERROR, .start = message, .length = (int)strlen(message), .line = scanner.line };
}

static char advance()
{
    return *scanner.current++;
}

static char peek()
{
    return *scanner.current;
}

static char peek_next()
{
    if (reached_end()) {
        return '\0';
    }

    return *(scanner.current + 1);
}

static bool match(char expected)
{
    if (reached_end()) {
        return false;
    }

    if (*scanner.current != expected) {
        return false;
    }

    scanner.current++;
    return true;
}

static Token skip_whitespace()
{
    while (true) {
        char c = peek();

        if (c == ' ' || c == '\r' || c == '\t') {
            advance();
            continue;
        }

        if (c == '\n') {
            scanner.line++;
            advance();
            continue;
        }

        if (c == '/') {
            if (peek_next() == '/') {
                while (peek() != '\n' && !reached_end()) {
                    advance();
                }
                continue;
            }

            if (peek_next() == '*') {
                advance();
                advance();

                while (true) {
                    if (match('*') && match('/')) {
                        break;
                    }

                    if (peek() == '\n') {
                        scanner.line++;
                    }

                    advance();
                    if (reached_end()) {
                        return error_token("Unterminated block comment.");
                    }
                }
                continue;
            }

            return make_token(TOKEN_NONE);
        }

        return make_token(TOKEN_NONE);
    }
}

static Token string()
{
    while (peek() != '"' && !reached_end()) {
        if (peek() == '\n') {
            scanner.line++;
        }
        advance();
    }

    if (reached_end()) {
        error_token("Unterminated string.");
    }

    advance();
    return make_token(TOKEN_STRING);
}

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static Token number()
{
    while (is_digit(peek())) {
        advance();
    }

    if (match('.')) {
        while (is_digit(peek())) {
            advance();
        }
    }

    return make_token(TOKEN_NUMBER);
}

static bool is_alpha(char c)
{
    return
        (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c == '_');
}

static bool is_alpha_num(char c)
{
    return is_alpha(c) || is_digit(c);
}

static TokenType check_keyword(int start, int length, const char* rest, TokenType type)
{
    if (scanner.current - scanner.start == start + length && memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

static TokenType identifier_type()
{
    switch (*scanner.start) {
        case 'a': return check_keyword(1, 2, "nd", TOKEN_AND);
        case 'b': return check_keyword(1, 4, "reak", TOKEN_BREAK);
        case 'c': {
            if (scanner.current - scanner.start > 1) {
                switch (*(scanner.start + 1)) {
                    case 'a': return check_keyword(2, 2, "se", TOKEN_CASE);
                    case 'l': return check_keyword(2, 3, "ass", TOKEN_CLASS);
                    case 'o': return check_keyword(2, 6, "ntinue", TOKEN_CONTINUE);
                }
            }
        }
        case 'd': return check_keyword(1, 6, "efault", TOKEN_DEFAULT);
        case 'e': return check_keyword(1, 3, "lse", TOKEN_ELSE);
        case 'f': {
            if (scanner.current - scanner.start > 1) {
                switch (*(scanner.start + 1)) {
                    case 'a': return check_keyword(2, 3, "lse", TOKEN_FALSE);
                    case 'o': return check_keyword(2, 1, "r", TOKEN_FOR);
                    case 'u': return check_keyword(2, 1, "n", TOKEN_FUN);
                }
            }
        }
        case 'i': return check_keyword(1, 1, "f", TOKEN_IF);
        case 'n': return check_keyword(1, 2, "il", TOKEN_NIL);
        case 'o': return check_keyword(1, 1, "r", TOKEN_OR);
        case 'p': return check_keyword(1, 4, "rint", TOKEN_PRINT);
        case 'r': return check_keyword(1, 5, "eturn", TOKEN_RETURN);
        case 's': {
            if (scanner.current - scanner.start > 1) {
                switch (*(scanner.start + 1)) {
                    case 'w': return check_keyword(2, 4, "itch", TOKEN_SWITCH);
                    case 'u': return check_keyword(2, 3, "per", TOKEN_SUPER);
                    case 't': return check_keyword(2, 4, "atic", TOKEN_STATIC);
                }
            }
        }
        case 't': {
            if (scanner.current - scanner.start > 1) {
                switch (*(scanner.start + 1)) {
                    case 'h': return check_keyword(2, 2, "is", TOKEN_THIS);
                    case 'r': return check_keyword(2, 2, "ue", TOKEN_TRUE);
                }
            }
        }
        case 'v': return check_keyword(1, 2, "ar", TOKEN_VAR);
        case 'w': return check_keyword(1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

static Token identifier()
{
    while (is_alpha_num(peek())) {
        advance();
    }

    return make_token(identifier_type());
}

Token scan_token()
{
    Token whitespace = skip_whitespace();
    if (whitespace.type == TOKEN_ERROR) {
        return whitespace;
    }

    scanner.start = scanner.current;
    if (reached_end()) {
        return make_token(TOKEN_EOF);
    }

    char c = advance();
    if (is_alpha(c)) {
        return identifier();
    }
    if (is_digit(c)) {
        return number();
    }

    switch (c) {
        case '(': return make_token(TOKEN_L_PAREN);
        case ')': return make_token(TOKEN_R_PAREN);
        case '{': return make_token(TOKEN_L_BRACE);
        case '}': return make_token(TOKEN_R_BRACE);
        case '[': return make_token(TOKEN_L_BRACKET);
        case ']': return make_token(TOKEN_R_BRACKET);
        case ';': return make_token(TOKEN_SEMICOLON);
        case ',': return make_token(TOKEN_COMMA);
        case '.': return make_token(TOKEN_DOT);
        case '?': return make_token(TOKEN_QUESTION);
        case ':': return make_token(TOKEN_COLON);
        case '-': return make_token(match('=') ? TOKEN_MINUS_EQUAL : TOKEN_MINUS);
        case '+': return make_token(match('=') ? TOKEN_PLUS_EQUAL : TOKEN_PLUS);
        case '/': return make_token(match('=') ? TOKEN_SLASH_EQUAL : TOKEN_SLASH);
        case '*': return make_token(match('=') ? TOKEN_STAR_EQUAL : TOKEN_STAR);
        case '%': return make_token(match('=') ? TOKEN_PERCENT_EQUAL : TOKEN_PERCENT);
        case '!': return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=': return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '>': return make_token(match('=') ? TOKEN_GREATER_EQUAL : (match('>') ? (match('=') ? TOKEN_R_SHIFT_EQUAL : TOKEN_R_SHIFT) : TOKEN_GREATER));
        case '<': return make_token(match('=') ? TOKEN_LESS_EQUAL : (match('<') ? (match('=') ? TOKEN_L_SHIFT_EQUAL : TOKEN_L_SHIFT) : TOKEN_LESS));
        case '~': return make_token(TOKEN_TILDE);
        case '&': return make_token(match('=') ? TOKEN_AMPERSAND_EQUAL : TOKEN_AMPERSAND);
        case '|': return make_token(match('=') ? TOKEN_PIPE_EQUAL : TOKEN_PIPE);
        case '^': return make_token(match('=') ? TOKEN_CARET_EQUAL : TOKEN_CARET);
        case '"': return string();
    }

    return error_token("Unexpected character.");
}
