#include "common.h"

#ifndef LISHP_TOKENIZER_H
#define LISHP_TOKENIZER_H

typedef enum {
	TOKEN_TYPE_STRING,
	TOKEN_TYPE_IDENTIFIER,
	TOKEN_TYPE_PUNCTUATION,
	TOKEN_TYPE_EOF,
} TokenType;

typedef enum {
	PUNCTUATION_OPEN_PAREN,
	PUNCTUATION_CLOSE_PAREN,
	PUNCTUATION_OPEN_BRACKET,
	PUNCTUATION_CLOSE_BRACKET,
	PUNCTUATION_OPEN_BRACE,
	PUNCTUATION_CLOSE_BRACE,
	PUNCTUATION_SINGLE_QUOTE,
	PUNCTUATION_BACKTICK,
	PUNCTUATION_TILDE,
	PUNCTUATION_TILDE_AT,
	PUNCTUATION_CARET,
	PUNCTUATION_AT
} Punctuation;

typedef struct {
	ErrorContext Context;
	TokenType Type;

	union {
		String* StringValue;
		String* IdentifierValue;
		Punctuation PunctuationValue;
		void* AnyValue;
	};
} Token;

typedef struct {
	char* SourceFilePath;

	char* Source;
	int SourceIndex;
	size_t SourceLength;
	int LineNumber;

	Token** Tokens;
	size_t TokenCapacity;
	int TokenIndex;
	int MaxTokenIndex;
} Tokenizer;

Tokenizer* NewTokenizer(char* SourceFilePath, char* Source, size_t SourceLength);
int IsTokenizerAtEnd(Tokenizer* this);
Token* GetNextToken(Tokenizer* this);
Token* PeekNextToken(Tokenizer* this);

#endif //LISHP_TOKENIZER_H
