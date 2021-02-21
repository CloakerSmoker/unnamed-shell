#include "eval.h"
#include "builtins.h"

int Hash(const char* String, int Length) {
	int Hash = 0;

	for (int Index = 0; Index < Length; Index++) {
		Hash = String[Index] + (Hash << 6) + (Hash << 16) - Hash;
	}

	return Hash;
}

SymbolMap* SymbolMap_New() {
	SymbolMap* this = alloc(sizeof(SymbolMap));

	this->ElementCapacity = 100;
	this->Elements = calloc(sizeof(SymbolEntry*), this->ElementCapacity);

	return this;
}

unsigned int SymbolMap_GetElementIndex(SymbolMap* this, int Hash) {
	return Hash % this->ElementCapacity;
}

SymbolEntry* SymbolMap_FindElement(SymbolMap* this, int Hash) {
	unsigned int Index = SymbolMap_GetElementIndex(this, Hash);

	SymbolEntry* Element = this->Elements[Index];

	if (!Element) {
		return NULL;
	}

	while (Element->Hash != Hash) {
		if (!Element->Next) {
			return Element;
		}

		Element = Element->Next;
	}

	return Element;
}

SymbolEntry* SymbolMap_GetElement(SymbolMap* this, int Hash) {
	SymbolEntry* Result = SymbolMap_FindElement(this, Hash);

	if (Result && Result->Hash == Hash) {
		return Result;
	}

	return NULL;
}

void SymbolMap_Upsert(SymbolMap* this, int Hash, void* Value) {
	unsigned int Index = SymbolMap_GetElementIndex(this, Hash);

	SymbolEntry* Element = SymbolMap_FindElement(this, Hash);
	SymbolEntry* NewElement = NULL;

	if (Element == NULL) {
		NewElement = alloc(sizeof(SymbolEntry));

		this->Elements[Index] = NewElement;
	}
	else if (Element->Hash != Hash) {
		NewElement = alloc(sizeof(SymbolEntry));
		NewElement->Next = Element->Next;

		Element->Next = NewElement;
	}
	else {
		NewElement = Element;
	}

	NewElement->Hash = Hash;
	NewElement->Value = Value;
}

SymbolEntry* SymbolMap_Get(SymbolMap* this, char* Key, size_t KeyLength) {
	return SymbolMap_GetElement(this, Hash(Key, KeyLength));
}
void SymbolMap_Set(SymbolMap* this, char* Key, size_t KeyLength, void* Value) {
	return SymbolMap_Upsert(this, Hash(Key, KeyLength), Value);
}

SymbolEntry* Environment_Get(Environment* this, String* SymbolName) {
	Environment* Next = this;

	while (Next) {
		SymbolEntry* Result = SymbolMap_Get(Next->Symbols, SymbolName->Buffer, SymbolName->Length);

		if (Result) {
#if DEBUG_SYMBOLS
			EVAL_DEBUG_PRINT_PREFIX
			printf("Get symbol ");
			String_Print(SymbolName);
			printf(" = '");
			Value_Print(Result->Value);
			printf("'\n");
#endif

			return Result;
		}

		Next = Next->Outer;
	}

#if DEBUG_SYMBOLS
	EVAL_DEBUG_PRINT_PREFIX
	printf("Get symbol ");
	String_Print(SymbolName);
	printf(" = not found\n");
#endif

	return NULL;
}

Environment* Environment_Find(Environment* this, String* SymbolName) {
	SymbolEntry* Result = SymbolMap_Get(this->Symbols, SymbolName->Buffer, SymbolName->Length);

	if (Result) {
		return this;
	}

	if (this->Outer) {
		return Environment_Find(this->Outer, SymbolName);
	}

	return NULL;
}

void Environment_Set(Environment* this, String* SymbolName, Value* Value) {
	Value_AddReference(Value);

#if DEBUG_SYMBOLS
	EVAL_DEBUG_PRINT_PREFIX
	printf("Set symbol ");
	String_Print(SymbolName);
	printf(" = '");
	Value_Print(Value);
	printf("'\n");
#endif

	Environment* ExistingEnvironment = Environment_Find(this, SymbolName);

	if (ExistingEnvironment != NULL) {
		SymbolMap_Set(ExistingEnvironment->Symbols, SymbolName->Buffer, SymbolName->Length, Value);
	}
	else {
		SymbolMap_Set(this->Symbols, SymbolName->Buffer, SymbolName->Length, Value);
	}
}

