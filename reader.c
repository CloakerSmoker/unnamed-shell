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
        printf("Expected opening '(' for list");
        exit(1);
    }

    List* Result = malloc(sizeof(List));

    Result->Values = calloc(sizeof(Value*), 30);
    Result->Length = 0;

    FirstToken = Tokenizer_Peek(this);

    while (FirstToken->Type != PUNCTUATION || FirstToken->PunctuationValue != CLOSE_PAREN) {
        Result->Values[Result->Length++] = ReadForm(this);

        FirstToken = Tokenizer_Peek(this);
    }

    Value* RealResult = malloc(sizeof(Value));

    RealResult->Type = VALUE_LIST;
    RealResult->ListValue = Result;

    return RealResult;
}

Value* ReadAtom(Tokenizer* this) {
    Token* TokenToConvert = Tokenizer_Next(this);
    Value* Result = malloc(sizeof(Value));

    if (TokenToConvert->Type == STRING) {
        Result->Type = VALUE_STRING;
        Result->StringValue = TokenToConvert->StringValue;
    }
    else if (TokenToConvert->Type == IDENTIFIER) {
        Result->Type = VALUE_IDENTIFIER;
        Result->IdentifierValue = TokenToConvert->IdentifierValue;
    }

    return Result;
}