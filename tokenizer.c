
#include "tokenizer.h"
#include <ctype.h>

char IsSpecial(char Check) {
    char* Special = "(){}[]'`^@~\" \t";

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

Tokenizer *Tokenizer_New(char *Source, int SourceLength) {
    Tokenizer *this = malloc(sizeof(Tokenizer));

    this->Source = Source;
    this->SourceLength = SourceLength;

    this->TokenCapacity = 8;
    this->Tokens = calloc(sizeof(Token *), this->TokenCapacity);

    return this;
}

Token *Tokenizer_AppendToken(Tokenizer *this, TokenType Type, void *Value) {
    if (this->MaxTokenIndex + 1 >= this->TokenCapacity) {
        this->TokenCapacity += 300;
        this->Tokens = realloc(this->Tokens, this->TokenCapacity * sizeof(Token *));
    }

    Token *NewToken = malloc(sizeof(Token));

    NewToken->Type = Type;
    NewToken->AnyValue = Value;

    this->Tokens[this->MaxTokenIndex++] = NewToken;

    return NewToken;
}
Token *Tokenizer_AppendToken_Int(Tokenizer* this, TokenType Type, int Value) {
    return Tokenizer_AppendToken(this, Type, (void*)Value);
}

char Tokenizer_AtEnd(Tokenizer *this) {
    return this->SourceIndex >= this->SourceLength;
}

char Tokenizer_PeekNextCharacter(Tokenizer *this) {
    return this->Source[this->SourceIndex];
}

char Tokenizer_GetNextCharacter(Tokenizer *this) {
    return this->Source[this->SourceIndex++];
}

Token *Tokenizer_GetNextToken(Tokenizer *this) {
    if (this->TokenIndex < this->MaxTokenIndex) {
        return this->Tokens[this->TokenIndex++];
    }

    this->TokenIndex++;

    while (1) {
        if (Tokenizer_AtEnd(this)) {
            return Tokenizer_AppendToken(this, END, NULL);
        }

        char FirstCharacter = Tokenizer_GetNextCharacter(this);

        if (isblank(FirstCharacter) || FirstCharacter == ',') {
            continue;
        }

#define CharacterToken(Character, Type, Value) case Character: return Tokenizer_AppendToken_Int(this, Type, Value)

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
                    return Tokenizer_AppendToken_Int(this, PUNCTUATION, TILDE_AT);
                }

                return Tokenizer_AppendToken_Int(this, PUNCTUATION, TILDE);
            case '+':
                return Tokenizer_AppendToken_Int(this, OPERATOR, PLUS);
            case '-':
                return Tokenizer_AppendToken_Int(this, OPERATOR, MINUS);
        }

        if (FirstCharacter == '"') {
            int StringStart = this->SourceIndex;
            int StringLength = 0;

            FirstCharacter = Tokenizer_PeekNextCharacter(this);

            while (FirstCharacter != '"' && !Tokenizer_AtEnd(this)) {
                FirstCharacter = Tokenizer_GetNextCharacter(this);

                if (FirstCharacter == '\\' && Tokenizer_PeekNextCharacter(this) == '"') {
                    Tokenizer_GetNextCharacter(this);
                }

                StringLength++;
            }

            char *StringCopy = calloc(1, StringLength);
            int UnescapedLength = this->SourceIndex - StringStart - 1;

            int EscapeIndex = 0;

            for (int CopyIndex = 0; CopyIndex < UnescapedLength; CopyIndex++) {
                char EscapeCharacter = this->Source[StringStart + CopyIndex];

                if (EscapeCharacter == '\\') {
                    StringCopy[EscapeIndex] = this->Source[StringStart + ++CopyIndex];
                } else {
                    StringCopy[EscapeIndex] = EscapeCharacter;
                }

                EscapeIndex++;
            }

            String *StringText = String_New(StringCopy, StringLength);

            return Tokenizer_AppendToken(this, STRING, StringText);
        }
        else if (!IsSpecial(FirstCharacter)) {
            int IdentifierStart = this->SourceIndex - 1;

            while (!IsSpecial(FirstCharacter) && !Tokenizer_AtEnd(this)) {
                FirstCharacter = Tokenizer_GetNextCharacter(this);
            }

            if (Tokenizer_AtEnd(this) || IsSpecial(FirstCharacter)) {
                this->SourceIndex--;
            }

            String *IdentifierText = String_New(&this->Source[IdentifierStart], this->SourceIndex - IdentifierStart);

            return Tokenizer_AppendToken(this, IDENTIFIER, IdentifierText);
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