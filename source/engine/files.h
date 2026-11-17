#ifndef FILES_H
#define FILES_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    uint8_t* content;
    size_t size;
} BinaryFileContents;

BinaryFileContents readBinFromFile(const char* fileName);

#endif