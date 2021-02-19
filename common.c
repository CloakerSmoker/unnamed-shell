#include "common.h"
#include <stdlib.h>

jmp_buf OnError;

String* String_New(char* Text, int Length) {
    String* this = malloc(sizeof(String));

    this->Buffer = Text;
    this->Length = Length;

    return this;
}
void String_Print(String* this) {
    printf("%.*s", this->Length, this->Buffer);
}

char TranslateColor(char Color) {
    if (Color & BRIGHT) {
        Color ^= BRIGHT;
        Color += 90;
    }
    else {
        Color += 30;
    }

    return Color;
}

void SetTerminalColors(char Foreground, char Background) {
    printf("\x1B[%im", TranslateColor(Foreground));
    printf("\x1B[%im", TranslateColor(Background) + 10);
}

void PrintSpaces(int Count) {
    for (int Index = 0; Index < Count; Index++) {
        putchar(' ');
    }
}

char* FindStartOfLine(char* Start, int Offset) {
    int LastLineStart = 0;

    for (int Index = 0; Index < Offset; Index++) {
        if (Start[Index] == 0xA || Start[Index] == 0xD) {
            LastLineStart = Index;
            break;
        }
    }

    return &Start[LastLineStart];
}

void Context_Alert(ErrorContext* Context, char* Message, char Color) {
    char LineNumberString[100] = {0};

    snprintf(LineNumberString, 100, "%d", Context->LineNumber);

    int LineNumberLength = strlen(LineNumberString);

    if (Message) {
        SetTerminalColors(Color, BLACK);
        printf("%s\n", Message);
    }

    SetTerminalColors(WHITE | BRIGHT, BLACK);

    PrintSpaces(LineNumberLength + 1);
    printf(" [ ");
    SetTerminalColors(BLUE | BRIGHT, BLACK);
    printf("%s", Context->SourceFilePath);
    SetTerminalColors(WHITE | BRIGHT, BLACK);
    printf(" ]\n");

    printf(" %s | ", LineNumberString);

    int OffsetInSource = Context->Position;
    char* LineText = FindStartOfLine(Context->Source, OffsetInSource);
    int PositionInLine = OffsetInSource - (LineText - Context->Source);

    int DashCount = 0;

    for (int LeftIndex = 0; LeftIndex < PositionInLine; LeftIndex++) {
        char NextCharacter = LineText[LeftIndex];

        if (NextCharacter == '\t') {
            printf("    ");
            DashCount += 4;
        }
        else {
            putchar(NextCharacter);
            DashCount += 1;
        }
    }

    int RightIndex = PositionInLine;

    while (1) {
        char NextCharacter = LineText[RightIndex++];

        if (NextCharacter == 0xA || NextCharacter == 0xD || NextCharacter == 0) {
            break;
        }

        putchar(NextCharacter);
    }

    printf("\n");

    PrintSpaces(LineNumberLength + 1);
    printf(" |-");

    for (int DashIndex = 0; DashIndex < DashCount; DashIndex++) {
        putchar('-');
    }

    SetTerminalColors(Color, BLACK);

    for (int ArrowIndex = 0; ArrowIndex < Context->Length; ArrowIndex++) {
        putchar('^');
    }

    SetTerminalColors(WHITE | BRIGHT, BLACK);

    printf("\n");
}

void Context_Error(ErrorContext* Context, char* Message) {
    Context_Alert(Context, Message, RED | BRIGHT);
}

ErrorContext *Context_Clone(ErrorContext* To, ErrorContext* From) {
    memcpy(To, From, sizeof(ErrorContext));

    return To;
}
ErrorContext *Context_Merge(ErrorContext* Left, ErrorContext* Right) {
    if (Left->Position > Right->Position) {
        Left->Position = Right->Position;
    }

    int LeftEnd = Left->Position + Left->Length;
    int RightEnd = Right->Position + Right->Length;

    if (LeftEnd < RightEnd) {
        Left->Length = RightEnd - Left->Position;
    }
    else {
        Left->Length = LeftEnd - Left->Position;
    }

    return Left;
}