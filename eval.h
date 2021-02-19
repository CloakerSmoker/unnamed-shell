#ifndef MAL_EVAL_H
#define MAL_EVAL_H

#include <stdint.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include "common.h"
#include "reader.h"
#include "printer.h"

typedef struct TagSymbolEntry {
    int Hash;

    union {
        Value* Value;
        Value* (*Function)(Value*);
        void* VoidValue;
    };

    struct TagSymbolEntry* Next;
} SymbolEntry;

typedef struct {
    SymbolEntry** Elements;
    int ElementCapacity;
} SymbolMap;

typedef struct TagEnvironment {
    struct TagEnvironment* Outer;
    SymbolMap* Symbols;
} Environment;

Environment* Eval_Setup();
Value* Eval_Apply(Environment*, Value*);
Value* Eval_GetParameterRaw(Value*, ValueType, int);

#endif //MAL_EVAL_H
