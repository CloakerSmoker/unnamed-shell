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

#define DEBUG_EVAL 1
#define DEBUG_SYMBOLS 1
#define DEBUG_REFCOUNTS 1

#if DEBUG_EVAL

extern int EvalDebugDepth;

#define EVAL_DEBUG_PRELUDE EvalDebugDepth++
#define EVAL_DEBUG_EPILOG EvalDebugDepth--
#define EVAL_DEBUG_PRINT_PREFIX for (int I = 0; I < EvalDebugDepth; I++) {printf("    ");}

#endif

int FileDescriptorSize(int);

typedef enum {
	STRING_SELF_ALLOCATED,
	STRING_BORROWING_MEMORY
} StringAllocationMethod;

typedef struct {
	int Length;
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

void Context_Alert(ErrorContext*, char*, char);
void Context_Error(ErrorContext*, char*);

#define CloneContext(Left, Right) (Context_Clone(&Left->Context, &Right->Context))
#define MergeContext(Left, Right) (Context_Merge(&Left->Context, &Right->Context))
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
		void* RawValue;
		List* ListValue;
		int64_t IntegerValue;
		String* StringValue;
		String* IdentifierValue;
		Function* FunctionValue;
		ChildProcess* ChildValue;
		char BoolValue;
	};

	int ReferenceCount;
} Value;

String* String_New(char*, int);
String* String_Adopt(char*, int);
String* String_Borrow(char*, int);
String* String_Clone(String*);
void String_Free(String*);
void String_Print(String*);

List* List_New(int);
void List_Free(List*);

Value* Value_AddReference(Value*);
int Value_Release(Value*);
Value* Value_New_Pointer(ValueType, void*);
Value* Value_New_Integer(ValueType, int64_t);
Value* Value_Clone(Value*);

#define Value_New(Type, Value) _Generic((Value), \
										List*: Value_New_Pointer,  \
										String*: Value_New_Pointer, \
										Function*: Value_New_Pointer, \
										ChildProcess*: Value_New_Pointer, \
										int64_t: Value_New_Integer, \
										long long: Value_New_Integer, \
										int: Value_New_Integer, \
										char: Value_New_Integer)(Type, Value)  \

#define Value_Nil() (Value_New(VALUE_NIL, 0))

#endif //MAL_COMMON_H
