#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "vm.h"
#include "file_reader.h"

void run_file(const char* fileName);
void run_prompt();

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
    vm_init(&vm);

    char* source = Reader_ReadFile(fileName);
    InterpretStatus status = vm_interpret(&vm, source, fileName);
    free(source);

    if (status == INTERPRET_COMPILE_ERROR) {
        exit(ERR_DATA);
    }
    if (status == INTERPRET_RUNTIME_ERROR) {
        exit(ERR_SOFTWARE);
    }

    vm_free(&vm);
}

void run_prompt()
{
    VM vm;
    vm_init(&vm);

    char line[1024];
    while (true) {
        printf("> ");

        if (!fgets(line, 1024, stdin)) {
            printf("\n");
            break;
        }

        vm_interpret(&vm, line, "main.archer");
    }

    vm_free(&vm);
}
