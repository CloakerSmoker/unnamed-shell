#include "common.h"

jmp_buf OnError;

char TranslateColor(char Color) {
	if (Color & BRIGHT) {
		Color ^= BRIGHT;
		Color += 90;
	}
	else {
		Color += 30;
	}

	return Color;
}

void SetTerminalColors(char Foreground, char Background) {
	printf("\x1B[%im", TranslateColor(Foreground));
	printf("\x1B[%im", TranslateColor(Background) + 10);
}

void PrintSpaces(int Count) {
	for (int Index = 0; Index < Count; Index++) {
		putchar(' ');
	}
}

char* FindStartOfLine(char* Start, int Offset) {
	int LastLineStart = 0;

	for (int Index = 0; Index < Offset; Index++) {
		if (Start[Index] == 0xA || Start[Index] == 0xD) {
			LastLineStart = Index + 1;
		}
	}

	return &Start[LastLineStart];
}

void ContextAlert(ErrorContext* Context, char* Message, char Color) {
	char LineNumberString[100] = {0};

	snprintf(LineNumberString, 100, "%d", Context->LineNumber);

	int LineNumberLength = (int)strlen(LineNumberString);

	if (Message) {
		SetTerminalColors(Color, BLACK);
		printf("%s\n", Message);
	}

	SetTerminalColors(WHITE | BRIGHT, BLACK);

	PrintSpaces(LineNumberLength + 1);
	printf(" [ ");
	SetTerminalColors(BLUE | BRIGHT, BLACK);
	printf("%s", Context->SourceFilePath);
	SetTerminalColors(WHITE | BRIGHT, BLACK);
	printf(" ]\n");

	printf(" %s | ", LineNumberString);

	int OffsetInSource = Context->Position;
	char* LineText = FindStartOfLine(Context->Source, OffsetInSource);
	int PositionInLine = OffsetInSource - (int)(LineText - Context->Source);

	int DashCount = 0;

	for (int LeftIndex = 0; LeftIndex < PositionInLine; LeftIndex++) {
		char NextCharacter = LineText[LeftIndex];

		if (NextCharacter == '\t') {
			printf("	");
			DashCount += 4;
		}
		else {
			putchar(NextCharacter);
			DashCount += 1;
		}
	}

	int RightIndex = PositionInLine;

	while (1) {
		char NextCharacter = LineText[RightIndex++];

		if (NextCharacter == 0xA || NextCharacter == 0xD || NextCharacter == 0) {
			break;
		}

		putchar(NextCharacter);
	}

	printf("\n");

	PrintSpaces(LineNumberLength + 1);
	printf(" |-");

	for (int DashIndex = 0; DashIndex < DashCount; DashIndex++) {
		putchar('-');
	}

	SetTerminalColors(Color, BLACK);

	for (int ArrowIndex = 0; ArrowIndex < Context->Length; ArrowIndex++) {
		putchar('^');
	}

	SetTerminalColors(WHITE | BRIGHT, BLACK);

	printf("\n");
}

void ContextError(ErrorContext* Context, char* Message) {
	ContextAlert(Context, Message, RED | BRIGHT);
}

ErrorContext* RawContextClone(ErrorContext* To, ErrorContext* From) {
	memcpy(To, From, sizeof(ErrorContext));

	return To;
}
ErrorContext* RawContextMerge(ErrorContext* Left, ErrorContext* Right) {
	if (Left->Position > Right->Position) {
		Left->Position = Right->Position;
	}

	int LeftEnd = Left->Position + Left->Length;
	int RightEnd = Right->Position + Right->Length;

	if (LeftEnd < RightEnd) {
		Left->Length = RightEnd - Left->Position;
	}
	else {
		Left->Length = LeftEnd - Left->Position;
	}

	return Left;
}

/// Copies the given string, and returns a String for the copied string
/// \param Text A pointer to the string to copy
/// \param Length
/// \return
String* MakeString(char* Text, size_t Length) {
	String* Result = malloc(sizeof(String));

	Result->Buffer = strndup(Text, Length);
	Result->Length = Length;
	Result->AllocationMethod = STRING_SELF_ALLOCATED;

	return Result;
}
/// Creates a new String referring to the given string, which is marked as just "borrowing" the memory, which means it will not be freed
/// \param Text A pointer to the string to borrow
/// \param Length
/// \return
String* BorrowString(char* Text, size_t Length) {
	String* Result = malloc(sizeof(String));

	Result->Buffer = Text;
	Result->Length = Length;
	Result->AllocationMethod = STRING_BORROWING_MEMORY;

	return Result;
}
/// Creates a new String referring to the given string, which will take ownership of the memory passed, and free it if needed.
/// \param Text A pointer to the string to take ownership of
/// \param Length
/// \return
String* AdoptString(char* Text, size_t Length) {
	String* Result = malloc(sizeof(String));

	Result->Buffer = Text;
	Result->Length = Length;
	Result->AllocationMethod = STRING_SELF_ALLOCATED;

	return Result;
}
/// Creates a new String containing the exact same text as the String passed, but in a separate allocation.
/// \param Target A pointer to the String to clone
/// \return
String* CloneString(String* Target) {
	String* Result = malloc(sizeof(String));

	Result->Buffer = strndup(Target->Buffer, Target->Length);
	Result->Length = Target->Length;
	Result->AllocationMethod = STRING_SELF_ALLOCATED;

	return Result;
}
/// Free-s the given String's memory, and the buffer containing the String's text if it is owned by said string
/// \param Target
void FreeString(String* Target) {
	if (Target->AllocationMethod == STRING_SELF_ALLOCATED) {
		free(Target->Buffer);
	}

	free(Target);
}
void PrintString(String* Target) {
	printf("%.*s", (int)Target->Length, Target->Buffer);
}

