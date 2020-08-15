#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "vm.h"
#include "errcode.h"

static void run_file(VM* vm, const char* fileName);
char* read_file(const char* fileName);
static void run_prompt(VM* vm);

int main(int argc, const char* argv[])
{
    VM vm;
    vm_init(&vm);

    if (argc > 2) {
        fprintf(stderr, "Usage: clox [script]\n");
        exit(ERR_USAGE);
    } else if (argc == 2) {
        run_file(&vm, argv[1]);
    } else {
        run_prompt(&vm);
    }

    vm_free(&vm);

    return 0;
}

static void run_file(VM* vm, const char* fileName)
{
    char* source = read_file(fileName);
    InterpretStatus status = vm_interpret(vm, source);
    free(source);

    if (status == INTERPRET_COMPILE_ERROR) {
        exit(ERR_DATA);
    }
    if (status == INTERPRET_RUNTIME_ERROR) {
        exit(ERR_SOFTWARE);
    }
}

char* read_file(const char* fileName)
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

static void run_prompt(VM* vm)
{
    char line[1024];
    while (true) {
        printf("> ");

        if (!fgets(line, 1024, stdin)) {
            printf("\n");
            break;
        }

        vm_interpret(vm, line);
    }
}