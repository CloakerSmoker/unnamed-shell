
#include "builtins.h"

#define Eval_Binary_Int(Name, Operator) Value* Eval_ ## Name(Value* Parameters) { \
    Value* Result = NewValue(VALUE_TYPE_INTEGER, 0);                                             \
    Result->IntegerValue = GetListIndex(Parameters, VALUE_TYPE_INTEGER, 0)->IntegerValue Operator GetListIndex(Parameters, VALUE_TYPE_INTEGER, 1)->IntegerValue; \
    for (int Index = 3; Index < Parameters->ListValue->Length; Index++) {         \
        Result->IntegerValue Operator ## = GetListIndex(Parameters, VALUE_TYPE_INTEGER, Index - 1)->IntegerValue; \
    }                                                                              \
    return Result;                                                                    \
}

Eval_Binary_Int(Add, +)

Eval_Binary_Int(Sub, -)

Eval_Binary_Int(Mul, *)

Eval_Binary_Int(Div, /)
Eval_Binary_Int(Mod, %)

#define EVAL_FUNCTION(Name) unused Value* Eval_ ## Name (Value* Parameters)

EVAL_FUNCTION(Quit) {
	int64_t ExitCode = 0;

	if (Parameters->ListValue->Length >= 2) {
		ExitCode = GetListIndex(Parameters, VALUE_TYPE_INTEGER, 0)->IntegerValue;
	}

	exit((int)ExitCode);
}

EVAL_FUNCTION(ListFiles) {
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

			Values[Index++] = NewValue(VALUE_TYPE_STRING, AdoptString(NameString, NameLength));
		}

		closedir(CurrentDirectory);

		List* ListResult = NewList(EntryCount);

		ListResult->Values = Values;

		return NewValue(VALUE_TYPE_LIST, ListResult);
	}

	return NilValue();
}

EVAL_FUNCTION(Print) {
	List* ParameterList = Parameters->ListValue;

	for (int Index = 1; Index < ParameterList->Length; Index++) {
		Value* NextValue = ParameterList->Values[Index];

		if (NextValue->Type == VALUE_TYPE_STRING) {
			NextValue->Type = VALUE_TYPE_IDENTIFIER;
			Value_Print(NextValue);
			NextValue->Type = VALUE_TYPE_STRING;
		}
		else {
			Value_Print(NextValue);
		}

		if (Index + 1 != ParameterList->Length) {
			putchar(' ');
		}
	}

	return NilValue();
}
EVAL_FUNCTION(PrintCharacter) {
	int64_t PrintValue = GetListIndex(Parameters, VALUE_TYPE_INTEGER, 0)->IntegerValue;

	printf("%c", (char)PrintValue);

	return NilValue();
}

EVAL_FUNCTION(ChangeDirectory) {
	String* TargetDirectory = GetListIndex(Parameters, VALUE_TYPE_STRING, 0)->StringValue;

	int ErrorCode = chdir(TargetDirectory->Buffer);

	return NewValue(VALUE_TYPE_INTEGER, ErrorCode);
}

EVAL_FUNCTION(GetCurrentDirectory) {
	char* Buffer = getcwd(NULL, 0);

	return NewValue(VALUE_TYPE_STRING, AdoptString(Buffer, strlen(Buffer)));
}

EVAL_FUNCTION(ListMake) {
	size_t Length = Parameters->ListValue->Length - 1;

	List* ResultList = NewList(Length);

	for (int Index = 0; Index < Length; Index++) {
		ResultList->Values[Index] = AddReferenceToValue(Parameters->ListValue->Values[Index + 1]);
	}

	return NewValue(VALUE_TYPE_LIST, ResultList);
}

EVAL_FUNCTION(ListLength) {
	Value* TargetValue = GetListIndex(Parameters, VALUE_TYPE_LIST, 0);

	return NewValue(VALUE_TYPE_INTEGER, TargetValue->ListValue->Length);
}