Environment* Environment_New(Environment* Outer) {
	Environment* Result = alloc(sizeof(Environment));

	Result->Symbols = SymbolMap_New();
	Result->Outer = Outer;

	return Result;
}
Environment* Environment_New_Bindings(Environment* Outer, Value* BindingNames, Value* BindingValues) {
	Environment* Result = Environment_New(Outer);

	for (int Index = 0; Index < BindingNames->ListValue->Length; Index++) {
		Value* BindingName = Eval_GetParameterRaw(BindingNames, VALUE_IDENTIFIER, Index);
		Value* BindingValue = Eval_GetParameterRaw(BindingValues, VALUE_ANY, Index + 1);

		Environment_Set(Result, BindingName->IdentifierValue, BindingValue);
	}

	return Result;
}
void Environment_ReleaseAndFree(Environment* this) {
	SymbolEntry** Symbols = this->Symbols->Elements;
	size_t SymbolCapacity = this->Symbols->ElementCapacity;

	for (int Index = 0; Index < SymbolCapacity; Index++) {
		SymbolEntry* NextSymbol = Symbols[Index];

		while (NextSymbol != NULL) {
			Value_Release(NextSymbol->Value);

			SymbolEntry* LastSymbol = NextSymbol = NextSymbol->Next;

			free(LastSymbol);
		}
	}

	free(this->Symbols);
	free(this);
}

char* Eval_GetTypeName(ValueType Type) {
	switch (Type) {
		case VALUE_INTEGER: return "integer";
		case VALUE_STRING: return "string";
		case VALUE_IDENTIFIER: return "identifier";
		case VALUE_FUNCTION: return "function";
		case VALUE_LIST: return "list";
		case VALUE_BOOL: return "bool";
		case VALUE_NIL: return "nil";
		case VALUE_CHILD: return "process";
		case VALUE_ANY: return "any";
		default: return "none";
	}
}

Value* Eval_GetParameterRaw(Value* ParameterList, ValueType ExpectedType, int Index) {
	if (ParameterList->ListValue->Length <= Index) {
		Error(ParameterList, "Not enough parameters");
		longjmp(OnError, 0);
	}

	Value* ParameterValue = ParameterList->ListValue->Values[Index];

	if (ParameterValue->Type != ExpectedType && ExpectedType != VALUE_ANY) {
		char MessageBuffer[100] = {0};

		snprintf(MessageBuffer, 100, "Wrong types, expected %s, got %s", Eval_GetTypeName(ExpectedType), Eval_GetTypeName(ParameterValue->Type));

		Error(ParameterValue, MessageBuffer);
		longjmp(OnError, 0);
	}

	return ParameterValue;
}

Value* Eval_CallFunction(unused Environment* this, Value* Call) {
	Value* FunctionNode = Call->ListValue->Values[0];

	if (FunctionNode->Type != VALUE_FUNCTION) {
		Error(FunctionNode, "Is not a valid function");
		longjmp(OnError, 0);
	}

	Function* TargetFunction = FunctionNode->FunctionValue;

	Value* Result = NULL;

	if (TargetFunction->IsNativeFunction) {
		Result = TargetFunction->NativeValue(Call);
	}
	else {
		size_t ParameterCount = Call->ListValue->Length - 1;

		if (ParameterCount != TargetFunction->ParameterBindings->ListValue->Length) {
			Error(Call, "Incorrect number of parameters passed to function");
			longjmp(OnError, 0);
		}

		Environment* Closure = Environment_New_Bindings(TargetFunction->Environment, TargetFunction->ParameterBindings, Call);

		Result = Eval_Apply(Closure, Value_AddReference(TargetFunction->Body));

		Environment_ReleaseAndFree(Closure);
	}

	Value_Release(Call);

	return Result;
}

