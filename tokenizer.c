
#include "tokenizer.h"
#include <ctype.h>

char IsSpecial(char Check) {
	char* Special = "(){}[]'`^@\" \t\n\r";

	for (int Index = 0; Index < strlen(Special); ++Index) {
		if (Check == Special[Index]) {
			return 1;
		}
	}

	return 0;
}

Tokenizer* NewTokenizer(char* SourceFilePath, char* Source, size_t SourceLength) {
	Tokenizer* this = alloc(sizeof(Tokenizer));

	this->SourceFilePath = SourceFilePath;
	this->Source = Source;
	this->SourceLength = SourceLength;
	this->LineNumber = 1;

	this->TokenCapacity = 8;
	this->Tokens = alloc(this->TokenCapacity * sizeof(Token*));

	return this;
}

Token* AppendToken(Tokenizer* this, int Position, int Length, TokenType Type, void* Value) {
	if (this->MaxTokenIndex + 1 >= this->TokenCapacity) {
		this->TokenCapacity += 300;
		this->Tokens = realloc(this->Tokens, this->TokenCapacity * sizeof(Token*));
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
Token* AppendIntegerToken(Tokenizer* this, int Position, int Length, TokenType Type, int64_t Value) {
	return AppendToken(this, Position, Length, Type, (void*)Value);
}

int IsTokenizerAtEnd(Tokenizer* this) {
	return this->SourceIndex >= this->SourceLength;
}

char PeekNextCharacter(Tokenizer* this) {
	return this->Source[this->SourceIndex];
}

char GetNextCharacter(Tokenizer* this) {
	char Result = this->Source[this->SourceIndex++];

	if (Result == 0xA) {
		this->LineNumber++;
	}

	return Result;
}

Token* GetNextToken(Tokenizer* this) {
	if (this->TokenIndex < this->MaxTokenIndex) {
		return this->Tokens[this->TokenIndex++];
	}

	this->TokenIndex++;

	while (1) {
		int TokenStartPosition = this->SourceIndex;

		if (IsTokenizerAtEnd(this)) {
			return AppendToken(this, TokenStartPosition, 1, TOKEN_TYPE_EOF, NULL);
		}

		char FirstCharacter = GetNextCharacter(this);

		if (isspace(FirstCharacter) || FirstCharacter == ',') {
			continue;
		}

		if (FirstCharacter == ';') {
			int CurrentLine = this->LineNumber;

			while (this->LineNumber == CurrentLine && !IsTokenizerAtEnd(this)) {
				GetNextCharacter(this);
			}

			continue;
		}

#define CharacterToken(Character, Type, Value) case Character: return AppendIntegerToken(this, TokenStartPosition, 1, Type, Value)

		switch (FirstCharacter) {
			CharacterToken('(', TOKEN_TYPE_PUNCTUATION, PUNCTUATION_OPEN_PAREN);
			CharacterToken(')', TOKEN_TYPE_PUNCTUATION, PUNCTUATION_CLOSE_PAREN);
			CharacterToken('{', TOKEN_TYPE_PUNCTUATION, PUNCTUATION_OPEN_BRACE);
			CharacterToken('}', TOKEN_TYPE_PUNCTUATION, PUNCTUATION_CLOSE_BRACE);
			CharacterToken('[', TOKEN_TYPE_PUNCTUATION, PUNCTUATION_OPEN_BRACKET);
			CharacterToken(']', TOKEN_TYPE_PUNCTUATION, PUNCTUATION_CLOSE_BRACKET);
			CharacterToken('\'', TOKEN_TYPE_PUNCTUATION, PUNCTUATION_SINGLE_QUOTE);
			CharacterToken('`', TOKEN_TYPE_PUNCTUATION, PUNCTUATION_BACKTICK);
			CharacterToken('^', TOKEN_TYPE_PUNCTUATION, PUNCTUATION_CARET);
			CharacterToken('@', TOKEN_TYPE_PUNCTUATION, PUNCTUATION_AT);
			case '~':
				if (PeekNextCharacter(this) == '@') {
					GetNextCharacter(this);

					return AppendIntegerToken(this, TokenStartPosition, 2, TOKEN_TYPE_PUNCTUATION, PUNCTUATION_TILDE_AT);
				}

				//return AppendIntegerToken(this, TokenStartPosition, 1, TOKEN_TYPE_PUNCTUATION, PUNCTUATION_TILDE);
			default: break;
		}

		if (FirstCharacter == '"') {
			int StringStart = this->SourceIndex;
			int StringLength = 0;

			FirstCharacter = 0;

			while (FirstCharacter != '"' && !IsTokenizerAtEnd(this)) {
				FirstCharacter = GetNextCharacter(this);

				if (FirstCharacter == '\\' && PeekNextCharacter(this) == '"') {
					if (GetNextCharacter(this) == 'n') {
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

			String *StringText = AdoptString(StringCopy, StringLength);

			return AppendToken(this, TokenStartPosition, UnescapedLength + 2, TOKEN_TYPE_STRING, StringText);
		}
		else if (!IsSpecial(FirstCharacter)) {
			int IdentifierStart = this->SourceIndex - 1;

			while (!IsSpecial(FirstCharacter) && !IsTokenizerAtEnd(this)) {
				FirstCharacter = GetNextCharacter(this);
			}

			if (IsTokenizerAtEnd(this) || IsSpecial(FirstCharacter)) {
				this->SourceIndex--;
			}

			int IdentifierLength = this->SourceIndex - IdentifierStart;

			String *IdentifierText = BorrowString(&this->Source[IdentifierStart], IdentifierLength);

			return AppendToken(this, IdentifierStart, IdentifierLength, TOKEN_TYPE_IDENTIFIER, IdentifierText);
		}
	}
}

Token* Tokenizer_Next(Tokenizer* this) {
	return GetNextToken(this);
}
Token* PeekNextToken(Tokenizer* this) {
	Token* Result = GetNextToken(this);

	this->TokenIndex--;

	return Result;
}