EVAL_FUNCTION(ListIndex) {
	List* TargetList = GetListIndex(Parameters, VALUE_TYPE_LIST, 0)->ListValue;
	int64_t TargetIndex = GetListIndex(Parameters, VALUE_TYPE_INTEGER, 1)->IntegerValue;

	if (TargetIndex >= TargetList->Length || TargetIndex < 0) {
		return NilValue();
	}

	return AddReferenceToValue(TargetList->Values[TargetIndex]);
}

EVAL_FUNCTION(ListMap) {
	List* Elements = GetListIndex(Parameters, VALUE_TYPE_LIST, 0)->ListValue;
	Value* MapFunction = GetReferenceToListIndex(Parameters, VALUE_TYPE_FUNCTION, 1);

	List* ResultList = NewList(Elements->Length);
	List* CallList = NewList(2);

	CallList->Values[0] = MapFunction;
	CallList->Values[1] = NilValue();

	Value* CallValue = NewValue(VALUE_TYPE_LIST, CallList);

	for (int Index = 0; Index < Elements->Length; Index++) {
		Value* NextValue = AddReferenceToValue(Elements->Values[Index]);

		ReleaseReferenceToValue(CallList->Values[1]);
		CallList->Values[1] = NextValue;

		Value* NewValue = EvaluateFunctionCall(NULL, AddReferenceToValue(CallValue));

		ResultList->Values[Index] = NewValue;
	}

	ReleaseReferenceToValue(CallValue);

	return NewValue(VALUE_TYPE_LIST, ResultList);
}

EVAL_FUNCTION(ListPush) {
	Value* TargetListValue = GetListIndex(Parameters, VALUE_TYPE_LIST, 0);
	List* TargetList = TargetListValue->ListValue;

	size_t AdditionalElementCount = Parameters->ListValue->Length - 2;
	size_t OldElementCount = TargetList->Length;
	size_t NewElementCount = OldElementCount + AdditionalElementCount;

	List* ResultList = NewList(NewElementCount);

	for (int Index = 0; Index < OldElementCount; Index++) {
		Value* NextValue = TargetList->Values[Index];

		ResultList->Values[Index] = AddReferenceToValue(NextValue);
	}

	for (int Index = 0; Index < AdditionalElementCount; Index++) {
		Value* NextValue = Parameters->ListValue->Values[Index + 2];

		ResultList->Values[OldElementCount + Index] = AddReferenceToValue(NextValue);
	}

	return NewValue(VALUE_TYPE_LIST, ResultList);
}
EVAL_FUNCTION(ListReplaceIndex) {
	Value* TargetList = GetListIndex(Parameters, VALUE_TYPE_LIST, 0);
	int64_t TargetIndex = GetListIndex(Parameters, VALUE_TYPE_INTEGER, 1)->IntegerValue;
	Value* NewValue = GetListIndex(Parameters, VALUE_TYPE_ANY, 2);

	if (TargetList->ListValue->Length < TargetIndex) {
		return NilValue();
	}

	Value* Result = CloneValue(TargetList);

	ReleaseReferenceToValue(Result->ListValue->Values[TargetIndex]);

	Result->ListValue->Values[TargetIndex] = AddReferenceToValue(NewValue);

	return Result;
}

EVAL_FUNCTION(StringLength) {
	String* TargetString = GetListIndex(Parameters, VALUE_TYPE_STRING, 0)->StringValue;

	return NewValue(VALUE_TYPE_INTEGER, TargetString->Length);
}

EVAL_FUNCTION(StringSplit) {
	String* TargetString = GetListIndex(Parameters, VALUE_TYPE_STRING, 0)->StringValue;

	List* ResultList = NewList(TargetString->Length);

	for (int Index = 0; Index < TargetString->Length; Index++) {
		ResultList->Values[Index] = NewValue(VALUE_TYPE_STRING, MakeString(&TargetString->Buffer[Index], 1));
	}

	return NewValue(VALUE_TYPE_LIST, ResultList);
}

void RaiseTypeError(Value* Blame, ValueType ExpectedType);

