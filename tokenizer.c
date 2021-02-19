
#include "tokenizer.h"
#include <ctype.h>

char IsSpecial(char Check) {
    char* Special = "(){}[]'`^@~\" \t\n\r";

    for (int Index = 0; Index < strlen(Special); ++Index) {
        if (Check == Special[Index]) {
            return 1;
        }
    }

    return 0;
}

void Token_Print(Token *this) {
    switch (this->Type) {
        case INTEGER:
            printf("%l", this->IntegerValue);
            break;
        case STRING:
            putchar('"');
            String_Print(this->StringValue);
            putchar('"');
            break;
        case IDENTIFIER:
            String_Print(this->IdentifierValue);
            break;
        case PUNCTUATION:
            switch (this->PunctuationValue) {
                case OPEN_PAREN:
                    putchar('(');
                    break;
                case CLOSE_PAREN:
                    putchar(')');
                    break;
            }
            break;
        case OPERATOR:
            switch (this->OperatorValue) {
                case PLUS:
                    putchar('+');
                    break;
                case MINUS:
                    putchar('-');
                    break;
            }
            break;
    }
}

Tokenizer *Tokenizer_New(char* SourceFilePath, char *Source, int SourceLength) {
    Tokenizer *this = alloc(sizeof(Tokenizer));

    this->SourceFilePath = SourceFilePath;
    this->Source = Source;
    this->SourceLength = SourceLength;
    this->LineNumber = 1;

    this->TokenCapacity = 8;
    this->Tokens = calloc(sizeof(Token *), this->TokenCapacity);

    return this;
}

void Tokenizer_Reset(Tokenizer* this, char* Source, int SourceLength) {
    this->Source = Source;
    this->SourceLength = SourceLength;
    this->LineNumber = 1;

    for (int Index = 0; Index < this->MaxTokenIndex; Index++) {
        free(this->Tokens[Index]);

        this->Tokens[Index] = NULL;
    }

    this->TokenIndex = 0;
    this->MaxTokenIndex = 0;
}

Token *Tokenizer_AppendToken(Tokenizer *this, int Position, int Length, TokenType Type, void *Value) {
    if (this->MaxTokenIndex + 1 >= this->TokenCapacity) {
        this->TokenCapacity += 300;
        this->Tokens = realloc(this->Tokens, this->TokenCapacity * sizeof(Token *));
    }

    Token *NewToken = alloc(sizeof(Token));

    NewToken->Type = Type;
    NewToken->AnyValue = Value;

    NewToken->Context.Length = Length;
    NewToken->Context.Position = Position;
    NewToken->Context.LineNumber = this->LineNumber;
    NewToken->Context.Source = this->Source;
    NewToken->Context.SourceFilePath = this->SourceFilePath;

    this->Tokens[this->MaxTokenIndex++] = NewToken;

    return NewToken;
}
Token *Tokenizer_AppendToken_Int(Tokenizer* this, int Position, int Length, TokenType Type, int Value) {
    return Tokenizer_AppendToken(this, Position, Length, Type, (void*)Value);
}

char Tokenizer_AtEnd(Tokenizer *this) {
    return this->SourceIndex >= this->SourceLength;
}

char Tokenizer_PeekNextCharacter(Tokenizer *this) {
    return this->Source[this->SourceIndex];
}

char Tokenizer_GetNextCharacter(Tokenizer *this) {
    char Result = this->Source[this->SourceIndex++];

    if (Result == 0xA) {
        this->LineNumber++;
    }

    return Result;
}

