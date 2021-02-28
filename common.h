#ifndef LISHP_COMMON_H
#define LISHP_COMMON_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "io.h"

#define alloc(Size) calloc(Size, 1)


extern jmp_buf OnError;

#define unused __unused

#define DEBUG_EVAL 0

#if DEBUG_EVAL

#define DEBUG_SYMBOLS 1
#define DEBUG_REFCOUNTS 1

extern int EvalDebugDepth;
extern int EvalNextValueID;

#define EVAL_DEBUG_PRELUDE EvalDebugDepth++
#define EVAL_DEBUG_EPILOG EvalDebugDepth--
#define EVAL_DEBUG_PRINT_PREFIX for (int I = 0; I < EvalDebugDepth; I++) {printf("    ");}

#endif

typedef enum {
	STRING_SELF_ALLOCATED,
	STRING_BORROWING_MEMORY
} StringAllocationMethod;

typedef struct {
	size_t Length;
	char* Buffer;
	StringAllocationMethod AllocationMethod;
} String;

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

void ContextAlert(ErrorContext* Context, char* Message, char Color);
void ContextError(ErrorContext* Context, char* Message);

ErrorContext* RawContextClone(ErrorContext* To, ErrorContext* From);
ErrorContext* RawContextMerge(ErrorContext* Left, ErrorContext* Right);

#define CloneContext(Left, Right) (RawContextClone(&(Left)->Context, &(Right)->Context))
#define MergeContext(Left, Right) (RawContextMerge(&(Left)->Context, &(Right)->Context))
#define Error(Left, Right) (ContextError(&Left->Context, Right))

struct TagValue;
struct TagEnvironment;

typedef struct {
	size_t Length;
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
	char IsMacro;
	char IsVariadic;
} Function;

typedef enum {
	VALUE_TYPE_LIST,
	VALUE_TYPE_INTEGER,
	VALUE_TYPE_STRING,
	VALUE_TYPE_IDENTIFIER,
	VALUE_TYPE_FUNCTION,
	VALUE_TYPE_NIL,
	VALUE_TYPE_BOOL,
	VALUE_TYPE_CHILD,
	VALUE_TYPE_ANY /// Fake value type used to mark that a value of any type is acceptable
} ValueType;

typedef struct TagValue {
	ErrorContext Context;
	ValueType Type;

	union {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
		void* RawValue;
#pragma clang diagnostic pop
		List* ListValue;
		int64_t IntegerValue;
		String* StringValue;
		String* IdentifierValue;
		Function* FunctionValue;
		ChildProcess* ChildValue;
		char BoolValue;
	};

	int ReferenceCount;

#if DEBUG_EVAL
	int ID;
#endif
} Value;

String* MakeString(char* Text, size_t Length);
String* BorrowString(char* Text, size_t Length);
String* AdoptString(char* Text, size_t Length);
String* CloneString(String* Target);
void FreeString(String* Target);
void PrintString(String* Target);

List* NewList(size_t Length);
void ExtendList(List* Target, size_t ElementCount);
void FreeList(List* Target);

Value* AddReferenceToValue(Value* Target);
int ReleaseReferenceToValue(Value* Target);
Value* NewPointerValue(ValueType Type, void* RawValue);
Value* NewIntegerValues(ValueType Type, int64_t RawValue);
Value* CloneValue(Value* Target);

#define NewValue(Type, Value) _Generic((Value), \
										List*: NewPointerValue,  \
										String*: NewPointerValue, \
										Function*: NewPointerValue, \
										ChildProcess*: NewPointerValue, \
										int64_t: NewIntegerValues, \
										long long: NewIntegerValues, \
										int: NewIntegerValues, \
										size_t: NewIntegerValues, \
										char: NewIntegerValues)(Type, Value)  \

#define NilValue() (NewValue(VALUE_TYPE_NIL, 0))

#endif //LISHP_COMMON_H