EVAL_FUNCTION(StringConcatenate) {
	size_t NewLength = 0;

	for (int Index = 1; Index < Parameters->ListValue->Length; Index++) {
		Value* Next = RawGetListIndex(Parameters, VALUE_TYPE_ANY, Index);

		if (Next->Type != VALUE_TYPE_STRING && Next->Type != VALUE_TYPE_IDENTIFIER) {
			RaiseTypeError(Next, VALUE_TYPE_STRING);
		}

		NewLength += Next->StringValue->Length;
	}

	char* Buffer = alloc(NewLength + 1);
	int Offset = 0;

	for (int Index = 1; Index < Parameters->ListValue->Length; Index++) {
		Value* Next = RawGetListIndex(Parameters, VALUE_TYPE_ANY, Index);

		memcpy(Buffer + Offset, Next->StringValue->Buffer, Next->StringValue->Length);

		Offset += Next->StringValue->Length;
	}

	return NewValue(VALUE_TYPE_STRING, AdoptString(Buffer, NewLength));
}
EVAL_FUNCTION(StringSlice) {
	String* Target = GetListIndex(Parameters, VALUE_TYPE_STRING, 0)->StringValue;
	int Start = GetListIndex(Parameters, VALUE_TYPE_INTEGER, 1)->IntegerValue;

	int ActualLength = Target->Length;
	int Length = ActualLength;

	if (Parameters->ListValue->Length >= 4) {
		Length = GetListIndex(Parameters, VALUE_TYPE_INTEGER, 2)->IntegerValue;
	}

	if (Start > ActualLength) {
		Start = ActualLength;
	}

	if (Start + Length > ActualLength) {
		Length = ActualLength - Start;
	}

	char* Buffer = alloc(Length + 1);

	strncpy(Buffer, Target->Buffer + Start, Length);

	return NewValue(VALUE_TYPE_STRING, AdoptString(Buffer, Length));
}
EVAL_FUNCTION(StringSymbol) {
	Value* Target = GetListIndex(Parameters, VALUE_TYPE_STRING, 0);

	Value* Result = CloneValue(Target);

	Result->Type = VALUE_TYPE_IDENTIFIER;

	return Result;
}

int Value_Equals(Value* Left, Value* Right) {
	if (Left->Type != Right->Type) {
		return 0;
	}

	switch (Left->Type) {
		case VALUE_TYPE_BOOL:
		case VALUE_TYPE_FUNCTION:
		case VALUE_TYPE_CHILD:
		case VALUE_TYPE_INTEGER:
			return Left->IntegerValue == Right->IntegerValue;
		case VALUE_TYPE_NIL:
			return 1;
		case VALUE_TYPE_LIST:
			if (Left->ListValue->Length != Right->ListValue->Length) {
				return 0;
			}

			for (int Index = 0; Index < Left->ListValue->Length; Index++) {
				if (!Value_Equals(Left->ListValue->Values[Index], Right->ListValue->Values[Index])) {
					return 0;
				}
			}

			return 1;
		case VALUE_TYPE_IDENTIFIER:
		case VALUE_TYPE_STRING:
			if (Left->StringValue->Length != Right->StringValue->Length) {
				return 0;
			}

			return !strncmp(Left->StringValue->Buffer, Right->StringValue->Buffer, Left->StringValue->Length);
		case VALUE_TYPE_ANY:
			break;
	}

	return 0;
}

