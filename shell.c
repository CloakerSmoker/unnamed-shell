#include "shell.h"

void Escape() {
	printf("\x1b[");
}

void CR() {
	Escape(); printf("1000D");
}
void LF() {
	Escape(); printf("1B");
}
void Cursor(int X, int Y) {
	Escape(); printf("%i;%iH", X, Y);
}

void CursorVertical(int Offset) {
	Escape();

	if (Offset < 0) {
		printf("%iA", -Offset);
	}
	else {
		printf("%iB", Offset);
	}
}
void CursorHorizontal(int Offset) {
	Escape();

	if (Offset < 0) {
		printf("%iD", -Offset);
	}
	else {
		printf("%iC", Offset);
	}
}

void EraseLine(int Direction) {
	Escape(); printf("%iK", Direction);
}
void Foreground(char Color) {
	Escape(); printf("%im", TranslateColor(Color));
}

void ClearLine() {
	CursorHorizontal(-1000);
	EraseLine(ERASE_CURSOR_TO_END);
}
void PrintGhostText(char* Text) {
	size_t Length = strlen(Text);

	Foreground(BRIGHT);

	printf("%s", Text);

	Escape(); printf("0m");

	CursorHorizontal(-Length);
}

Command* LastCommand = NULL;

Command* CurrentCommand = NULL;
char CurrentCommandDisplayed = 0;

void ClearInput() {
	CursorHorizontal(-1000);
	CursorHorizontal(1);
	EraseLine(ERASE_CURSOR_TO_END);
}

char* History(char ArrowKey) {
	if (ArrowKey == LEFT_ARROW) {
		ClearInput();
		CurrentCommand = NULL;
	}

	if (CurrentCommand) {
		if (CurrentCommandDisplayed && ArrowKey == RIGHT_ARROW) {
			ClearInput();
			printf("%s", CurrentCommand->Input);

			return CurrentCommand->Input;
		}
		else if (ArrowKey == UP_ARROW) {
			Command* NewCommand = CurrentCommand->Last;

			if (NewCommand) {
				CurrentCommand = NewCommand;
				ClearInput();
				PrintGhostText(CurrentCommand->Input);
				CurrentCommandDisplayed = 1;
			}
		}
		else if (ArrowKey == DOWN_ARROW) {
			Command* NewCommand = CurrentCommand->Next;

			CurrentCommand = NewCommand;
			ClearInput();

			if (NewCommand) {
				PrintGhostText(CurrentCommand->Input);
				CurrentCommandDisplayed = 1;
			}
		}
	}
	else if (ArrowKey == UP_ARROW) {
		CurrentCommand = LastCommand;

		if (CurrentCommand) {
			ClearInput();
			PrintGhostText(CurrentCommand->Input);
			CurrentCommandDisplayed = 1;
		}
	}

	return NULL;
}
void OnKey(char* InputBuffer, int Index) {
	char NextCharacter = InputBuffer[Index];

	if (CurrentCommand && CurrentCommandDisplayed) {
		if (NextCharacter != CurrentCommand->Input[Index]) {
			CursorHorizontal(-1000);
			CursorHorizontal(1 + Index + 1);
			EraseLine(ERASE_CURSOR_TO_END);
			CurrentCommand = NULL;
			CurrentCommandDisplayed = 0;
		}
	}
	else {
		Command* NextCommand = LastCommand;

		while (NextCommand) {
			if (!strncmp(InputBuffer, NextCommand->Input, Index + 1)) {
				PrintGhostText(&NextCommand->Input[Index + 1]);
				CurrentCommand = NextCommand;
				CurrentCommandDisplayed = 1;
				break;
			}

			NextCommand = NextCommand->Last;
		}
	}
}
void AddToHistory(char* Input) {
	Command* New = alloc(sizeof(Command));

	if (LastCommand) {
		LastCommand->Next = New;
	}

	New->Last = LastCommand;
	New->Input = Input;

	LastCommand = New;

	CurrentCommand = NULL;
}
