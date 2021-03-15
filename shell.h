#ifndef LISHP_SHELL_H
#define LISHP_SHELL_H

#include "common.h"

typedef struct TagCommand {
	struct TagCommand* Last;
	struct TagCommand* Next;
	char* Input;
	size_t Length;
} Command;

void Escape();

void CR();
void LF();

void Cursor(int X, int Y);
void CursorVertical(int Offset);
void CursorHorizontal(int Offset);

#define ERASE_CURSOR_TO_END 0
#define ERASE_START_TO_CURSOR 1

void EraseLine(int Direction);
void Foreground(char Color);

void ClearLine();
void PrintGhostText(char* Text);

#define UP_ARROW 65
#define DOWN_ARROW 66
#define RIGHT_ARROW 67
#define LEFT_ARROW 68

void OnKey(char* InputBuffer, int Index);

char* History(char ArrowKey);
void AddToHistory(char* Input);

#endif //LISHP_SHELL_H