EVAL_FUNCTION(Equals) {
	Value* FirstValue = GetListIndex(Parameters, VALUE_TYPE_ANY, 0);

// Ensure we've got at least two parameters
	GetListIndex(Parameters, VALUE_TYPE_ANY, 1);

	int ResultValue = 1;

	for (int Index = 2; Index < Parameters->ListValue->Length; Index++) {
		Value* NextValue = Parameters->ListValue->Values[Index];

		if (!Value_Equals(FirstValue, NextValue)) {
			ResultValue = 0;
			break;
		}
	}

	return NewValue(VALUE_TYPE_BOOL, ResultValue);
}
EVAL_FUNCTION(Less) {
	int64_t Left = GetListIndex(Parameters, VALUE_TYPE_INTEGER, 0)->IntegerValue;
	int64_t Right = GetListIndex(Parameters, VALUE_TYPE_INTEGER, 1)->IntegerValue;

	return NewValue(VALUE_TYPE_BOOL, Left < Right);
}
EVAL_FUNCTION(Greater) {
	int64_t Left = GetListIndex(Parameters, VALUE_TYPE_INTEGER, 0)->IntegerValue;
	int64_t Right = GetListIndex(Parameters, VALUE_TYPE_INTEGER, 1)->IntegerValue;

	return NewValue(VALUE_TYPE_BOOL, Left > Right);
}

EVAL_FUNCTION(CreateProcess) {
	String* Path = GetListIndex(Parameters, VALUE_TYPE_STRING, 0)->StringValue;

	char* Arguments[2] = {Path->Buffer, 0};

	return NewValue(VALUE_TYPE_CHILD, NewChildProcess(Path->Buffer, Arguments));
}

EVAL_FUNCTION(ReadProcessOutput) {
	ChildProcess* Child = GetListIndex(Parameters, VALUE_TYPE_CHILD, 0)->ChildValue;

	size_t OutputSize = 0;
	char* Output = ReadFromChildProcessStream(Child, STDOUT_FILENO, &OutputSize);

	return NewValue(VALUE_TYPE_STRING, AdoptString(Output, OutputSize));
}

EVAL_FUNCTION(GetReferenceCount) {
	Value* Parameter = GetListIndex(Parameters, VALUE_TYPE_ANY, 0);

	return NewValue(VALUE_TYPE_INTEGER, Parameter->ReferenceCount);
}

EVAL_FUNCTION(ReadCharacter) {
	char* Result = alloc(1);

	*Result = getchar();

	return NewValue(VALUE_TYPE_STRING, AdoptString(Result, 1));
}

EVAL_FUNCTION(Eval) {
	Value* AST = GetReferenceToListIndex(Parameters, VALUE_TYPE_ANY, 0);

	Environment* Env = SetupEnvironment();

	Value* Result = Evaluate(Env, AST);

	DestroyEnvironment(Env);

	return Result;
}
EVAL_FUNCTION(Parse) {
	Value* Input = GetListIndex(Parameters, VALUE_TYPE_STRING, 0);

	char* ClonedInput = strndup(Input->StringValue->Buffer, Input->StringValue->Length);

	Tokenizer* InputTokenizer = NewTokenizer("*", ClonedInput, Input->StringValue->Length);

	Value* Tree = ReadForm(InputTokenizer);

	return Tree;
}

