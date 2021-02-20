#ifndef MAL_COMMON_H
#define MAL_COMMON_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "io.h"

extern jmp_buf OnError;

#define EXTRA_ADDITIONS 1

int FileDescriptorSize(int);

typedef struct {
    int Length;
    char* Buffer;
} String;

String* String_New(char*, int);
void String_Print(String*);

typedef struct {
    char* Source;
    char* SourceFilePath;
    int Position;
    int Length;
    int LineNumber;
} ErrorContext;

#define BLACK 0
#define RED 1
#define GREEN 2
#define YELLOW 3
#define BLUE 4
#define MAGENTA 5
#define CYAN 6
#define WHITE 7
#define BRIGHT 8

void Context_Alert(ErrorContext*, char*, char);
void Context_Error(ErrorContext*, char*);

#define CloneContext(Left, Right) (Context_Clone(&Left->Context, &Right->Context));
#define MergeContext(Left, Right) (Context_Merge(&Left->Context, &Right->Context));
#define Error(Left, Right) (Context_Error(&Left->Context, Right))

ErrorContext* Context_Clone(ErrorContext*, ErrorContext*);
ErrorContext* Context_Merge(ErrorContext*, ErrorContext*);

#define alloc(Size) calloc(Size, 1)

struct TagValue;
struct TagEnvironment;

typedef struct {
    int Length;
    struct TagValue** Values;
} List;

typedef struct {
    union {
        struct TagValue* ParameterBindings;
        struct TagValue* (*NativeValue)(struct TagValue*);
    };

    struct TagValue* Body;
    struct TagEnvironment* Environment;
    char IsNativeFunction;
} Function;

typedef enum {
    VALUE_LIST,
    VALUE_INTEGER,
    VALUE_STRING,
    VALUE_IDENTIFIER,
    VALUE_FUNCTION,
    VALUE_NIL,
    VALUE_BOOL,
    VALUE_CHILD,
    VALUE_ANY
} ValueType;

typedef struct TagValue {
    ErrorContext Context;
    ValueType Type;

    union {
        List* ListValue;
        int64_t IntegerValue;
        String* StringValue;
        String* IdentifierValue;
        Function* FunctionValue;
        ChildProcess* ChildValue;
        char BoolValue;
    };
} Value;

#endif //MAL_COMMON_H
