#include "common.h"

#include "tokenizer.h"
#include "reader.h"

Value* ReadList(Tokenizer*);
Value* ReadAtom(Tokenizer*);

Value* ReadForm(Tokenizer* this) {
    Token* FirstToken = Tokenizer_Peek(this);

    if (FirstToken->Type == PUNCTUATION && FirstToken->PunctuationValue == OPEN_PAREN) {
        return ReadList(this);
    }
    else {
        return ReadAtom(this);
    }
}

Value* ReadList(Tokenizer* this) {
    Token* FirstToken = Tokenizer_Next(this);

    if (FirstToken->Type != PUNCTUATION || FirstToken->OperatorValue != OPEN_PAREN) {
        Error(FirstToken, "Expected opening '(' for list");
        exit(1);
    }

    List* Result = malloc(sizeof(List));

    Result->Values = calloc(sizeof(Value*), 30);
    Result->Length = 0;

    Token* NextToken = Tokenizer_Peek(this);

    while (NextToken->Type != PUNCTUATION || NextToken->PunctuationValue != CLOSE_PAREN) {
        Value* LastForm = Result->Values[Result->Length++] = ReadForm(this);

        NextToken = Tokenizer_Peek(this);

        if (NextToken->Type == END) {
            Error(FirstToken, "Unclosed list opened here:");
            Error(LastForm, "Last valid entry:");
            exit(1);
        }
    }

    Tokenizer_Next(this);

    Value* RealResult = malloc(sizeof(Value));

    CloneContext(RealResult, FirstToken);
    MergeContext(RealResult, NextToken);

    RealResult->Type = VALUE_LIST;
    RealResult->ListValue = Result;

    return RealResult;
}

Value* ReadAtom(Tokenizer* this) {
    Token* TokenToConvert = Tokenizer_Next(this);
    Value* Result = alloc(sizeof(Value));

    CloneContext(Result, TokenToConvert);

    if (TokenToConvert->Type == STRING) {
        Result->Type = VALUE_STRING;
        Result->StringValue = TokenToConvert->StringValue;
    }
    else if (TokenToConvert->Type == IDENTIFIER) {
        char IsNumber = 1;

        for (int Index = 0; Index < TokenToConvert->IdentifierValue->Length; Index++) {
            if (!isdigit(TokenToConvert->IdentifierValue->Buffer[Index])) {
                IsNumber = 0;
                break;
            }
        }

        if (IsNumber) {
            Result->Type = VALUE_INTEGER;
            Result->IntegerValue = strtoll(TokenToConvert->IdentifierValue->Buffer, NULL, 10);
        }
        else {
            Result->Type = VALUE_IDENTIFIER;
            Result->IdentifierValue = TokenToConvert->IdentifierValue;
        }
    }

    return Result;
}