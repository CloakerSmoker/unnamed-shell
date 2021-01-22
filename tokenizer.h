#include "common.h"

#ifndef MAL_TOKENIZER_H
#define MAL_TOKENIZER_H

typedef enum {
    INTEGER,
    STRING,
    IDENTIFIER,
    OPERATOR,
    PUNCTUATION,
    END,
} TokenType;

typedef enum {
    PLUS,
    MINUS,
    TIMES,
    DIVIDE
} Operator;

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
    TokenType Type;

    union {
        int64_t IntegerValue;
        String* StringValue;
        String* IdentifierValue;
        Operator OperatorValue;
        Punctuation PunctuationValue;
        void* AnyValue;
    };
} Token;

typedef struct {
    char* Source;
    int SourceIndex;
    int SourceLength;

    Token** Tokens;
    int TokenCapacity;
    int TokenIndex;
    int MaxTokenIndex;
} Tokenizer;

char IsSpecial(char);
void Token_Print(Token*);

Tokenizer* Tokenizer_New(char*, int);
Token* Tokenizer_AppendToken(Tokenizer*, TokenType, void*);
char Tokenizer_AtEnd(Tokenizer*);
char Tokenizer_PeekNextToken(Tokenizer*);
char Tokenizer_GetNextCharacter(Tokenizer*);
Token* Tokenizer_GetNextToken(Tokenizer*);
Token* Tokenizer_Next(Tokenizer*);
Token* Tokenizer_Peek(Tokenizer*);

#endif //MAL_TOKENIZER_H