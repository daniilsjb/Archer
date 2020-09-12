#include <string.h>
#include <ctype.h>

#include "scanner.h"
#include "common.h"

void scanner_init(Scanner* scanner, const char* source)
{
    scanner->start = source;
    scanner->current = source;
    scanner->line = 1;

    scanner_clear(scanner);
}

static void clear_interpolation_stack(Scanner* scanner)
{
    memset(scanner->unmatchedInterpolations, 0, sizeof(scanner->unmatchedInterpolations));
    scanner->interpolationDepth = -1;
}

void scanner_clear(Scanner* scanner)
{
    clear_interpolation_stack(scanner);
}

static void move_start(Scanner* scanner)
{
    scanner->start = scanner->current;
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

static Token make_token_at(Scanner* scanner, TokenType type, const char* end)
{
    return (Token) { .type = type, .start = scanner->start, .length = end - scanner->start, .line = scanner->line };
}

static Token make_token(Scanner* scanner, TokenType type)
{
    return make_token_at(scanner, type, scanner->current);
}

static Token error_token(const char* message, int line)
{
    return (Token) { .type = TOKEN_ERROR, .start = message, .length = strlen(message), .line = line };
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

static void open_brace(Scanner* scanner)
{
    scanner->unmatchedInterpolations[scanner->interpolationDepth]++;
}

static void close_brace(Scanner* scanner)
{
    scanner->unmatchedInterpolations[scanner->interpolationDepth]--;
}

static bool all_braces_matched(Scanner* scanner)
{
    return scanner->unmatchedInterpolations[scanner->interpolationDepth] == 0;
}

static bool interpolating(Scanner* scanner)
{
    return scanner->interpolationDepth != -1;
}

static bool interpolating_expression(Scanner* scanner)
{
    return interpolating(scanner) && !all_braces_matched(scanner);
}

static bool interpolating_identifier(Scanner* scanner)
{
    return interpolating(scanner) && all_braces_matched(scanner);
}

static void end_identifier_interpolation(Scanner* scanner)
{
    scanner->unmatchedInterpolations[scanner->interpolationDepth] = -1;
}

static bool interpolated_identifier(Scanner* scanner)
{
    return scanner->unmatchedInterpolations[scanner->interpolationDepth] == -1;
}

static char enter_interpolation(Scanner* scanner)
{
    return ++scanner->interpolationDepth;
}

static char leave_interpolation(Scanner* scanner)
{
    scanner->unmatchedInterpolations[scanner->interpolationDepth] = 0;
    return --scanner->interpolationDepth;
}

static Token string(Scanner* scanner)
{
    while (!match(scanner, '"')) {
        if (peek(scanner) == '\n' || reached_end(scanner)) {
            return error_token("Unterminated string.", scanner->line);
        }

        char c = advance(scanner);
        if (c == '$') {
            if (!is_alpha(peek(scanner)) && peek(scanner) != '{') {
                continue;
            }

            if (enter_interpolation(scanner) >= MAX_INTERPOLATION_DEPTH) {
                return error_token("Exceeded string interpolation limit.", scanner->line);
            }

            const char* end = scanner->current - 1;
            if (match(scanner, '{')) {
                open_brace(scanner);
            }

            return make_token_at(scanner, TOKEN_STRING_INTERP, end);
        }

        if (c == '\\') {
            switch (peek(scanner)) {
                case 'a':
                case 'b':
                case 'f':
                case 'n':
                case 'r':
                case 't':
                case 'v':
                case '\\':
                case '\'':
                case '\"':
                case '$': advance(scanner); break;
                default: return error_token("Invalid escape sequence.", scanner->line);
            }
        }
    }

    return make_token_at(scanner, TOKEN_STRING, scanner->current - 1);
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

static TokenType check_keyword(Scanner* scanner, size_t start, size_t length, const char* rest, TokenType type)
{
    if (scanner->current - scanner->start == start + length && memcmp(scanner->start + start, rest, length) == 0) {
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
        case 'w': {
            if (scanner->current - scanner->start > 1) {
                switch (*(scanner->start + 1)) {
                    case 'h': {
                        if (scanner->current - scanner->start > 2) {
                            switch (*(scanner->start + 2)) {
                                case 'e': return check_keyword(scanner, 3, 1, "n", TOKEN_WHEN);
                                case 'i': return check_keyword(scanner, 3, 2, "le", TOKEN_WHILE);
                            }
                        }
                    }
                }
            }
        }
    }

    return TOKEN_IDENTIFIER;
}

static Token identifier(Scanner* scanner)
{
    while (is_alpha_num(peek(scanner))) {
        advance(scanner);
    }

    TokenType type = identifier_type(scanner);
    if (interpolating_identifier(scanner)) {
        if (type != TOKEN_IDENTIFIER) {
            return error_token("Expected an identifier in string interpolation.", scanner->line);
        }

        end_identifier_interpolation(scanner);
    }

    return make_token(scanner, type);
}

static Token left_brace(Scanner* scanner)
{
    if (interpolating(scanner)) {
        open_brace(scanner);
    }

    return make_token(scanner, TOKEN_L_BRACE);
}

static Token right_brace(Scanner* scanner)
{
    if (interpolating(scanner)) {
        close_brace(scanner);

        if (all_braces_matched(scanner)) {
            leave_interpolation(scanner);
            move_start(scanner);
            return string(scanner);
        }
    }

    return make_token(scanner, TOKEN_R_BRACE);
}

Token scanner_scan_token(Scanner* scanner)
{
    Token whitespace = skip_whitespace(scanner);
    if (whitespace.type == TOKEN_ERROR) {
        return whitespace;
    }

    move_start(scanner);
    if (reached_end(scanner)) {
        return make_token(scanner, TOKEN_EOF);
    }

    if (interpolated_identifier(scanner)) {
        leave_interpolation(scanner);
        return string(scanner);
    }

    char c = advance(scanner);
    switch (c) {
        case '(': return make_token(scanner, TOKEN_L_PAREN);
        case ')': return make_token(scanner, TOKEN_R_PAREN);
        case '{': return left_brace(scanner);
        case '}': return right_brace(scanner);
        case '[': return make_token(scanner, TOKEN_L_BRACKET);
        case ']': return make_token(scanner, TOKEN_R_BRACKET);
        case ';': return make_token(scanner, TOKEN_SEMICOLON);
        case ',': return make_token(scanner, TOKEN_COMMA);
        case '.': return make_token(scanner, TOKEN_DOT);
        case '?': {
            switch (peek(scanner)) {
                case '.': advance(scanner); return make_token(scanner, TOKEN_QUESTION_DOT);
                case ':': advance(scanner); return make_token(scanner, TOKEN_QUESTION_COLON);
                case '[': advance(scanner); return make_token(scanner, TOKEN_QUESTION_L_BRACKET);
            }
            return make_token(scanner, TOKEN_QUESTION);
        }
        case ':': return make_token(scanner, TOKEN_COLON);
        case '\\': return make_token(scanner, TOKEN_BACKSLASH);
        case '-': {
            switch (peek(scanner)) {
                case '=': advance(scanner); return make_token(scanner, TOKEN_MINUS_EQUAL);
                case '>': advance(scanner); return make_token(scanner, TOKEN_R_ARROW);
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
                case '*': {
                    advance(scanner);
                    switch (peek(scanner)) {
                        case '=': advance(scanner); return make_token(scanner, TOKEN_DOUBLE_STAR_EQUAL);
                    }
                    return make_token(scanner, TOKEN_DOUBLE_STAR);
                }
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
        case '"': {
            move_start(scanner);
            return string(scanner);
        }
    }

    if (is_alpha(c)) {
        return identifier(scanner);
    }

    if (is_digit(c)) {
        return number(scanner);
    }

    return error_token("Unexpected character.", scanner->line);
}
