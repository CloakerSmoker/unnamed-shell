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
			LastLineStart = Index;
			break;
		}
	}

	return &Start[LastLineStart];
}

void Context_Alert(ErrorContext* Context, char* Message, char Color) {
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

void Context_Error(ErrorContext* Context, char* Message) {
	Context_Alert(Context, Message, RED | BRIGHT);
}

ErrorContext *Context_Clone(ErrorContext* To, ErrorContext* From) {
	memcpy(To, From, sizeof(ErrorContext));

	return To;
}
ErrorContext *Context_Merge(ErrorContext* Left, ErrorContext* Right) {
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
String* String_New(char* Text, size_t Length) {
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
String* String_Borrow(char* Text, size_t Length) {
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
String* String_Adopt(char* Text, size_t Length) {
	String* Result = malloc(sizeof(String));

	Result->Buffer = Text;
	Result->Length = Length;
	Result->AllocationMethod = STRING_SELF_ALLOCATED;

	return Result;
}
/// Creates a new String containing the exact same text as the String passed, but in a separate allocation.
/// \param Target A pointer to the String to clone
/// \return
String* String_Clone(String* Target) {
	String* Result = malloc(sizeof(String));

	Result->Buffer = strndup(Target->Buffer, Target->Length);
	Result->Length = Target->Length;
	Result->AllocationMethod = STRING_SELF_ALLOCATED;

	return Result;
}
/// Free-s the given String's memory, and the buffer containing the String's text if it is owned by said string
/// \param Target
void String_Free(String* Target) {
	if (Target->AllocationMethod == STRING_SELF_ALLOCATED) {
		free(Target->Buffer);
	}

	free(Target);
}
void String_Print(String* Target) {
	printf("%.*s", (int)Target->Length, Target->Buffer);
}

List* List_New(size_t Length) {
	List* Result = alloc(sizeof(List));

	Result->Length = Length;
	Result->Values = alloc(Length * sizeof(Value*));

	return Result;
}
void List_Extend(List* Target, size_t ElementCount) {
	Target->Length += ElementCount;
	Target->Values = realloc(Target->Values, Target->Length * sizeof(Value*));
}
void List_Free(List* Target) {
	free(Target->Values);
	free(Target);
}

void Value_Free(Value* TargetValue) {
#if DEBUG_EVAL
	SetTerminalColors(GREEN | BRIGHT, BLACK);
	EVAL_DEBUG_PRINT_PREFIX printf("Destroy %i\n", TargetValue->ID);
	SetTerminalColors(WHITE | BRIGHT, BLACK);
#endif

	switch (TargetValue->Type) {
		case VALUE_STRING:
		case VALUE_IDENTIFIER:
			String_Free(TargetValue->StringValue);
			break;
		case VALUE_FUNCTION:
			if (!TargetValue->FunctionValue->IsNativeFunction) {
				Value_Release(TargetValue->FunctionValue->Body);
				Value_Release(TargetValue->FunctionValue->ParameterBindings);
			}

			break;
		case VALUE_LIST:
			for (int Index = 0; Index < TargetValue->ListValue->Length; Index++) {
				Value_Release(TargetValue->ListValue->Values[Index]);
			}

			List_Free(TargetValue->ListValue);
			break;
		default: break;
	}

	free(TargetValue);
}

void Value_Print(Value*);

#if DEBUG_EVAL
int EvalDebugDepth = 0;
int EvalNextValueID = 1;
#endif

Value* Value_AddReference(Value* TargetValue) {
#if DEBUG_REFCOUNTS
	SetTerminalColors(RED | GREEN, BLACK);
	EVAL_DEBUG_PRINT_PREFIX printf("AddRef %i '", TargetValue->ID);
	Value_Print(TargetValue);
#endif

	TargetValue->ReferenceCount++;

#if DEBUG_REFCOUNTS
	printf("' (%i references left)\n", TargetValue->ReferenceCount);
	SetTerminalColors(WHITE | BRIGHT, BLACK);
#endif

	return TargetValue;
}
int Value_Release(Value* TargetValue) {
#if DEBUG_REFCOUNTS
	SetTerminalColors(BLUE | BRIGHT, BLACK);
	EVAL_DEBUG_PRINT_PREFIX printf("Release %i '", TargetValue->ID);
	Value_Print(TargetValue);
#endif

	int ReferenceCount = --TargetValue->ReferenceCount;

#if DEBUG_REFCOUNTS
	printf("' (%i references left)\n", ReferenceCount);
	SetTerminalColors(WHITE | BRIGHT, BLACK);
#endif

	if (ReferenceCount == 0) {
		Value_Free(TargetValue);
	}

	return ReferenceCount;
}

Value* Value_New_Pointer(ValueType Type, void* RawValue) {
	Value* Result = alloc(sizeof(Value));

	Result->Type = Type;
	Result->RawValue = RawValue;

#if DEBUG_EVAL
	Result->ID = EvalNextValueID++;

	SetTerminalColors(RED, BLACK);
	EVAL_DEBUG_PRINT_PREFIX printf("Created %i\n", Result->ID);
	SetTerminalColors(WHITE | BRIGHT, BLACK);
#endif

	return Value_AddReference(Result);
}
Value* Value_New_Integer(ValueType Type, int64_t RawValue) {
	Value* Result = alloc(sizeof(Value));

	Result->Type = Type;
	Result->IntegerValue = RawValue;

#if DEBUG_EVAL
	Result->ID = EvalNextValueID++;

	SetTerminalColors(RED, BLACK);
	EVAL_DEBUG_PRINT_PREFIX printf("Created %i\n", Result->ID);
	SetTerminalColors(WHITE | BRIGHT, BLACK);
#endif

	return Value_AddReference(Result);
}
Value* Value_Clone(Value* TargetValue) {
	Value* Result = Value_New(TargetValue->Type, TargetValue->IntegerValue);

	switch (TargetValue->Type) {
		case VALUE_LIST:
			for (int Index = 0; Index < TargetValue->ListValue->Length; Index++) {
				Value_AddReference(TargetValue->ListValue->Values[Index]);
			}

			List* ResultList = alloc(sizeof(List));
			ResultList->Length = TargetValue->ListValue->Length;
			ResultList->Values = alloc(ResultList->Length * sizeof(Value*));

			memcpy(ResultList->Values, TargetValue->ListValue->Values, ResultList->Length * sizeof(Value*));

			Result->ListValue = ResultList;

			break;
		case VALUE_STRING:
		case VALUE_IDENTIFIER:
			Result->StringValue = String_Clone(TargetValue->StringValue);
			break;
		case VALUE_FUNCTION:
			if (!TargetValue->FunctionValue->IsNativeFunction) {
				Value_AddReference(TargetValue->FunctionValue->ParameterBindings);
				Value_AddReference(TargetValue->FunctionValue->Body);
			}
		default:
			Result->IntegerValue = TargetValue->IntegerValue;
			break;
	}

	return Result;
}
