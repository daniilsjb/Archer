#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "vm.h"
#include "file_reader.h"

static void run_file(const char* fileName);
static void run_prompt();

int main(int argc, const char* argv[])
{
    if (argc > 2) {
        fprintf(stderr, "Usage: archer [script]\n");
        exit(ERR_USAGE);
    } else if (argc == 2) {
        run_file(argv[1]);
    } else {
        run_prompt();
    }

    return 0;
}

void run_file(const char* fileName)
{
    VM vm;
    Vm_Init(&vm);

    char* source = Reader_ReadFile(fileName);
    InterpretStatus status = Vm_Interpret(&vm, source, fileName);
    free(source);

    if (status == INTERPRET_COMPILE_ERROR) {
        exit(ERR_DATA);
    }
    if (status == INTERPRET_RUNTIME_ERROR) {
        exit(ERR_SOFTWARE);
    }

    Vm_Free(&vm);
}

void run_prompt()
{
    VM vm;
    Vm_Init(&vm);

    char line[1024];
    while (true) {
        printf("> ");

        if (!fgets(line, 1024, stdin)) {
            printf("\n");
            break;
        }

        Vm_Interpret(&vm, line, "main.archer");
    }

    Vm_Free(&vm);
}
