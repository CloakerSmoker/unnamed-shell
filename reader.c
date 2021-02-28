#include "common.h"

#include "tokenizer.h"
#include "reader.h"

Value* ReadForm(Tokenizer* this) {
	Token* FirstToken = PeekNextToken(this);

	if (FirstToken->Type == TOKEN_TYPE_PUNCTUATION) {
		if (FirstToken->PunctuationValue == PUNCTUATION_OPEN_PAREN) {
			return ReadList(this);
		}

#define ReaderMacro(Punctuation, Replacement) else if (FirstToken->PunctuationValue == (Punctuation)) { \
        GetNextToken(this); \
        Value* Quote = NewValue(VALUE_TYPE_IDENTIFIER, BorrowString((Replacement), strlen(Replacement))); \
        CloneContext(Quote, FirstToken);                                                                                                 \
        List* ResultList = NewList(2); \
        ResultList->Values[0] = Quote; \
        Value* QuotedValue = ResultList->Values[1] = ReadForm(this); \
        Value* Result = NewValue(VALUE_TYPE_LIST, ResultList); \
        CloneContext(Result, Quote); \
        MergeContext(Result, QuotedValue); \
        return Result; \
    }

		ReaderMacro(PUNCTUATION_SINGLE_QUOTE, "quote")
		ReaderMacro(PUNCTUATION_BACKTICK, "quasiquote")
		ReaderMacro(PUNCTUATION_TILDE, "unquote")
		ReaderMacro(PUNCTUATION_TILDE_AT, "splice-unquote")
	}
	else {
		return ReadAtom(this);
	}
}

Value* ReadList(Tokenizer* this) {
	Token* FirstToken = GetNextToken(this);

	if (FirstToken->Type != TOKEN_TYPE_PUNCTUATION || FirstToken->PunctuationValue != PUNCTUATION_OPEN_PAREN) {
		Error(FirstToken, "Expected opening '(' for list");
		longjmp(OnError, 0);
	}

	List* Result = alloc(sizeof(List));

	Result->Values = calloc(sizeof(Value*), 30);
	Result->Length = 0;

	Token* NextToken = PeekNextToken(this);

	while (NextToken->Type != TOKEN_TYPE_PUNCTUATION || NextToken->PunctuationValue != PUNCTUATION_CLOSE_PAREN) {
		Value* LastForm = Result->Values[Result->Length++] = ReadForm(this);

		NextToken = PeekNextToken(this);

		if (NextToken->Type == TOKEN_TYPE_EOF) {
			Error(FirstToken, "Unclosed list opened here:");
			Error(LastForm, "Last valid entry:");
			longjmp(OnError, 0);
		}
	}

	GetNextToken(this);

	Value* RealResult = NewValue(VALUE_TYPE_LIST, Result);

	CloneContext(RealResult, FirstToken);
	MergeContext(RealResult, NextToken);

	return RealResult;
}

Value* ReadAtom(Tokenizer* this) {
	Token* TokenToConvert = GetNextToken(this);
	Value* Result = NULL;

	if (TokenToConvert->Type == TOKEN_TYPE_STRING) {
		Result = NewValue(VALUE_TYPE_STRING, TokenToConvert->StringValue);
	}
	else if (TokenToConvert->Type == TOKEN_TYPE_IDENTIFIER) {
		char IsNumber = 1;

		for (int Index = 0; Index < TokenToConvert->IdentifierValue->Length; Index++) {
			if (!isdigit(TokenToConvert->IdentifierValue->Buffer[Index])) {
				IsNumber = 0;
				break;
			}
		}

		if (IsNumber) {
			Result = NewValue(VALUE_TYPE_INTEGER, strtoll(TokenToConvert->IdentifierValue->Buffer, NULL, 10));
		}
		else {
			if (!strncmp(TokenToConvert->IdentifierValue->Buffer, "true", TokenToConvert->IdentifierValue->Length)) {
				Result = NewValue(VALUE_TYPE_BOOL, 1);
			}
			else if (!strncmp(TokenToConvert->IdentifierValue->Buffer, "false", TokenToConvert->IdentifierValue->Length)) {
				Result = NewValue(VALUE_TYPE_BOOL, 0);
			}
			else if (!strncmp(TokenToConvert->IdentifierValue->Buffer, "nil", TokenToConvert->IdentifierValue->Length)) {
				Result = NewValue(VALUE_TYPE_NIL, 0);
			}
			else {
				Result = NewValue(VALUE_TYPE_IDENTIFIER, TokenToConvert->IdentifierValue);
			}
		}
	}
	else {
		Error(TokenToConvert, "Unexpected token in atom");
		longjmp(OnError, 0);
	}

	CloneContext(Result, TokenToConvert);

	return Result;
}
