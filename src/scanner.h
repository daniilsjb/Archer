#ifndef SCANNER_H
#define SCANNER_H

#include "token.h"

#define MAX_INTERPOLATION_DEPTH 8

typedef struct {
    const char* start;
    const char* current;
    int line;

    int unmatchedInterpolations[MAX_INTERPOLATION_DEPTH];
    char interpolationDepth;
} Scanner;

void scanner_init(Scanner* scanner, char const* source);
void scanner_clear(Scanner* scanner);

Token scanner_scan_token(Scanner* scanner);

#endif
