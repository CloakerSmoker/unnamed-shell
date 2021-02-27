#ifndef LISHP_EVAL_H
#define LISHP_EVAL_H

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

SymbolMap* NewSymbolMap();
void SetSymbolMapEntry(SymbolMap* this, char* Key, size_t KeyLength, void* Value);

Environment* SetupEnvironment();
Environment* NewEnvironmentWithBindings(Environment* Outer, Value* BindingNames, Value* BindingValues);
void DestroyEnvironment(Environment* Target);
Value* Evaluate(Environment* this, Value* Target);
Value* EvaluateFunctionCall(Environment* this, Value* Call);

Value* RawGetListIndex(Value* Target, ValueType ExpectedType, int Index);
#define GetListIndex(List, Type, Index) RawGetListIndex((List), (Type), (Index) + 1)
#define GetReferenceToListIndex(List, Type, Index) AddReferenceToValue(GetListIndex((List), (Type), (Index)))
#define RawGetReferenceToListIndex(List, Type, Index) AddReferenceToValue(RawGetListIndex((List), (Type), (Index)))

#endif //LISHP_EVAL_H
