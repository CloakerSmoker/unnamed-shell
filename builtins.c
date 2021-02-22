
#include "builtins.h"

#define Eval_Binary_Int(Name, Operator) Value* Eval_ ## Name(Value* Parameters) { \
    Value* Result = Value_New(VALUE_INTEGER, 0);                                             \
    Result->IntegerValue = Eval_GetParameter(Parameters, VALUE_INTEGER, 0)->IntegerValue Operator Eval_GetParameter(Parameters, VALUE_INTEGER, 1)->IntegerValue; \
    for (int Index = 3; Index < Parameters->ListValue->Length; Index++) {         \
        Result->IntegerValue Operator ## = Eval_GetParameter(Parameters, VALUE_INTEGER, Index - 1)->IntegerValue; \
    }                                                                              \
    return Result;                                                                    \
}

Eval_Binary_Int(Add, +)

Eval_Binary_Int(Sub, -)

Eval_Binary_Int(Mul, *)

Eval_Binary_Int(Div, /)

#define EVAL_FUNCTION __unused

EVAL_FUNCTION Value* Eval_Quit(Value* Parameters) {
	int64_t ExitCode = 0;

	if (Parameters->ListValue->Length >= 2) {
		ExitCode = Eval_GetParameter(Parameters, VALUE_INTEGER, 0)->IntegerValue;
	}

	exit((int)ExitCode);
}

EVAL_FUNCTION Value* Eval_ListFiles(unused Value* Parameters) {
	DIR* CurrentDirectory = opendir(".");

	if (CurrentDirectory) {
		size_t EntryCount = 0;

		while (readdir(CurrentDirectory) != NULL) {
			EntryCount += 1;
		}

		seekdir(CurrentDirectory, 0);

		struct dirent* NextDirectoryEntry;
		Value** Values = calloc(sizeof(Value*), EntryCount);
		int Index = 0;

		while ((NextDirectoryEntry = readdir(CurrentDirectory)) != NULL) {
			size_t NameLength = strlen(NextDirectoryEntry->d_name);

			char* NameString = alloc(NameLength + 2);
			memcpy(NameString, NextDirectoryEntry->d_name, NameLength);

			if (NextDirectoryEntry->d_type == DT_DIR) {
				NameString[NameLength++] = '/';
			}

			Values[Index++] = Value_New(VALUE_STRING, String_Adopt(NameString, NameLength));
		}

		closedir(CurrentDirectory);

		List* ListResult = List_New(EntryCount);

		ListResult->Values = Values;

		return Value_New(VALUE_LIST, ListResult);
	}

	return Value_Nil();
}

EVAL_FUNCTION Value* Eval_Print(Value* Parameters) {
	List* ParameterList = Parameters->ListValue;

	for (int Index = 1; Index < ParameterList->Length; Index++) {
		Value* NextValue = ParameterList->Values[Index];

		if (NextValue->Type == VALUE_STRING) {
			NextValue->Type = VALUE_IDENTIFIER;
			Value_Print(NextValue);
			NextValue->Type = VALUE_STRING;
		}
		else {
			Value_Print(NextValue);
		}

		if (Index + 1 != ParameterList->Length) {
			putchar(' ');
		}
	}

	return Value_Nil();
}

EVAL_FUNCTION Value* Eval_ChangeDirectory(Value* Parameters) {
	String* TargetDirectory = Eval_GetParameter(Parameters, VALUE_STRING, 0)->StringValue;

	int ErrorCode = chdir(TargetDirectory->Buffer);

	return Value_New(VALUE_INTEGER, ErrorCode);
}

EVAL_FUNCTION Value* Eval_GetCurrentDirectory(unused Value* Parameters) {
	char* Buffer = getcwd(NULL, 0);

	return Value_New(VALUE_STRING, String_Adopt(Buffer, strlen(Buffer)));
}

EVAL_FUNCTION Value* Eval_ListMake(Value* Parameters) {
	size_t Length = Parameters->ListValue->Length - 1;

	List* ResultList = List_New(Length);

	for (int Index = 0; Index < Length; Index++) {
		ResultList->Values[Index] = Value_AddReference(Parameters->ListValue->Values[Index + 1]);
	}

	return Value_New(VALUE_LIST, ResultList);
}

EVAL_FUNCTION Value* Eval_ListLength(Value* Parameters) {
	Value* TargetValue = Eval_GetParameter(Parameters, VALUE_LIST, 0);

	return Value_New(VALUE_INTEGER, TargetValue->ListValue->Length);
}

EVAL_FUNCTION Value* Eval_ListIndex(Value* Parameters) {
	List* TargetList = Eval_GetParameter(Parameters, VALUE_LIST, 0)->ListValue;
	int64_t TargetIndex = Eval_GetParameter(Parameters, VALUE_INTEGER, 1)->IntegerValue;

	if (TargetIndex >= TargetList->Length || TargetIndex < 0) {
		return Value_Nil();
	}

	return Value_AddReference(TargetList->Values[TargetIndex]);
}

