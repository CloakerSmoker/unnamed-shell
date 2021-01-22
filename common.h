#ifndef MAL_COMMON_H
#define MAL_COMMON_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    int Length;
    char* Buffer;
} String;

String* String_New(char*, int);
void String_Print(String*);

#endif //MAL_COMMON_H
