#include "common.h"

#ifndef MAL_TOKENIZER_H
#define MAL_TOKENIZER_H

typedef enum {
	STRING,
	IDENTIFIER,
	PUNCTUATION,
	END,
} TokenType;

typedef enum {
	OPEN_PAREN,
	CLOSE_PAREN,
	OPEN_BRACKET,
	CLOSE_BRACKET,
	OPEN_BRACE,
	CLOSE_BRACE,
	SINGLE_QUOTE,
	BACKTICK,
	TILDE,
	TILDE_AT,
	CARET,
	AT
} Punctuation;

typedef struct {
	ErrorContext Context;
	TokenType Type;

	union {
		int64_t IntegerValue;
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

char IsSpecial(char);
unused void Token_Print(Token*);

Tokenizer* Tokenizer_New(char*, char*, size_t);
unused void Tokenizer_Reset(Tokenizer*, char*, size_t);
Token* Tokenizer_AppendToken(Tokenizer*, int, int, TokenType, void*);
int Tokenizer_AtEnd(Tokenizer*);
char Tokenizer_PeekNextCharacter(Tokenizer*);
char Tokenizer_GetNextCharacter(Tokenizer*);
Token* Tokenizer_GetNextToken(Tokenizer*);
Token* Tokenizer_Next(Tokenizer*);
Token* Tokenizer_Peek(Tokenizer*);

#endif //MAL_TOKENIZER_H
