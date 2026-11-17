#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "files.h"

BinaryFileContents readBinFromFile(const char* fileName) {
    struct stat info;
    BinaryFileContents binFileContent;
    FILE* fptr = fopen(fileName, "rb");
    
    if (stat(fileName, &info) != 0) {
        perror("Failed to read file stats");
        return binFileContent;
    }
    if (fptr == NULL) {
        perror("Failed to open file");
        return binFileContent;
    }
    
    char* content = malloc(info.st_size);
    if (content == NULL) {
        perror("Failed to allocate memory for file content");
        fclose(fptr);
        return binFileContent;
    }

    size_t bytesRead = fread(content, 1, info.st_size, fptr);
    fclose(fptr);

    if (bytesRead < sizeof(uint8_t)) {
        fprintf(stderr, "Error: File is too small to contain 32-bit integers.\n");
        free(content);
        return binFileContent;
    }
    size_t num_ints = bytesRead / sizeof(uint8_t);
    uint8_t* values = malloc(num_ints * sizeof(uint8_t));
    binFileContent.size = num_ints;
    binFileContent.content = values;
    
    if (values == NULL) {
        perror("Failed to allocate memory for integer array");
        free(content);
        return binFileContent;
    }

    // Safely copy and unpack the raw bytes into the integer array
    for (size_t i = 0; i < num_ints; i++) {
        memcpy(&values[i], content + (i * sizeof(uint8_t)), sizeof(uint8_t));
    }

    // Free the raw file buffer since we no longer need it
    free(content);

    // Return the valid pointer to the copied integer data
    return binFileContent;
}