Value* Eval_AST(Environment* this, Value* Target) {
#if DEBUG_EVAL
	EVAL_DEBUG_PRINT_PREFIX printf("Eval_AST %i '", Target->ID);
	Value_Print(Target);
	printf("' {\n");

	EVAL_DEBUG_PRELUDE;
#endif

	Value* Result;

	switch (Target->Type) {
		case VALUE_IDENTIFIER: {
			String* IdentifierText = Target->IdentifierValue;

			SymbolEntry* Symbol = Environment_Get(this, IdentifierText);

			if (Symbol != NULL) {
				Result = Value_AddReference(Symbol->Value);
			}
			else {
				Error(Target, "Undefined symbol");
				longjmp(OnError, 0);
			}

			break;
		}
		case VALUE_LIST: {
			List* ResultList = List_New(Target->ListValue->Length);

			for (int Index = 0; Index < Target->ListValue->Length; Index++) {
				Value* NextValue = Value_AddReference(Target->ListValue->Values[Index]);

				ResultList->Values[Index] = Eval_Apply(this, NextValue);
			}

			Result = Value_New(VALUE_LIST, ResultList);

			break;
		}
		default:
			Result = Value_Clone(Target);

			break;
	}

#if DEBUG_EVAL
	EVAL_DEBUG_EPILOG;

	EVAL_DEBUG_PRINT_PREFIX printf("} = ");
	Value_Print(Result);
	printf("\n");
#endif

	CloneContext(Result, Target);

	return Result;
}

Value* Eval_ToBool(Value* Target) {
	char BoolValue = 1;

	if (Target->Type == VALUE_NIL || Target->Type == VALUE_INTEGER && Target->IntegerValue == 0) {
		BoolValue = 0;
	}
	else if (Target->Type == VALUE_BOOL) {
		BoolValue = Target->BoolValue;
	}

	Value_Release(Target);

	return Value_New(VALUE_BOOL, BoolValue);
}