EVAL_FUNCTION Value* Eval_ListMap(Value* Parameters) {
	List* Elements = Eval_GetParameter(Parameters, VALUE_LIST, 0)->ListValue;
	Value* MapFunction = Eval_GetParameterReference(Parameters, VALUE_FUNCTION, 1);

	List* ResultList = List_New(Elements->Length);
	List* CallList = List_New(2);

	CallList->Values[0] = MapFunction;
	CallList->Values[1] = Value_Nil();

	Value* CallValue = Value_New(VALUE_LIST, CallList);

	for (int Index = 0; Index < Elements->Length; Index++) {
		Value* NextValue = Value_AddReference(Elements->Values[Index]);

		Value_Release(CallList->Values[1]);
		CallList->Values[1] = NextValue;

		Value* NewValue = Eval_CallFunction(NULL, Value_AddReference(CallValue));

		ResultList->Values[Index] = NewValue;
	}

	Value_Release(CallValue);

	return Value_New(VALUE_LIST, ResultList);
}

EVAL_FUNCTION Value* Eval_ListPush(Value* Parameters) {
	Value* TargetListValue = Eval_GetParameter(Parameters, VALUE_LIST, 0);
	List* TargetList = TargetListValue->ListValue;

	size_t AdditionalElementCount = Parameters->ListValue->Length - 2;
	size_t OldElementCount = TargetList->Length;
	size_t NewElementCount = OldElementCount + AdditionalElementCount;

	List* ResultList = List_New(NewElementCount);

	for (int Index = 0; Index < OldElementCount; Index++) {
		Value* NextValue = TargetList->Values[Index];

		ResultList->Values[Index] = Value_AddReference(NextValue);
	}

	for (int Index = 0; Index < AdditionalElementCount; Index++) {
		Value* NextValue = Parameters->ListValue->Values[Index + 2];

		ResultList->Values[OldElementCount + Index] = Value_AddReference(NextValue);
	}

	Value_Release(TargetListValue);

	return Value_New(VALUE_LIST, ResultList);
}

EVAL_FUNCTION Value* Eval_StringLength(Value* Parameters) {
	String* TargetString = Eval_GetParameter(Parameters, VALUE_STRING, 0)->StringValue;

	return Value_New(VALUE_INTEGER, TargetString->Length);
}

EVAL_FUNCTION Value* Eval_StringSplit(Value* Parameters) {
	String* TargetString = Eval_GetParameter(Parameters, VALUE_STRING, 0)->StringValue;

	List* ResultList = List_New(TargetString->Length);

	for (int Index = 0; Index < TargetString->Length; Index++) {
		ResultList->Values[Index] = Value_New(VALUE_STRING, String_New(&TargetString->Buffer[Index], 1));
	}

	return Value_New(VALUE_LIST, ResultList);
}

int Value_Equals(Value* Left, Value* Right) {
	if (Left->Type != Right->Type) {
		return 0;
	}

	switch (Left->Type) {
		case VALUE_BOOL:
		case VALUE_FUNCTION:
		case VALUE_CHILD:
		case VALUE_INTEGER:
			return Left->IntegerValue == Right->IntegerValue;
		case VALUE_NIL:
			return 1;
		case VALUE_LIST:
			if (Left->ListValue->Length != Right->ListValue->Length) {
				return 0;
			}

			for (int Index = 0; Index < Left->ListValue->Length; Index++) {
				if (!Value_Equals(Left->ListValue->Values[Index], Right->ListValue->Values[Index])) {
					return 0;
				}
			}

			return 1;
		case VALUE_IDENTIFIER:
		case VALUE_STRING:
			if (Left->StringValue->Length != Right->StringValue->Length) {
				return 0;
			}

			return !strncmp(Left->StringValue->Buffer, Right->StringValue->Buffer, Left->StringValue->Length);
		case VALUE_ANY:
			break;
	}

	return 0;
}

EVAL_FUNCTION Value* Eval_Equals(Value* Parameters) {
	Value* FirstValue = Eval_GetParameter(Parameters, VALUE_ANY, 0);

// Ensure we've got at least two parameters
	Eval_GetParameter(Parameters, VALUE_ANY, 1);

	int ResultValue = 1;

	for (int Index = 2; Index < Parameters->ListValue->Length; Index++) {
		Value* NextValue = Parameters->ListValue->Values[Index];

		if (!Value_Equals(FirstValue, NextValue)) {
			ResultValue = 0;
			break;
		}
	}

	return Value_New(VALUE_BOOL, ResultValue);
}

