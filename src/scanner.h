#ifndef SCANNER_H
#define SCANNER_H

#include "token.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

void scanner_init(Scanner* scanner, char const* source);
Token scanner_scan_token(Scanner* scanner);

#endif
