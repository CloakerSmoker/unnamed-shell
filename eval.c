#include "eval.h"

int Hash(const char* String, int Length) {
	int Hash = 0;

	for (int Index = 0; Index < Length; Index++) {
		Hash = String[Index] + (Hash << 6) + (Hash << 16) - Hash;
	}

	return Hash;
}

SymbolMap* NewSymbolMap() {
	SymbolMap* this = alloc(sizeof(SymbolMap));

	this->ElementCapacity = 100;
	this->Elements = calloc(sizeof(SymbolEntry*), this->ElementCapacity);

	return this;
}

unsigned int GetSymbolMapElementIndex(SymbolMap* this, int Hash) {
	return Hash % this->ElementCapacity;
}

SymbolEntry* FindSymbolMapElement(SymbolMap* this, int Hash) {
	unsigned int Index = GetSymbolMapElementIndex(this, Hash);

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

SymbolEntry* GetSymbolMapElement(SymbolMap* this, int Hash) {
	SymbolEntry* Result = FindSymbolMapElement(this, Hash);

	if (Result && Result->Hash == Hash) {
		return Result;
	}

	return NULL;
}

void UpsertSymbolMapEntry(SymbolMap* this, int Hash, void* Value) {
	unsigned int Index = GetSymbolMapElementIndex(this, Hash);

	SymbolEntry* Element = FindSymbolMapElement(this, Hash);
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

SymbolEntry* GetSymbolMapEntry(SymbolMap* this, char* Key, size_t KeyLength) {
	return GetSymbolMapElement(this, Hash(Key, KeyLength));
}
void SetSymbolMapEntry(SymbolMap* this, char* Key, size_t KeyLength, void* Value) {
	return UpsertSymbolMapEntry(this, Hash(Key, KeyLength), Value);
}

SymbolEntry* GetEnvironmentEntry(Environment* this, String* SymbolName) {
	Environment* Next = this;

	while (Next) {
		SymbolEntry* Result = GetSymbolMapEntry(Next->Symbols, SymbolName->Buffer, SymbolName->Length);

		if (Result) {
#if DEBUG_SYMBOLS
			EVAL_DEBUG_PRINT_PREFIX
			printf("Get symbol ");
			PrintString(SymbolName);
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
	PrintString(SymbolName);
	printf(" = not found\n");
#endif

	return NULL;
}

Environment* FindEnvironmentEntry(Environment* this, String* SymbolName) {
	SymbolEntry* Result = GetSymbolMapEntry(this->Symbols, SymbolName->Buffer, SymbolName->Length);

	if (Result) {
		return this;
	}

	if (this->Outer) {
		return FindEnvironmentEntry(this->Outer, SymbolName);
	}

	return NULL;
}

void SetEnvironmentEntry(Environment* this, String* SymbolName, Value* Value) {
	AddReferenceToValue(Value);

#if DEBUG_SYMBOLS
	EVAL_DEBUG_PRINT_PREFIX
	printf("Set symbol ");
	PrintString(SymbolName);
	printf(" = '");
	Value_Print(Value);
	printf("'\n");
#endif

	Environment* ExistingEnvironment = FindEnvironmentEntry(this, SymbolName);

	if (ExistingEnvironment != NULL) {
		SetSymbolMapEntry(ExistingEnvironment->Symbols, SymbolName->Buffer, SymbolName->Length, Value);
	}
	else {
		SetSymbolMapEntry(this->Symbols, SymbolName->Buffer, SymbolName->Length, Value);
	}
}

Environment* NewEnvironment(Environment* Outer) {
	Environment* Result = alloc(sizeof(Environment));

	Result->Symbols = NewSymbolMap();
	Result->Outer = Outer;

	return Result;
}

void ReleaseEnvironmentEntries(Environment* Target) {
	SymbolEntry** Symbols = Target->Symbols->Elements;
	size_t SymbolCapacity = Target->Symbols->ElementCapacity;

	for (int Index = 0; Index < SymbolCapacity; Index++) {
		SymbolEntry* NextSymbol = Symbols[Index];

		while (NextSymbol != NULL) {
			ReleaseReferenceToValue(NextSymbol->Value);

			SymbolEntry* LastSymbol = NextSymbol;
			NextSymbol = NextSymbol->Next;
			free(LastSymbol);
		}
	}
}
void FreeEnvironment(Environment* Target) {
	free(Target->Symbols);
	free(Target);
}
void DestroyEnvironment(Environment* Target) {
	ReleaseEnvironmentEntries(Target);
	FreeEnvironment(Target);
}

char* GetValueTypeName(ValueType Type) {
	switch (Type) {
		case VALUE_TYPE_INTEGER: return "integer";
		case VALUE_TYPE_STRING: return "string";
		case VALUE_TYPE_IDENTIFIER: return "identifier";
		case VALUE_TYPE_FUNCTION: return "function";
		case VALUE_TYPE_LIST: return "list";
		case VALUE_TYPE_BOOL: return "bool";
		case VALUE_TYPE_NIL: return "nil";
		case VALUE_TYPE_CHILD: return "process";
		case VALUE_TYPE_ANY: return "any";
		default: return "none";
	}
}
void RaiseTypeError(Value* Blame, ValueType ExpectedType) {
	char MessageBuffer[100] = {0};

	snprintf(MessageBuffer, 100, "Wrong types, expected %s, got %s", GetValueTypeName(ExpectedType),
	         GetValueTypeName(Blame->Type));

	Error(Blame, MessageBuffer);
	longjmp(OnError, 0);
}

Value* RawGetListIndex(Value* Target, ValueType ExpectedType, int Index) {
	if (Target->ListValue->Length <= Index) {
		Error(Target, "Not enough parameters");
		longjmp(OnError, 0);
	}

	Value* ParameterValue = Target->ListValue->Values[Index];

	if (ParameterValue->Type != ExpectedType && ExpectedType != VALUE_TYPE_ANY) {
		RaiseTypeError(ParameterValue, ExpectedType);
	}

	return ParameterValue;
}

Value* RawEvaluateFunctionCall(unused Environment* this, Function* TargetFunction, Value* Call) {
	Value* Result = NULL;

	if (TargetFunction->IsNativeFunction) {
		Result = TargetFunction->NativeValue(Call);
	}
	else {
		Value* BindingNames = TargetFunction->ParameterBindings;

		size_t PassedParameterCount = Call->ListValue->Length - 1;
		size_t ExpectedParameterCount = BindingNames->ListValue->Length;

		size_t BindingNamesCount = ExpectedParameterCount;

		if (TargetFunction->IsVariadic) {
			if (PassedParameterCount < ExpectedParameterCount - 2) {
				Error(Call, "Incorrect number of parameters passed to function");
				longjmp(OnError, 0);
			}

			BindingNamesCount -= 2;
		}
		else if (PassedParameterCount != ExpectedParameterCount) {
			Error(Call, "Incorrect number of parameters passed to function");
			longjmp(OnError, 0);
		}

		Environment* Closure = NewEnvironment(TargetFunction->Environment);

		for (int Index = 0; Index < BindingNamesCount; Index++) {
			Value* BindingName = RawGetListIndex(BindingNames, VALUE_TYPE_IDENTIFIER, Index);
			Value* BindingValue = RawGetListIndex(Call, VALUE_TYPE_ANY, Index + 1);

			SetEnvironmentEntry(Closure, BindingName->IdentifierValue, BindingValue);
		}

		if (TargetFunction->IsVariadic) {
			Value* FinalBindingName = RawGetListIndex(BindingNames, VALUE_TYPE_IDENTIFIER, BindingNames->ListValue->Length - 1);

			size_t VariadicCount = Call->ListValue->Length - 1 - BindingNamesCount;

			List* VariadicList = NewList(VariadicCount);

			for (size_t Index = 0; Index < VariadicCount; Index++) {
				VariadicList->Values[Index] = RawGetListIndex(Call, VALUE_TYPE_ANY, Index + BindingNamesCount + 1);
			}

			Value* VariadicParameters = NewValue(VALUE_TYPE_LIST, VariadicList);

			SetEnvironmentEntry(Closure, FinalBindingName->IdentifierValue, VariadicParameters);
		}

		Result = Evaluate(Closure, AddReferenceToValue(TargetFunction->Body));

		DestroyEnvironment(Closure);
	}

	ReleaseReferenceToValue(Call);

	return Result;
}
Value* EvaluateFunctionCall(Environment* this, Value* Call) {
	Value* FunctionNode = Call->ListValue->Values[0];

	if (FunctionNode->Type != VALUE_TYPE_FUNCTION) {
		Error(FunctionNode, "Is not a valid function");
		longjmp(OnError, 0);
	}

	Function* TargetFunction = FunctionNode->FunctionValue;

	return RawEvaluateFunctionCall(this, TargetFunction, Call);
}

Value* EvaluateAST(Environment* this, Value* Target) {
#if DEBUG_EVAL
	EVAL_DEBUG_PRINT_PREFIX printf("EvaluateAST %i '", Target->ID);
	Value_Print(Target);
	printf("' {\n");

	EVAL_DEBUG_PRELUDE;
#endif

	Value* Result;

	switch (Target->Type) {
		case VALUE_TYPE_IDENTIFIER: {
			String* IdentifierText = Target->IdentifierValue;

			SymbolEntry* Symbol = GetEnvironmentEntry(this, IdentifierText);

			if (Symbol != NULL) {
				Result = AddReferenceToValue(Symbol->Value);
			}
			else {
				Error(Target, "Undefined symbol");
				longjmp(OnError, 0);
			}

			break;
		}
		case VALUE_TYPE_LIST: {
			List* ResultList = NewList(Target->ListValue->Length);

			for (int Index = 0; Index < Target->ListValue->Length; Index++) {
				Value* NextValue = AddReferenceToValue(Target->ListValue->Values[Index]);

				ResultList->Values[Index] = Evaluate(this, NextValue);
			}

			Result = NewValue(VALUE_TYPE_LIST, ResultList);

			break;
		}
		default:
			Result = CloneValue(Target);

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

Value* ConvertToBooleanValue(Value* Target) {
	char BoolValue = 1;

	if (Target->Type == VALUE_TYPE_NIL || Target->Type == VALUE_TYPE_INTEGER && Target->IntegerValue == 0) {
		BoolValue = 0;
	}
	else if (Target->Type == VALUE_TYPE_BOOL) {
		BoolValue = Target->BoolValue;
	}

	ReleaseReferenceToValue(Target);

	return NewValue(VALUE_TYPE_BOOL, BoolValue);
}

Value* QuoteValue(Environment* this, Value* Target, int IsQuasiQuote) {
	if (Target->Type == VALUE_TYPE_LIST && Target->ListValue->Length != 0) {
		Value* FirstValue = Target->ListValue->Values[0];

		if (FirstValue->Type == VALUE_TYPE_IDENTIFIER && IsQuasiQuote) {
			String* Name = FirstValue->IdentifierValue;

			if (!strncmp(Name->Buffer, "unquote", Name->Length)) {
				Value* UnquoteValue = GetReferenceToListIndex(Target, VALUE_TYPE_ANY, 0);

				return Evaluate(this, UnquoteValue);
			}
		}

		List* ResultList = NewList(Target->ListValue->Length);
		int ResultListIndex = 0;

		for (int Index = 0; Index < Target->ListValue->Length; Index++) {
			Value* ListElement = Target->ListValue->Values[Index];

			if (ListElement->Type == VALUE_TYPE_LIST && ListElement->ListValue->Length != 0) {
				FirstValue = ListElement->ListValue->Values[0];

				if (FirstValue->Type == VALUE_TYPE_IDENTIFIER && IsQuasiQuote) {
					String* Name = FirstValue->IdentifierValue;

					if (!strncmp(Name->Buffer, "splice-unquote", Name->Length)) {
						ResultList->Length--; // Exclude `"splice-unquote"` from the list length

						Value* RawUnquoteValue = GetReferenceToListIndex(ListElement, VALUE_TYPE_ANY, 0);

						Value* UnquoteValue = Evaluate(this, RawUnquoteValue);

						if (UnquoteValue->Type == VALUE_TYPE_LIST) {
							List* SpliceList = UnquoteValue->ListValue;

							ExtendList(ResultList, SpliceList->Length);

							for (int SpliceIndex = 0; SpliceIndex < SpliceList->Length; SpliceIndex++) {
								Value* NextSpliceValue = AddReferenceToValue(SpliceList->Values[SpliceIndex]);

								ResultList->Values[ResultListIndex++] = NextSpliceValue;
							}
						}
						else {
							ExtendList(ResultList, 1);

							ResultList->Values[ResultListIndex++] = AddReferenceToValue(UnquoteValue);
						}

						continue;
					}
				}
			}

			ResultList->Values[ResultListIndex++] = QuoteValue(this, ListElement, IsQuasiQuote);
		}

		Value* Result = NewValue(VALUE_TYPE_LIST, ResultList);

		CloneContext(Result, Target);

		return Result;
	}
	else {
		return CloneValue(Target);
	}
}

char IsMacroCall(Environment* this, Value* Target, Function** OutFunction) {
	if (Target->Type == VALUE_TYPE_LIST) {
		if (Target->ListValue->Length >= 1) {
			Value* FirstValue = Target->ListValue->Values[0];

			if (FirstValue->Type == VALUE_TYPE_IDENTIFIER) {
				SymbolEntry* FoundMacroEntry = GetEnvironmentEntry(this, FirstValue->IdentifierValue);

				if (FoundMacroEntry != NULL) {
					Value* FoundMacro = FoundMacroEntry->Value;

					if (FoundMacro->Type == VALUE_TYPE_FUNCTION && FoundMacro->FunctionValue->IsMacro) {
						*OutFunction = FoundMacro->FunctionValue;
						return 1;
					}
				}
			}
		}
	}

	return 0;
}
Value* ExpandMacro(Environment* this, Value* OriginalTarget) {
	Value* Target = OriginalTarget;
	Function* NextMacro = NULL;

	while (IsMacroCall(this, Target, &NextMacro)) {
		Target = RawEvaluateFunctionCall(this, NextMacro, Target);

		CloneContext(Target, OriginalTarget);
	}

	return Target;
}

Value* Evaluate(Environment* this, Value* Target) {
#if DEBUG_EVAL
	EVAL_DEBUG_PRINT_PREFIX printf("Evaluate %i '", Target->ID);
	Value_Print(Target);
	printf("' {\n");

	EVAL_DEBUG_PRELUDE;
#endif

	Value* Result = NULL;

	switch (Target->Type) {
		case VALUE_TYPE_LIST: {
			if (Target->ListValue->Length >= 1) {
				Target = ExpandMacro(this, Target);

				if (Target->Type != VALUE_TYPE_LIST) {
					Result = EvaluateAST(this, Target);
					break;
				}

				Value* NameValue = Target->ListValue->Values[0];

				if (NameValue->Type == VALUE_TYPE_IDENTIFIER) {
					String* Name = NameValue->IdentifierValue;

					if (!strncmp(Name->Buffer, "expand", Name->Length)) {
						Value* Macro = GetListIndex(Target, VALUE_TYPE_ANY, 0);

						Result = ExpandMacro(this, Macro);

						break;
					}
					else if (!strncmp(Name->Buffer, "while*", Name->Length)) {
						Value* Body = GetListIndex(Target, VALUE_TYPE_ANY, 0);

						Value* WhileValue = ConvertToBooleanValue(Evaluate(this, AddReferenceToValue(Body)));

						while (WhileValue->BoolValue) {
							ReleaseReferenceToValue(WhileValue);
							WhileValue = ConvertToBooleanValue(Evaluate(this, AddReferenceToValue(Body)));
						}

						ReleaseReferenceToValue(WhileValue);

						Result = NilValue();

						break;
					}
					else if (!strncmp(Name->Buffer, "def!", Name->Length)) {
						Value* Key = GetListIndex(Target, VALUE_TYPE_IDENTIFIER, 0);
						Value* Value = GetListIndex(Target, VALUE_TYPE_ANY, 1);

						SymbolEntry* Symbol = GetEnvironmentEntry(this, Key->IdentifierValue);

						Result = Evaluate(this, AddReferenceToValue(Value));

						if (Symbol != NULL) {
							ReleaseReferenceToValue(Symbol->Value);
						}

						SetEnvironmentEntry(this, Key->IdentifierValue, Result);

						Result = NilValue();

						break;
					}
					if (!strncmp(Name->Buffer, "macro!", Name->Length)) {
						Value* Key = GetListIndex(Target, VALUE_TYPE_IDENTIFIER, 0);
						Value* Value = GetListIndex(Target, VALUE_TYPE_ANY, 1);

						SymbolEntry* Symbol = GetEnvironmentEntry(this, Key->IdentifierValue);

						Result = Evaluate(this, AddReferenceToValue(Value));

						if (Result->Type != VALUE_TYPE_FUNCTION) {
							RaiseTypeError(Result, VALUE_TYPE_FUNCTION);
						}

						if (Symbol != NULL) {
							ReleaseReferenceToValue(Symbol->Value);
						}

						Result->FunctionValue->IsMacro = 1;
						SetEnvironmentEntry(this, Key->IdentifierValue, Result);

						break;
					}
					else if (!strncmp(Name->Buffer, "let*", Name->Length)) {
						Value* BindingsList = GetListIndex(Target, VALUE_TYPE_LIST, 0);
						Value* LetBody = GetListIndex(Target, VALUE_TYPE_ANY, 1);

						size_t BindingCount = BindingsList->ListValue->Length;

						if (BindingCount % 2 != 0) {
							Error(BindingsList, "Uneven number of `let*` binding elements:");
							Error(BindingsList->ListValue->Values[BindingCount - 1], "Not part of a binding pair:");
							longjmp(OnError, 0);
						}

						BindingCount /= 2;

						Environment* LetEnvironment = NewEnvironment(this);

						for (int PairIndex = 0; PairIndex < BindingCount; PairIndex++) {
							int ValueIndex = PairIndex * 2;

							Value* BindingName = RawGetReferenceToListIndex(BindingsList, VALUE_TYPE_IDENTIFIER,
							                                                ValueIndex);
							Value* BindingValue = RawGetReferenceToListIndex(BindingsList, VALUE_TYPE_ANY,
							                                                    ValueIndex + 1);

							Value* NewBindingValue = Evaluate(LetEnvironment, AddReferenceToValue(BindingValue));

							SetEnvironmentEntry(LetEnvironment, BindingName->IdentifierValue, NewBindingValue);

							ReleaseReferenceToValue(BindingName);
							ReleaseReferenceToValue(BindingValue);
							ReleaseReferenceToValue(NewBindingValue);
						}

						Result = Evaluate(LetEnvironment, AddReferenceToValue(LetBody));

						DestroyEnvironment(LetEnvironment);

						break;
					}
					else if (!strncmp(Name->Buffer, "if", Name->Length)) {
						Value* Condition = GetReferenceToListIndex(Target, VALUE_TYPE_ANY, 0);
						Value* TrueBranch = GetReferenceToListIndex(Target, VALUE_TYPE_ANY, 1);
						Value* FalseBranch = NULL;

						if (Target->ListValue->Length == 4) {
							FalseBranch = GetReferenceToListIndex(Target, VALUE_TYPE_ANY, 2);
						}

						Value* ConditionResult = ConvertToBooleanValue(Evaluate(this, Condition));

						if (ConditionResult->BoolValue == 1) {
							Result = Evaluate(this, TrueBranch);

							if (FalseBranch != NULL) {
								ReleaseReferenceToValue(FalseBranch);
							}
						}
						else if (FalseBranch != NULL) {
							Result = Evaluate(this, FalseBranch);

							ReleaseReferenceToValue(TrueBranch);
						}
						else {
							Result = NilValue();

							ReleaseReferenceToValue(TrueBranch);
						}

						ReleaseReferenceToValue(ConditionResult);

						break;
					}
					else if (!strncmp(Name->Buffer, "do", Name->Length)) {
						size_t Length = Target->ListValue->Length;

						for (int Index = 1; Index < Length - 1; Index++) {
							Value* NextValue = Target->ListValue->Values[Index];

							ReleaseReferenceToValue(Evaluate(this, AddReferenceToValue(NextValue)));
						}

						if (Length == 1) {
							Result = NilValue();
						}
						else {
							Value* LastValue = Target->ListValue->Values[Length - 1];

							Result = Evaluate(this, AddReferenceToValue(LastValue));
						}

						break;
					}
					else if (!strncmp(Name->Buffer, "fn*", Name->Length)) {
						Value* BindingNames = GetReferenceToListIndex(Target, VALUE_TYPE_LIST, 0);
						Value* Body = GetReferenceToListIndex(Target, VALUE_TYPE_ANY, 1);
						char IsVariadic = 0;

						for (int Index = 0; Index < BindingNames->ListValue->Length; Index++) {
							Value* NextBindingValue = RawGetListIndex(BindingNames, VALUE_TYPE_IDENTIFIER, Index);

							String* NextBinding = NextBindingValue->StringValue;

							if (!strncmp(NextBinding->Buffer, "...", NextBinding->Length)) {
								IsVariadic = 1;

								RawGetListIndex(BindingNames, VALUE_TYPE_IDENTIFIER, Index + 1);

								if (Index + 2 != BindingNames->ListValue->Length) {
									Error(BindingNames, "Variadic binding name should be last in binding list");
								}
							}
						}

						Function* NewFunction = alloc(sizeof(Function));

						NewFunction->ParameterBindings = BindingNames;
						NewFunction->Body = Body;
						NewFunction->Environment = this;
						NewFunction->IsVariadic = IsVariadic;

						Value* NewFunctionValue = NewValue(VALUE_TYPE_FUNCTION, NewFunction);

						CloneContext(NewFunctionValue, Target);

						Result = NewFunctionValue;

						break;
					}
					else if (!strncmp(Name->Buffer, "load!", Name->Length)) {
						Value* FileNameValue = GetListIndex(Target, VALUE_TYPE_ANY, 0);

						Value* ActualFileNameValue = Evaluate(this, AddReferenceToValue(FileNameValue));

						if (ActualFileNameValue->Type != VALUE_TYPE_STRING) {
							RaiseTypeError(ActualFileNameValue, VALUE_TYPE_STRING);
						}

						String* FileName = ActualFileNameValue->StringValue;

						FILE* FileDescriptor = fopen(FileName->Buffer, "r");

						if (FileDescriptor == NULL) {
							char MessageBuffer[100] = {0};

							snprintf(MessageBuffer, 100, "Could not open file '%s'", FileName->Buffer);

							Error(ActualFileNameValue, MessageBuffer);
							longjmp(OnError, 0);
						}

						fseek(FileDescriptor, 0, SEEK_END);

						long Length = ftell(FileDescriptor);

						fseek(FileDescriptor, 0, SEEK_SET);

						char* Buffer = alloc(Length + 1);

						fread(Buffer, 1, Length, FileDescriptor);

						Tokenizer* FileTokenizer = NewTokenizer(FileName->Buffer, Buffer, strlen(Buffer));

						Value* FileTree = ReadForm(FileTokenizer);

						Result = Evaluate(this, AddReferenceToValue(FileTree));

						ReleaseReferenceToValue(FileTree);
						ReleaseReferenceToValue(ActualFileNameValue);

						break;
					}
					else if (!strncmp(Name->Buffer, "quote", Name->Length)) {
						Value* QuotedValue = GetListIndex(Target, VALUE_TYPE_ANY, 0);

						Result = QuoteValue(this, QuotedValue, 0);

						break;
					}
					else if (!strncmp(Name->Buffer, "quasiquote", Name->Length)) {
						Value* QuotedValue = GetListIndex(Target, VALUE_TYPE_ANY, 0);

						Result = QuoteValue(this, QuotedValue, 1);

						break;
					}
				}
			}

			Value* Call = EvaluateAST(this, Target);

			if (Call->ListValue->Length >= 1) {
				Result = EvaluateFunctionCall(this, Call);
			}

			break;
		}
		default:
			Result = EvaluateAST(this, Target);
	}

	ReleaseReferenceToValue(Target);

#if DEBUG_EVAL
	EVAL_DEBUG_EPILOG;

	EVAL_DEBUG_PRINT_PREFIX printf("} = ");
	Value_Print(Result);
	printf("\n");
#endif

	CloneContext(Result, Target);

	return Result;
}
