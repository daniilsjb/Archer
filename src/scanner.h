#ifndef SCANNER_H
#define SCANNER_H

#include "token.h"

void scanner_init(char const* source);
Token scan_token();

#endif