Token *Tokenizer_GetNextToken(Tokenizer *this) {
    if (this->TokenIndex < this->MaxTokenIndex) {
        return this->Tokens[this->TokenIndex++];
    }

    this->TokenIndex++;

    while (1) {
        int TokenStartPosition = this->SourceIndex;

        if (Tokenizer_AtEnd(this)) {
            return Tokenizer_AppendToken(this, TokenStartPosition, 1, END, NULL);
        }

        char FirstCharacter = Tokenizer_GetNextCharacter(this);

        if (isspace(FirstCharacter) || FirstCharacter == ',') {
            continue;
        }

        if (FirstCharacter == ';') {
            int CurrentLine = this->LineNumber;

            while (this->LineNumber = CurrentLine && !Tokenizer_AtEnd(this)) {
                Tokenizer_GetNextToken(this);
            };

            continue;
        }

#define CharacterToken(Character, Type, Value) case Character: return Tokenizer_AppendToken_Int(this, TokenStartPosition, 1, Type, Value)

        switch (FirstCharacter) {
            CharacterToken('(', PUNCTUATION, OPEN_PAREN);
            CharacterToken(')', PUNCTUATION, CLOSE_PAREN);
            CharacterToken('{', PUNCTUATION, OPEN_BRACE);
            CharacterToken('}', PUNCTUATION, CLOSE_BRACE);
            CharacterToken('[', PUNCTUATION, OPEN_BRACKET);
            CharacterToken(']', PUNCTUATION, CLOSE_BRACKET);
            CharacterToken('\'', PUNCTUATION, SINGLE_QUOTE);
            CharacterToken('`', PUNCTUATION, BACKTICK);
            CharacterToken('^', PUNCTUATION, CARET);
            CharacterToken('@', PUNCTUATION, AT);
            case '~':
                if (Tokenizer_PeekNextCharacter(this) == '@') {
                    return Tokenizer_AppendToken_Int(this, TokenStartPosition, 2, PUNCTUATION, TILDE_AT);
                }

                return Tokenizer_AppendToken_Int(this, TokenStartPosition, 1, PUNCTUATION, TILDE);
        }

        if (FirstCharacter == '"') {
            int StringStart = this->SourceIndex;
            int StringLength = 0;

            FirstCharacter = Tokenizer_PeekNextCharacter(this);

            while (FirstCharacter != '"' && !Tokenizer_AtEnd(this)) {
                FirstCharacter = Tokenizer_GetNextCharacter(this);

                if (FirstCharacter == '\\' && Tokenizer_PeekNextCharacter(this) == '"') {
                    if (Tokenizer_GetNextCharacter(this) == 'n') {
                        StringLength++;
                    }
                }

                StringLength++;
            }

            StringLength -= 1;

            char *StringCopy = calloc(1, StringLength + 1);
            int UnescapedLength = this->SourceIndex - StringStart - 1;

            int EscapeIndex = 0;

            for (int CopyIndex = 0; CopyIndex < UnescapedLength; CopyIndex++) {
                char EscapeCharacter = this->Source[StringStart + CopyIndex];

                if (EscapeCharacter == '\\') {
                    char EscapedCharacter = StringCopy[EscapeIndex] = this->Source[StringStart + ++CopyIndex];

                    if (EscapedCharacter == 'n') {
                        StringCopy[EscapeIndex++] = 13;
                        StringCopy[EscapeIndex] = 10;
                    }
                } else {
                    StringCopy[EscapeIndex] = EscapeCharacter;
                }

                EscapeIndex++;
            }

            String *StringText = String_New(StringCopy, StringLength);

            return Tokenizer_AppendToken(this, TokenStartPosition, UnescapedLength + 2, STRING, StringText);
        }
        else if (!IsSpecial(FirstCharacter)) {
            int IdentifierStart = this->SourceIndex - 1;

            while (!IsSpecial(FirstCharacter) && !Tokenizer_AtEnd(this)) {
                FirstCharacter = Tokenizer_GetNextCharacter(this);
            }

            if (Tokenizer_AtEnd(this) || IsSpecial(FirstCharacter)) {
                this->SourceIndex--;
            }

            int IdentifierLength = this->SourceIndex - IdentifierStart;

            String *IdentifierText = String_New(&this->Source[IdentifierStart], IdentifierLength);

            return Tokenizer_AppendToken(this, IdentifierStart, IdentifierLength, IDENTIFIER, IdentifierText);
        }
    }
}

Token* Tokenizer_Next(Tokenizer* this) {
    return Tokenizer_GetNextToken(this);
}
Token* Tokenizer_Peek(Tokenizer* this) {
    Token* Result = Tokenizer_GetNextToken(this);

    this->TokenIndex--;

    return Result;
}