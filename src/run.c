#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "run.h"
#include "errcode.h"
#include "parser.h"
#include "vm.h"

static char* read_file(const char* fileName)
{
    FILE* file = fopen(fileName, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file '%s'.\n", fileName);
        exit(ERR_IO);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read '%s'.\n", fileName);
        exit(ERR_IO);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file '%s'.\n", fileName);
        exit(ERR_IO);
    }

    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

void run_file(const char* fileName)
{
    VM vm;
    vm_init(&vm);

    char* source = read_file(fileName);
    InterpretStatus status = vm_interpret(&vm, source);
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

        vm_interpret(&vm, line);
    }

    vm_free(&vm);
}
