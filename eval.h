#ifndef MAL_EVAL_H
#define MAL_EVAL_H

#include <stdint.h>
#include "common.h"
#include "reader.h"

typedef enum {
    SYMBOL_VALUE,
    SYMBOL_FUNCTION
} SymbolType;

typedef struct TagSymbolEntry {
    int Hash;

    SymbolType Type;

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

#endif //MAL_EVAL_H
