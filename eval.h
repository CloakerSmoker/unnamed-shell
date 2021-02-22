#ifndef MAL_EVAL_H
#define MAL_EVAL_H

#include <stdint.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include "common.h"
#include "reader.h"
#include "printer.h"
#include "io.h"

typedef struct TagSymbolEntry {
	int Hash;

	Value* Value;

	struct TagSymbolEntry* Next;
} SymbolEntry;

typedef struct {
	SymbolEntry** Elements;
	size_t ElementCapacity;
} SymbolMap;

typedef struct TagEnvironment {
	struct TagEnvironment* Outer;
	SymbolMap* Symbols;
} Environment;

SymbolMap* SymbolMap_New();
void SymbolMap_Set(SymbolMap*, char*, size_t, void*);

Environment* Eval_Setup();
Environment* Environment_New_Bindings(Environment*, Value*, Value*);
void Environment_ReleaseAndFree(Environment*);
Value* Eval_Apply(Environment*, Value*);
Value* Eval_GetParameterRaw(Value*, ValueType, int);
Value* Eval_CallFunction(Environment*, Value*);

#define Eval_GetParameter(List, Type, Index) Eval_GetParameterRaw((List), (Type), (Index) + 1)
#define Eval_GetParameterReference(List, Type, Index) Value_AddReference(Eval_GetParameter((List), (Type), (Index)))
#define Eval_GetParameterReferenceRaw(List, Type, Index) Value_AddReference(Eval_GetParameterRaw((List), (Type), (Index)))

#endif //MAL_EVAL_H