List* NewList(size_t Length) {
	List* Result = alloc(sizeof(List));

	Result->Length = Length;
	Result->Values = alloc(Length * sizeof(Value*));

	return Result;
}
void ExtendList(List* Target, size_t ElementCount) {
	Target->Length += ElementCount;
	Target->Values = realloc(Target->Values, Target->Length * sizeof(Value*));
}
void FreeList(List* Target) {
	free(Target->Values);
	free(Target);
}

void Value_Free(Value* Target) {
#if DEBUG_EVAL
	SetTerminalColors(GREEN | BRIGHT, BLACK);
	EVAL_DEBUG_PRINT_PREFIX printf("Destroy %i\n", Target->ID);
	SetTerminalColors(WHITE | BRIGHT, BLACK);
#endif

	switch (Target->Type) {
		case VALUE_TYPE_STRING:
		case VALUE_TYPE_IDENTIFIER:
			FreeString(Target->StringValue);
			break;
		case VALUE_TYPE_FUNCTION:
			if (!Target->FunctionValue->IsNativeFunction) {
				ReleaseReferenceToValue(Target->FunctionValue->Body);
				ReleaseReferenceToValue(Target->FunctionValue->ParameterBindings);
			}

			break;
		case VALUE_TYPE_LIST:
			for (int Index = 0; Index < Target->ListValue->Length; Index++) {
				ReleaseReferenceToValue(Target->ListValue->Values[Index]);
			}

			FreeList(Target->ListValue);
			break;
		default: break;
	}

	free(Target);
}

void Value_Print(Value*);

#if DEBUG_EVAL
int EvalDebugDepth = 0;
int EvalNextValueID = 1;
#endif

Value* AddReferenceToValue(Value* Target) {
#if DEBUG_REFCOUNTS
	SetTerminalColors(RED | GREEN, BLACK);
	EVAL_DEBUG_PRINT_PREFIX printf("AddRef %i '", Target->ID);
	Value_Print(Target);
#endif

	Target->ReferenceCount++;

#if DEBUG_REFCOUNTS
	printf("' (%i references left)\n", Target->ReferenceCount);
	SetTerminalColors(WHITE | BRIGHT, BLACK);
#endif

	return Target;
}
int ReleaseReferenceToValue(Value* Target) {
#if DEBUG_REFCOUNTS
	SetTerminalColors(BLUE | BRIGHT, BLACK);
	EVAL_DEBUG_PRINT_PREFIX printf("Release %i '", Target->ID);
	Value_Print(Target);
#endif

	int ReferenceCount = --Target->ReferenceCount;

#if DEBUG_REFCOUNTS
	printf("' (%i references left)\n", ReferenceCount);
	SetTerminalColors(WHITE | BRIGHT, BLACK);
#endif

	if (ReferenceCount == 0) {
		Value_Free(Target);
	}

	return ReferenceCount;
}

Value* NewPointerValue(ValueType Type, void* RawValue) {
	Value* Result = alloc(sizeof(Value));

	Result->Type = Type;
	Result->RawValue = RawValue;

#if DEBUG_EVAL
	Result->ID = EvalNextValueID++;

	SetTerminalColors(RED, BLACK);
	EVAL_DEBUG_PRINT_PREFIX printf("Created %i\n", Result->ID);
	SetTerminalColors(WHITE | BRIGHT, BLACK);
#endif

	return AddReferenceToValue(Result);
}
Value* NewIntegerValues(ValueType Type, int64_t RawValue) {
	Value* Result = alloc(sizeof(Value));

	Result->Type = Type;
	Result->IntegerValue = RawValue;

#if DEBUG_EVAL
	Result->ID = EvalNextValueID++;

	SetTerminalColors(RED, BLACK);
	EVAL_DEBUG_PRINT_PREFIX printf("Created %i\n", Result->ID);
	SetTerminalColors(WHITE | BRIGHT, BLACK);
#endif

	return AddReferenceToValue(Result);
}
Value* CloneValue(Value* Target) {
	Value* Result = NewValue(Target->Type, Target->IntegerValue);
	CloneContext(Result, Target);

	switch (Target->Type) {
		case VALUE_TYPE_LIST:
			for (int Index = 0; Index < Target->ListValue->Length; Index++) {
				AddReferenceToValue(Target->ListValue->Values[Index]);
			}

			List* ResultList = alloc(sizeof(List));
			ResultList->Length = Target->ListValue->Length;
			ResultList->Values = alloc(ResultList->Length * sizeof(Value*));

			memcpy(ResultList->Values, Target->ListValue->Values, ResultList->Length * sizeof(Value*));

			Result->ListValue = ResultList;

			break;
		case VALUE_TYPE_STRING:
		case VALUE_TYPE_IDENTIFIER:
			Result->StringValue = CloneString(Target->StringValue);
			break;
		case VALUE_TYPE_FUNCTION:
			if (!Target->FunctionValue->IsNativeFunction) {
				AddReferenceToValue(Target->FunctionValue->ParameterBindings);
				AddReferenceToValue(Target->FunctionValue->Body);
			}
		default:
			Result->IntegerValue = Target->IntegerValue;
			break;
	}

	return Result;
}
