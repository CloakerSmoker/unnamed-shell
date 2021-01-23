#include "eval.h"

int Hash(char* String, int Length) {
    int Hash = 0;
    int NextCharacter;

    char* End = &String[Length];

    while ((NextCharacter = *String++) && String <= End)
        Hash = NextCharacter + (Hash << 6) + (Hash << 16) - Hash;

    return Hash;
}

SymbolMap* SymbolMap_New() {
    SymbolMap* this = alloc(sizeof(SymbolMap));

    this->ElementCapacity = 100;
    this->Elements = calloc(sizeof(SymbolEntry*), this->ElementCapacity);

    return this;
}

int SymbolMap_GetElementIndex(SymbolMap* this, int Hash) {
    int Index = Hash % this->ElementCapacity;

    if (Index < 0) {
        Index = -Index;
    }

    return Index;
}

SymbolEntry* SymbolMap_FindElement(SymbolMap* this, int Hash) {
    int Index = SymbolMap_GetElementIndex(this, Hash);

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

void SymbolMap_Upsert(SymbolMap* this, int Hash, SymbolType Type, void* Value) {
    int Index = SymbolMap_GetElementIndex(this, Hash);

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
    NewElement->Type = Type;
    NewElement->VoidValue = Value;
}

SymbolEntry* SymbolMap_Get(SymbolMap* this, char* Key, int KeyLength) {
    return SymbolMap_GetElement(this, Hash(Key, KeyLength));
}
void SymbolMap_Set(SymbolMap* this, char* Key, int KeyLength, SymbolType Type, void* Value) {
    return SymbolMap_Upsert(this, Hash(Key, KeyLength), Type, Value);
}


Environment* Environment_New(Environment* Outer) {
    Environment* Result = alloc(sizeof(Environment));

    Result->Symbols = SymbolMap_New();
    Result->Outer = Outer;

    return Result;
}

SymbolEntry* Environment_Get(Environment* this, String* SymbolName) {
    Environment* Next = this;

    while (Next) {
        SymbolEntry* Result = SymbolMap_Get(Next->Symbols, SymbolName->Buffer, SymbolName->Length);

        if (Result) {
            return Result;
        }

        Next = Next->Outer;
    }

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

void Environment_SetRaw(Environment* this, String* SymbolName, SymbolType Type, void* Value) {
    SymbolMap_Set(this->Symbols, SymbolName->Buffer, SymbolName->Length, Type, Value);
}
void Environment_Set(Environment* this, String* SymbolName, Value* Value) {
    SymbolMap_Set(this->Symbols, SymbolName->Buffer, SymbolName->Length, SYMBOL_VALUE, Value);
}


char* Eval_GetTypeName(ValueType Type) {
    switch (Type) {
        case VALUE_INTEGER: return "integer";
        case VALUE_STRING: return "string";
        case VALUE_IDENTIFIER: return "identifier";
        case VALUE_FUNCTION: return "function";
        case VALUE_LIST: return "list";
        case VALUE_ANY: return "any";
    }

    return "none";
}

Value* Eval_GetParameterRaw(Value* ParameterList, ValueType ExpectedType, int Index) {
    if (ParameterList->ListValue->Length <= Index) {
        Error(ParameterList, "Not enough parameters for function");
        exit(1);
    }

    Value* ParameterValue = ParameterList->ListValue->Values[Index];

    if (ParameterValue->Type != ExpectedType && ExpectedType != VALUE_ANY) {
        char MessageBuffer[100] = {0};

        snprintf(MessageBuffer, 100, "Wrong types, expected %s, got %s", Eval_GetTypeName(ExpectedType), Eval_GetTypeName(ParameterValue->Type));

        Error(ParameterValue, MessageBuffer);
        exit(1);
    }

    return ParameterValue;
}

#define Eval_GetParameter(List, Type, Index) Eval_GetParameterRaw(List, Type, Index + 1)

#define Eval_Binary_Int(Name, Operator) Value* Eval_ ## Name(Value* Parameters) { \
    Value* Result = alloc(sizeof(Value));                                             \
    Result->Type = VALUE_INTEGER;                                                     \
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

Environment* Eval_Setup() {
    SymbolMap* Symbols = SymbolMap_New();

#define AddSymbolFunction(Name, Character) SymbolMap_Set(Symbols, #Character, strlen(#Character), SYMBOL_FUNCTION, Eval_ ## Name)

    AddSymbolFunction(Add, +);
    AddSymbolFunction(Sub, -);
    AddSymbolFunction(Mul, *);
    AddSymbolFunction(Div, /);

    Environment* Result = alloc(sizeof(Result));

    Result->Outer = NULL;
    Result->Symbols = Symbols;

    return Result;
}

Value* Eval_Apply(Environment*, Value*);

Value* Eval_AST(Environment* this, Value* Target) {
    Value* OriginalTarget = Target;

    switch (Target->Type) {
        case VALUE_IDENTIFIER:;
            String* IdentifierText = Target->IdentifierValue;

            SymbolEntry* Symbol = Environment_Get(this, IdentifierText);

            if (Symbol != NULL) {
                if (Symbol->Type == SYMBOL_FUNCTION) {
                    Value* Result = alloc(sizeof(Value));

                    Result->Type = VALUE_FUNCTION;
                    Result->FunctionValue = Symbol->Function;

                    Target = Result;
                }
                else if (Symbol->Type == SYMBOL_VALUE) {
                    Target = Symbol->Value;
                }
            }
            else {
                Error(Target, "Undefined symbol");
                exit(1);
            }

            break;
        case VALUE_LIST:
            for (int Index = 0; Index < Target->ListValue->Length; Index++) {
                Target->ListValue->Values[Index] = Eval_Apply(this, Target->ListValue->Values[Index]);
            }
            break;
    }

    CloneContext(Target, OriginalTarget);

    return Target;
}
Value* Eval_Apply(Environment* this, Value* Target) {
    Value* OriginalTarget = Target;

    switch (Target->Type) {
        case VALUE_LIST:
            if (Target->ListValue->Length >= 1) {
                String* Name = Target->ListValue->Values[0]->StringValue;

                if (!strncmp(Name->Buffer, "def!", Name->Length)) {
                    Value* DefKey = Eval_GetParameter(Target, VALUE_IDENTIFIER, 0);
                    Value* DefValue = Eval_GetParameter(Target, VALUE_ANY, 1);

                    Environment_Set(this, DefKey->IdentifierValue, DefValue);

                    Target = Eval_Apply(this, DefValue);

                    break;
                }
                else if (!strncmp(Name->Buffer, "let*", Name->Length)) {
                    Value *BindingsList = Eval_GetParameter(Target, VALUE_LIST, 0);
                    Value *LetBody = Eval_GetParameter(Target, VALUE_ANY, 1);

                    int BindingCount = BindingsList->ListValue->Length;

                    if (BindingCount % 2 != 0) {
                        Error(BindingsList, "Uneven number of `let*` binding elements:");
                        Error(BindingsList->ListValue->Values[BindingCount - 1], "Not part of a binding pair:");
                        exit(1);
                    }

                    BindingCount /= 2;

                    Environment *NewEnvironment = Environment_New(this);

                    for (int PairIndex = 0; PairIndex < BindingCount; PairIndex++) {
                        int ValueIndex = PairIndex * 2;

                        Value *BindingName = Eval_GetParameterRaw(BindingsList, VALUE_IDENTIFIER, ValueIndex);
                        Value *BindingValue = Eval_GetParameterRaw(BindingsList, VALUE_ANY, ValueIndex + 1);

                        BindingValue = Eval_Apply(NewEnvironment, BindingValue);

                        Environment_Set(NewEnvironment, BindingName->IdentifierValue, BindingValue);
                    }

                    Target = Eval_Apply(NewEnvironment, LetBody);

                    free(NewEnvironment);

                    break;
                }
            }

            Target = Eval_AST(this, Target);

            if (Target->ListValue->Length >= 1) {
                Value* Function = Target->ListValue->Values[0];

                if (Function->Type != VALUE_FUNCTION) {
                    Error(Function, "Is not a valid function");
                }

                Target = Function->FunctionValue(Target);
            }
        default:
            Target = Eval_AST(this, Target);
    }

    CloneContext(Target, OriginalTarget);

    return Target;
}