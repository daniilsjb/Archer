#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filereader.h"
#include "errcode.h"

char* Reader_ReadFile(const char* fileName)
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
