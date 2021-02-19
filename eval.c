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

void SymbolMap_Upsert(SymbolMap* this, int Hash, void* Value) {
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
    NewElement->VoidValue = Value;
}

SymbolEntry* SymbolMap_Get(SymbolMap* this, char* Key, int KeyLength) {
    return SymbolMap_GetElement(this, Hash(Key, KeyLength));
}
void SymbolMap_Set(SymbolMap* this, char* Key, int KeyLength, void* Value) {
    return SymbolMap_Upsert(this, Hash(Key, KeyLength), Value);
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

void Environment_Set(Environment* this, String* SymbolName, Value* Value) {
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


char* Eval_GetTypeName(ValueType Type) {
    switch (Type) {
        case VALUE_INTEGER: return "integer";
        case VALUE_STRING: return "string";
        case VALUE_IDENTIFIER: return "identifier";
        case VALUE_FUNCTION: return "function";
        case VALUE_LIST: return "list";
        case VALUE_BOOL: return "bool";
        case VALUE_NIL: return "nil";
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

Value* Eval_Nil() {
    Value* Result = alloc(sizeof(Value));

    Result->Type = VALUE_NIL;

    return Result;
}

#define EVAL_FUNCTION __unused
#define unused __unused

EVAL_FUNCTION Value* Eval_Quit(Value* Parameters) {
    int ExitCode = 0;

    if (Parameters->ListValue->Length >= 2) {
        ExitCode = Eval_GetParameter(Parameters, VALUE_INTEGER, 0)->IntegerValue;
    }

    exit(ExitCode);
}
EVAL_FUNCTION Value* Eval_ListFiles(unused Value* Parameters) {
    DIR* CurrentDirectory = opendir(".");

    if (CurrentDirectory) {
        int EntryCount = 0;

        while (readdir(CurrentDirectory) != NULL) {
            EntryCount += 1;
        }

        seekdir(CurrentDirectory, 0);

        struct dirent* NextDirectoryEntry;
        Value** Values = calloc(sizeof(Value*), EntryCount);
        int Index = 0;

        while ((NextDirectoryEntry = readdir(CurrentDirectory)) != NULL) {
            int NameLength = strlen(NextDirectoryEntry->d_name);

            char* NameString = alloc(NameLength + 2);
            memcpy(NameString, NextDirectoryEntry->d_name, NameLength);

            if (NextDirectoryEntry->d_type == DT_DIR) {
                NameString[NameLength++] = '/';
            }

            Value* NextValue = Values[Index++] = alloc(sizeof(Value));

            NextValue->Type = VALUE_STRING;
            NextValue->StringValue = String_New(NameString, NameLength);
        }

        List* ListResult = alloc(sizeof(List));

        ListResult->Length = EntryCount;
        ListResult->Values = Values;

        Value* Result = alloc(sizeof(Value));

        Result->Type = VALUE_LIST;
        Result->ListValue = ListResult;

        return Result;
    }

    return Eval_Nil();
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

    return Eval_Nil();
}
EVAL_FUNCTION Value* Eval_ChangeDirectory(Value* Parameters) {
    String* TargetDirectory = Eval_GetParameter(Parameters, VALUE_STRING, 0)->StringValue;

    int ErrorCode = chdir(TargetDirectory->Buffer);

    Value* Result = alloc(sizeof(Value));

    Result->Type = VALUE_INTEGER;
    Result->IntegerValue = ErrorCode;

    return Result;
}
EVAL_FUNCTION Value* Eval_GetCurrentDirectory(Value* Parameters) {
    char* Buffer;

    Buffer = getcwd(NULL, 0);

    String* ResultString = String_New(Buffer, strlen(Buffer));

    Value* Result = alloc(sizeof(Value));

    Result->Type = VALUE_STRING;
    Result->StringValue = ResultString;

    return Result;
}

EVAL_FUNCTION Value* Eval_ListMake(Value* Parameters) {
    int Length = Parameters->ListValue->Length - 1;

    List* ResultList = alloc(sizeof(List));
    ResultList->Length = Length;
    ResultList->Values = alloc(Length * sizeof(Value*));

    memcpy(ResultList->Values, &Parameters->ListValue->Values[1], Length * sizeof(Value*));

    Value* Result = alloc(sizeof(Value));
    Result->Type = VALUE_LIST;
    Result->ListValue = ResultList;

    return Result;
}
EVAL_FUNCTION Value* Eval_ListLength(Value* Parameters) {
    Value* TargetValue = Eval_GetParameter(Parameters, VALUE_LIST, 0);

    Value* Result = alloc(sizeof(Value));

    Result->Type = VALUE_INTEGER;
    Result->IntegerValue = TargetValue->ListValue->Length;

    return Result;
}
EVAL_FUNCTION Value* Eval_ListIndex(Value* Parameters) {
    List* TargetList = Eval_GetParameter(Parameters, VALUE_LIST, 0)->ListValue;
    int64_t TargetIndex = Eval_GetParameter(Parameters, VALUE_INTEGER, 1)->IntegerValue;

    if (TargetIndex >= TargetList->Length || TargetIndex < 0) {
        return Eval_Nil();
    }

    return TargetList->Values[TargetIndex];
}
EVAL_FUNCTION Value* Eval_ListMap(Value* Parameters) {
    List* Elements = Eval_GetParameter(Parameters, VALUE_LIST, 0)->ListValue;
    Function* MapFunction = Eval_GetParameter(Parameters, VALUE_FUNCTION, 1)->FunctionValue;

    List* ResultList = alloc(sizeof(List));
    ResultList->Length = Elements->Length;
    ResultList->Values = alloc(Elements->Length * sizeof(Value*));

    List* SingleValueList = alloc(sizeof(List));
    SingleValueList->Length = 2;
    SingleValueList->Values = alloc(SingleValueList->Length * sizeof(Value*));

    Value* SingleValueListValue = alloc(sizeof(Value));
    SingleValueListValue->Type = VALUE_LIST;
    SingleValueListValue->ListValue = SingleValueList;

    for (int Index = 0; Index < Elements->Length; Index++) {
        Value* NextValue = Elements->Values[Index];

        SingleValueList->Values[1] = NextValue;

        if (MapFunction->IsNativeFunction) {
            NextValue = MapFunction->NativeValue(SingleValueListValue);
        }
        else {
            if (MapFunction->ParameterBindings->ListValue->Length != 1) {
                Error(NextValue, "Incorrect number of parameters passed to function");
                longjmp(OnError, 0);
            }

            Environment* Closure = Environment_New_Bindings(MapFunction->Environment, MapFunction->ParameterBindings, SingleValueListValue);

            NextValue = Eval_Apply(Closure, MapFunction->Body);
        }

        ResultList->Values[Index] = NextValue;
    }

    free(SingleValueListValue);
    free(SingleValueList);

    Value* Result = alloc(sizeof(List));

    Result->Type = VALUE_LIST;
    Result->ListValue = ResultList;

    return Result;
}
EVAL_FUNCTION Value* Eval_ListPush(Value* Parameters) {
    List* TargetList = Eval_GetParameter(Parameters, VALUE_LIST, 0)->ListValue;

    int AdditionalElementCount = Parameters->ListValue->Length - 2;
    int OldElementCount = TargetList->Length;
    int NewElementCount = OldElementCount + AdditionalElementCount;

    List* ResultList = alloc(sizeof(List));
    ResultList->Length = NewElementCount;
    ResultList->Values = alloc(NewElementCount * sizeof(Value*));

    memcpy(ResultList->Values, TargetList->Values, OldElementCount * sizeof(Value*));

    for (int Index = 0; Index < AdditionalElementCount; Index++) {
        Value* NextValue = Parameters->ListValue->Values[Index + 2];

        ResultList->Values[OldElementCount + Index] = NextValue;
    }

    Value* Result = alloc(sizeof(Value));
    Result->Type = VALUE_LIST;
    Result->ListValue = ResultList;

    return Result;
}

EVAL_FUNCTION Value* Eval_StringLength(Value* Parameters) {
    Value* TargetValue = Eval_GetParameter(Parameters, VALUE_STRING, 0);

    Value* Result = alloc(sizeof(Value));

    Result->Type = VALUE_INTEGER;
    Result->IntegerValue = TargetValue->StringValue->Length;

    return Result;
}
EVAL_FUNCTION Value* Eval_StringSplit(Value* Parameters) {
    String* TargetString = Eval_GetParameter(Parameters, VALUE_STRING, 0)->StringValue;

    List* ResultList = alloc(sizeof(List));
    ResultList->Length = TargetString->Length;
    ResultList->Values = alloc(TargetString->Length * sizeof(Value*));

    for (int Index = 0; Index < TargetString->Length; Index++) {
        Value* NextCharacterValue = alloc(sizeof(Value));
        NextCharacterValue->Type = VALUE_STRING;
        NextCharacterValue->StringValue = String_New(&TargetString->Buffer[Index], 1);

        ResultList->Values[Index] = NextCharacterValue;
    }

    Value* Result = alloc(sizeof(Value));
    Result->Type = VALUE_LIST;
    Result->ListValue = ResultList;

    return Result;
}

int Value_Equals(Value* Left, Value* Right) {
    if (Left->Type != Right->Type) {
        return 0;
    }

    switch (Left->Type) {
        case VALUE_BOOL:
        case VALUE_FUNCTION:
        case VALUE_INTEGER: return Left->IntegerValue == Right->IntegerValue;
        case VALUE_NIL: return 1;
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
    }

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

    Value* Result = alloc(sizeof(Value));
    Result->Type = VALUE_BOOL;
    Result->BoolValue = ResultValue;

    return Result;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "bugprone-sizeof-expression"
Environment* Eval_Setup() {
    SymbolMap* Symbols = SymbolMap_New();

#define AddSymbolFunction(Name, LispName)      \
do {                                     \
Value* FunctionValue = alloc(sizeof(Value));    \
Function* Function = alloc(sizeof(Function));   \
Function->IsNativeFunction = 1;                 \
Function->NativeValue = Eval_ ## Name;          \
FunctionValue->Type = VALUE_FUNCTION;           \
FunctionValue->FunctionValue = Function;        \
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

    Environment* Result = alloc(sizeof(Result));

    Result->Outer = NULL;
    Result->Symbols = Symbols;

    return Result;
}
#pragma clang diagnostic pop

Value* Eval_AST(Environment* this, Value* Target) {
    Value* OriginalTarget = Target;

    switch (Target->Type) {
        case VALUE_IDENTIFIER:;
            String* IdentifierText = Target->IdentifierValue;

            SymbolEntry* Symbol = Environment_Get(this, IdentifierText);

            if (Symbol != NULL) {
                Target = Symbol->Value;
            }
            else {
                Error(Target, "Undefined symbol");
                longjmp(OnError, 0);
            }

            break;
        case VALUE_LIST:;
            List* ResultList = alloc(sizeof(List));
            ResultList->Length = Target->ListValue->Length;
            ResultList->Values = alloc(Target->ListValue->Length * sizeof(Value*));

            for (int Index = 0; Index < Target->ListValue->Length; Index++) {
                ResultList->Values[Index] = Eval_Apply(this, Target->ListValue->Values[Index]);
            }

            Target = alloc(sizeof(Value));
            Target->Type = VALUE_LIST;
            Target->ListValue = ResultList;

            break;
        default: break;
    }

    CloneContext(Target, OriginalTarget);

    return Target;
}

Value* Eval_ToBool(Value* Target) {
    char BoolValue = 1;

    if (Target->Type == VALUE_NIL) {
        BoolValue = 0;
    }
    else if (Target->Type == VALUE_INTEGER && Target->IntegerValue == 0) {
        BoolValue = 0;
    }
    else if (Target->Type == VALUE_BOOL) {
        BoolValue = Target->BoolValue;
    }

    Target->Type = VALUE_BOOL;
    Target->BoolValue = BoolValue;

    return Target;
}

Value* Eval_Apply(Environment* this, Value* Target) {
    Value* OriginalTarget = Target;

    switch (Target->Type) {
        case VALUE_LIST:
            if (Target->ListValue->Length >= 1) {
                Value* NameValue = Target->ListValue->Values[0];

                if (NameValue->Type == VALUE_IDENTIFIER) {
                    String* Name = NameValue->IdentifierValue;

                    if (!strncmp(Name->Buffer, "def!", Name->Length)) {
                        Value* DefKey = Eval_GetParameter(Target, VALUE_IDENTIFIER, 0);
                        Value* DefValue = Eval_GetParameter(Target, VALUE_ANY, 1);

                        Target = Eval_Apply(this, DefValue);

                        Environment_Set(this, DefKey->IdentifierValue, Target);

                        break;
                    }
                    else if (!strncmp(Name->Buffer, "let*", Name->Length)) {
                        Value *BindingsList = Eval_GetParameter(Target, VALUE_LIST, 0);
                        Value *LetBody = Eval_GetParameter(Target, VALUE_ANY, 1);

                        int BindingCount = BindingsList->ListValue->Length;

                        if (BindingCount % 2 != 0) {
                            Error(BindingsList, "Uneven number of `let*` binding elements:");
                            Error(BindingsList->ListValue->Values[BindingCount - 1], "Not part of a binding pair:");
                            longjmp(OnError, 0);
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
                    else if (!strncmp(Name->Buffer, "if", Name->Length)) {
                        Value* Condition = Eval_GetParameter(Target, VALUE_ANY, 0);
                        Value* TrueBranch = Eval_GetParameter(Target, VALUE_ANY, 1);
                        Value* FalseBranch = NULL;

                        if (Target->ListValue->Length == 4) {
                            FalseBranch = Eval_GetParameter(Target, VALUE_ANY, 2);
                        }

                        Condition = Eval_ToBool(Eval_Apply(this, Condition));

                        if (Condition->BoolValue == 1) {
                            Target = Eval_Apply(this, TrueBranch);
                        }
                        else if (FalseBranch != NULL) {
                            Target = Eval_Apply(this, FalseBranch);
                        }
                        else {
                            Target = alloc(sizeof(Value));

                            Target->Type = VALUE_NIL;
                        }

                        break;
                    }
                    else if (!strncmp(Name->Buffer, "do", Name->Length)) {
                        Value* LastValue = NULL;

                        for (int Index = 1; Index < Target->ListValue->Length; Index++) {
                            LastValue = Eval_Apply(this, Target->ListValue->Values[Index]);
                        }

                        if (LastValue == NULL) {
                            LastValue = Eval_Nil();
                        }

                        Target = LastValue;

                        break;
                    }
                    else if (!strncmp(Name->Buffer, "fn*", Name->Length)) {
                        Value* BindingNames = Eval_GetParameter(Target, VALUE_LIST, 0);
                        Value* Body = Eval_GetParameter(Target, VALUE_ANY, 1);

                        for (int Index = 0; Index < BindingNames->ListValue->Length; Index++) {
                            Eval_GetParameterRaw(BindingNames, VALUE_IDENTIFIER, Index);
                        }

                        Function* NewFunction = alloc(sizeof(Function));

                        NewFunction->ParameterBindings = BindingNames;
                        NewFunction->Body = Body;
                        NewFunction->Environment = this;

                        Value* NewFunctionValue = alloc(sizeof(Value));

                        NewFunctionValue->Type = VALUE_FUNCTION;
                        NewFunctionValue->FunctionValue = NewFunction;

                        CloneContext(NewFunctionValue, Target);

                        Target = NewFunctionValue;

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

                        int Length = ftell(FileDescriptor);

                        fseek(FileDescriptor, 0, SEEK_SET);

                        char* Buffer = alloc(Length + 1);

                        fread(Buffer, 1, Length, FileDescriptor);

                        Tokenizer* FileTokenizer = Tokenizer_New(FileName->Buffer, Buffer, strlen(Buffer));

                        Value* FileTree = ReadForm(FileTokenizer);

                        //Value_Print(FileTree);

                        Target = Eval_Apply(this, FileTree);

                        break;
                    }
                }
            }

            Target = Eval_AST(this, Target);

            if (Target->ListValue->Length >= 1) {
                Value* FunctionNode = Target->ListValue->Values[0];

                if (FunctionNode->Type != VALUE_FUNCTION) {
                    Error(FunctionNode, "Is not a valid function");
                    longjmp(OnError, 0);
                }

                Function* TargetFunction = FunctionNode->FunctionValue;

                if (TargetFunction->IsNativeFunction) {
                    Target = TargetFunction->NativeValue(Target);
                }
                else {
                    int ParameterCount = Target->ListValue->Length - 1;

                    if (ParameterCount != TargetFunction->ParameterBindings->ListValue->Length) {
                        Error(Target, "Incorrect number of parameters passed to function");
                        longjmp(OnError, 0);
                    }

                    Environment* Closure = Environment_New_Bindings(TargetFunction->Environment, TargetFunction->ParameterBindings, Target);

                    Target = Eval_Apply(Closure, TargetFunction->Body);
                }
            }

            break;
        default:
            Target = Eval_AST(this, Target);
    }

    CloneContext(Target, OriginalTarget);

    return Target;
}