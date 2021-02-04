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

void Scanner_Init(Scanner* scanner, char const* source);
void Scanner_Clear(Scanner* scanner);

Token Scanner_ScanToken(Scanner* scanner);

#endif
