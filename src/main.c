#include <stdio.h>
#include <stdlib.h>

#include "run.h"
#include "errcode.h"

int main(int argc, const char* argv[])
{
    if (argc > 2) {
        fprintf(stderr, "Usage: clox [script]\n");
        exit(ERR_USAGE);
    } else if (argc == 2) {
        run_file(argv[1]);
    } else {
        run_prompt();
    }

    return 0;
}