EVAL_FUNCTION Value* Eval_CreateProcess(Value* Parameters) {
	String* Path = Eval_GetParameter(Parameters, VALUE_STRING, 0)->StringValue;

	char* Arguments[2] = {Path->Buffer, 0};

	return Value_New(VALUE_CHILD, ChildProcess_New(Path->Buffer, Arguments));
}

EVAL_FUNCTION Value* Eval_ReadProcessOutput(Value* Parameters) {
	ChildProcess* Child = Eval_GetParameter(Parameters, VALUE_CHILD, 0)->ChildValue;

	size_t OutputSize = 0;
	char* Output = ChildProcess_ReadStream(Child, STDOUT_FILENO, &OutputSize);

	return Value_New(VALUE_STRING, String_Adopt(Output, OutputSize));
}

EVAL_FUNCTION Value* Eval_GetReferenceCount(Value* Parameters) {
	Value* Parameter = Eval_GetParameter(Parameters, VALUE_ANY, 0);

	return Value_New(VALUE_INTEGER, Parameter->ReferenceCount);
}

EVAL_FUNCTION Value* Eval_Eval(Value* Parameters) {
	Value* AST = Eval_GetParameterReference(Parameters, VALUE_ANY, 0);

	Environment* Env = Eval_Setup();

	Value* Result = Eval_Apply(Env, AST);

	Environment_ReleaseAndFree(Env);

	return Result;
}
EVAL_FUNCTION Value* Eval_Parse(Value* Parameters) {
	Value* Input = Eval_GetParameter(Parameters, VALUE_STRING, 0);

	char* ClonedInput = strndup(Input->StringValue->Buffer, Input->StringValue->Length);

	Tokenizer* InputTokenizer = Tokenizer_New("*", ClonedInput, Input->StringValue->Length);

	Value* Tree = ReadForm(InputTokenizer);

	return Tree;
}

EVAL_FUNCTION Value* Eval_Slurp(Value* Parameters) {
	Value* FileNameValue = Eval_GetParameter(Parameters, VALUE_STRING, 0);
	String* FileName = FileNameValue->StringValue;

	FILE* FileDescriptor = fopen(FileName->Buffer, "r");

	if (FileDescriptor == NULL) {
		char MessageBuffer[100] = {0};

		snprintf(MessageBuffer, 100, "Could not open file '%s'", FileName->Buffer);

		Error(FileNameValue, MessageBuffer);
		longjmp(OnError, 0);
	}

	fseek(FileDescriptor, 0, SEEK_END);

	long Length = ftell(FileDescriptor);

	fseek(FileDescriptor, 0, SEEK_SET);

	char* Buffer = alloc(Length + 1);

	fread(Buffer, 1, Length, FileDescriptor);

	return Value_New(VALUE_STRING, String_Adopt(Buffer, strlen(Buffer)));
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-sizeof-expression"

Environment* Eval_Setup() {
	SymbolMap* Symbols = SymbolMap_New();

#define AddSymbolFunction(Name, LispName)      \
do {                                     \
Function* RawFunction = alloc(sizeof(Function));   \
RawFunction->IsNativeFunction = 1;                 \
RawFunction->NativeValue = Eval_ ## Name;      \
Value* FunctionValue = Value_New(VALUE_FUNCTION, RawFunction);    \
SymbolMap_Set(Symbols, #LispName, strlen(#LispName), FunctionValue);} while (0)

	AddSymbolFunction(Add, +);
	AddSymbolFunction(Sub, -);
	AddSymbolFunction(Mul, *);
	AddSymbolFunction(Div, /);
	AddSymbolFunction(Quit, quit);
	AddSymbolFunction(ListFiles, ls);
	AddSymbolFunction(Print, print);
	AddSymbolFunction(ChangeDirectory, cd);
	AddSymbolFunction(GetCurrentDirectory, pwd);
	AddSymbolFunction(ListMake, list.make);
	AddSymbolFunction(ListLength, list.length);
	AddSymbolFunction(ListIndex, list.index);
	AddSymbolFunction(ListMap, list.map);
	AddSymbolFunction(ListPush, list.push);
	AddSymbolFunction(StringLength, string.length);
	AddSymbolFunction(StringSplit, string.split);
	AddSymbolFunction(Equals, =);
	AddSymbolFunction(CreateProcess, process.make);
	AddSymbolFunction(ReadProcessOutput, process.read_output);
	AddSymbolFunction(GetReferenceCount, debug.grc);
	AddSymbolFunction(Eval, eval);
	AddSymbolFunction(Parse, parse);
	AddSymbolFunction(Slurp, slurp);

	Environment* Result = alloc(sizeof(Environment));

	Result->Outer = NULL;
	Result->Symbols = Symbols;

	return Result;
}

#pragma clang diagnostic pop