EVAL_FUNCTION(Slurp) {
	Value* FileNameValue = GetListIndex(Parameters, VALUE_TYPE_STRING, 0);
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

	return NewValue(VALUE_TYPE_STRING, AdoptString(Buffer, strlen(Buffer)));
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-sizeof-expression"

String* StringifyValue(Value* Target) {
	if (Target->Type == VALUE_TYPE_IDENTIFIER || Target->Type == VALUE_TYPE_STRING) {
		return CloneString(Target->IdentifierValue);
	}
	else if (Target->Type == VALUE_TYPE_INTEGER) {
		char* Buffer = alloc(40);

		snprintf(Buffer, 40, "%li", Target->IntegerValue);

		return AdoptString(Buffer, strlen(Buffer));
	}

	return NULL;
}

EVAL_FUNCTION(ToString) {
	Value* Target = GetListIndex(Parameters, VALUE_TYPE_ANY, 0);

	return NewValue(VALUE_TYPE_STRING, StringifyValue(Target));
}

EVAL_FUNCTION(OSEnvironmentGet) {
	String* KeyName = GetListIndex(Parameters, VALUE_TYPE_STRING, 0)->StringValue;

	char* RawResult = getenv(KeyName->Buffer);

	if (RawResult) {
		return NewValue(VALUE_TYPE_STRING, MakeString(RawResult, strlen(RawResult)));
	}
	else {
		return NilValue();
	}
}

EVAL_FUNCTION(GetASCIICode) {
	String* Target = GetListIndex(Parameters, VALUE_TYPE_STRING, 0)->StringValue;

	int Result = 0;

	if (Target->Length) {
		Result = Target->Buffer[0];
	}

	return NewValue(VALUE_TYPE_INTEGER, Result);
}
EVAL_FUNCTION(GetASCIICharacter) {
	int64_t Target = GetListIndex(Parameters, VALUE_TYPE_INTEGER, 0)->IntegerValue;

	char* Result = alloc(2);

	*Result = (char)Target;

	return NewValue(VALUE_TYPE_STRING, AdoptString(Result, 1));
}

EVAL_FUNCTION(Reload) {
	execv(Arguments[0], Arguments);
}
EVAL_FUNCTION(Time) {
	struct timespec Now;

	clock_gettime(CLOCK_REALTIME, &Now);

	int64_t Micros = Now.tv_sec * 1000000;

	Micros += Now.tv_nsec/1000;

	if (Now.tv_nsec % 1000 >= 500) {
		++Micros;
	}

	return NewValue(VALUE_TYPE_INTEGER, Micros);
}

Environment* SetupEnvironment() {
	SymbolMap* Symbols = NewSymbolMap();

#define AddSymbolFunction(Name, LispName)      \
do {                                     \
Function* RawFunction = alloc(sizeof(Function));   \
RawFunction->IsNativeFunction = 1;                 \
RawFunction->NativeValue = Eval_ ## Name;      \
Value* FunctionValue = NewValue(VALUE_TYPE_FUNCTION, RawFunction); \
SetSymbolMapEntry(Symbols, #LispName, strlen(#LispName), FunctionValue);} while (0)

	AddSymbolFunction(Add, +);
	AddSymbolFunction(Sub, -);
	AddSymbolFunction(Mul, *);
	AddSymbolFunction(Div, /);
	AddSymbolFunction(Mod, %);
	AddSymbolFunction(Quit, quit);
	AddSymbolFunction(ListFiles, ls);
	AddSymbolFunction(Print, print);
	AddSymbolFunction(PrintCharacter, print.char);
	AddSymbolFunction(ChangeDirectory, cd);
	AddSymbolFunction(GetCurrentDirectory, pwd);
	AddSymbolFunction(ListMake, list.make);
	AddSymbolFunction(ListLength, list.length);
	AddSymbolFunction(ListIndex, list.index);
	AddSymbolFunction(ListMap, list.map);
	AddSymbolFunction(ListPush, list.push);
	AddSymbolFunction(ListReplaceIndex, list.replace-index);
	AddSymbolFunction(StringLength, string.length);
	AddSymbolFunction(StringSplit, string.split);
	AddSymbolFunction(StringConcatenate, string.concat);
	AddSymbolFunction(StringSlice, string.sub);
	AddSymbolFunction(StringSymbol, string->symbol);
	AddSymbolFunction(Equals, =);
	AddSymbolFunction(Less, <);
	AddSymbolFunction(Greater, >);
	AddSymbolFunction(CreateProcess, process.make);
	AddSymbolFunction(ReadProcessOutput, process.read_output);
	AddSymbolFunction(GetReferenceCount, debug.grc);
	AddSymbolFunction(ReadCharacter, read.char);
	AddSymbolFunction(Eval, eval);
	AddSymbolFunction(Parse, parse);
	AddSymbolFunction(Slurp, slurp);
	AddSymbolFunction(ToString, any->string);
	AddSymbolFunction(OSEnvironmentGet, env.get);
	AddSymbolFunction(GetASCIICode, ascii);
	AddSymbolFunction(GetASCIICharacter, character);
	AddSymbolFunction(Reload, reload);
	AddSymbolFunction(Time, time);

	Environment* Result = alloc(sizeof(Environment));

	Result->Outer = NULL;
	Result->Symbols = Symbols;

	return Result;
}

#pragma clang diagnostic pop