Value* Eval_Apply(Environment* this, Value* Target) {
#if DEBUG_EVAL
	EVAL_DEBUG_PRINT_PREFIX printf("Eval_Apply %i '", Target->ID);
	Value_Print(Target);
	printf("' {\n");

	EVAL_DEBUG_PRELUDE;
#endif

	Value* Result = Target;

	switch (Target->Type) {
		case VALUE_LIST: {
			if (Target->ListValue->Length >= 1) {
				Value* NameValue = Target->ListValue->Values[0];

				if (NameValue->Type == VALUE_IDENTIFIER) {
					String* Name = NameValue->IdentifierValue;

					if (!strncmp(Name->Buffer, "def!", Name->Length)) {
						Value* DefKey = Eval_GetParameter(Target, VALUE_IDENTIFIER, 0);
						Value* DefValue = Eval_GetParameter(Target, VALUE_ANY, 1);

						SymbolEntry* Symbol = Environment_Get(this, DefKey->IdentifierValue);

						Result = Eval_Apply(this, Value_AddReference(DefValue));

						if (Symbol != NULL) {
							Value_Release(Symbol->Value);
						}

						Environment_Set(this, DefKey->IdentifierValue, Result);

						break;
					}
					else if (!strncmp(Name->Buffer, "let*", Name->Length)) {
						Value* BindingsList = Eval_GetParameter(Target, VALUE_LIST, 0);
						Value* LetBody = Eval_GetParameter(Target, VALUE_ANY, 1);

						size_t BindingCount = BindingsList->ListValue->Length;

						if (BindingCount % 2 != 0) {
							Error(BindingsList, "Uneven number of `let*` binding elements:");
							Error(BindingsList->ListValue->Values[BindingCount - 1], "Not part of a binding pair:");
							longjmp(OnError, 0);
						}

						BindingCount /= 2;

						Environment* NewEnvironment = Environment_New(this);

						for (int PairIndex = 0; PairIndex < BindingCount; PairIndex++) {
							int ValueIndex = PairIndex * 2;

							Value* BindingName = Eval_GetParameterReferenceRaw(BindingsList, VALUE_IDENTIFIER,
							                                                   ValueIndex);
							Value* BindingValue = Eval_GetParameterReferenceRaw(BindingsList, VALUE_ANY,
							                                                    ValueIndex + 1);

							Value* NewBindingValue = Eval_Apply(NewEnvironment, Value_AddReference(BindingValue));

							Environment_Set(NewEnvironment, BindingName->IdentifierValue, NewBindingValue);

							Value_Release(BindingName);
							Value_Release(BindingValue);
							Value_Release(NewBindingValue);
						}

						Result = Eval_Apply(NewEnvironment, Value_AddReference(LetBody));

						Environment_ReleaseAndFree(NewEnvironment);

						break;
					}
					else if (!strncmp(Name->Buffer, "if", Name->Length)) {
						Value* Condition = Eval_GetParameterReference(Target, VALUE_ANY, 0);
						Value* TrueBranch = Eval_GetParameterReference(Target, VALUE_ANY, 1);
						Value* FalseBranch = NULL;

						if (Target->ListValue->Length == 4) {
							FalseBranch = Eval_GetParameterReference(Target, VALUE_ANY, 2);
						}

						Value* ConditionResult = Eval_ToBool(Eval_Apply(this, Condition));

						if (ConditionResult->BoolValue == 1) {
							Result = Eval_Apply(this, TrueBranch);

							if (FalseBranch != NULL) {
								Value_Release(FalseBranch);
							}
						}
						else if (FalseBranch != NULL) {
							Result = Eval_Apply(this, FalseBranch);

							Value_Release(TrueBranch);
						}
						else {
							Result = Value_Nil();

							Value_Release(TrueBranch);
						}

						Value_Release(ConditionResult);

						break;
					}
					else if (!strncmp(Name->Buffer, "do", Name->Length)) {
						size_t Length = Target->ListValue->Length;

						for (int Index = 1; Index < Length - 1; Index++) {
							Value* NextValue = Target->ListValue->Values[Index];

							Value_Release(Eval_Apply(this, Value_AddReference(NextValue)));
						}

						if (Length == 1) {
							Result = Value_Nil();
						}
						else {
							Value* LastValue = Target->ListValue->Values[Length - 1];

							Result = Eval_Apply(this, Value_AddReference(LastValue));
						}

						break;
					}
					else if (!strncmp(Name->Buffer, "fn*", Name->Length)) {
						Value* BindingNames = Eval_GetParameterReference(Target, VALUE_LIST, 0);
						Value* Body = Eval_GetParameterReference(Target, VALUE_ANY, 1);

						for (int Index = 0; Index < BindingNames->ListValue->Length; Index++) {
							Eval_GetParameterRaw(BindingNames, VALUE_IDENTIFIER, Index);
						}

						Function* NewFunction = alloc(sizeof(Function));

						NewFunction->ParameterBindings = BindingNames;
						NewFunction->Body = Body;
						NewFunction->Environment = this;

						Value* NewFunctionValue = Value_New(VALUE_FUNCTION, NewFunction);

						CloneContext(NewFunctionValue, Target);

						Result = NewFunctionValue;

						break;
					}
					else if (!strncmp(Name->Buffer, "load!", Name->Length)) {
						Value* FileNameValue = Eval_GetParameter(Target, VALUE_STRING, 0);
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

						Tokenizer* FileTokenizer = Tokenizer_New(FileName->Buffer, Buffer, strlen(Buffer));

						Value* FileTree = ReadForm(FileTokenizer);

						Result = Eval_Apply(this, FileTree);

						break;
					}
				}
			}

			Value* Call = Eval_AST(this, Target);

			if (Call->ListValue->Length >= 1) {
				Result = Eval_CallFunction(this, Call);
			}

			break;
		}
		default:
			Result = Eval_AST(this, Target);
	}

	Value_Release(Target);

#if DEBUG_EVAL
	EVAL_DEBUG_EPILOG;

	EVAL_DEBUG_PRINT_PREFIX printf("} = ");
	Value_Print(Result);
	printf("\n");
#endif

	CloneContext(Result, Target);

	return Result;
}
