#include <stdio.h>
#include "tokenizer.h"
#include "reader.h"
#include "printer.h"
#include "eval.h"
#include "shell.h"

void SetupTermios();
void ResetTermios();
char* ReadLine();

#define ReadCharacter() (char)getchar()

Value* CurrentlyExpanding = NULL;

char** Arguments;

int main(int argc, char** argv) {
	Arguments = argv;

	setvbuf(stdout, NULL, _IONBF, 0);
	SetupTermios(1);

	Environment* Env = SetupEnvironment();

	if (setjmp(OnError)) {
		if (CurrentlyExpanding) {
			printf("While expanding: ");
			Value_Print(CurrentlyExpanding);
			printf("\n");
			CurrentlyExpanding = NULL;
		}

		exit(-1);
	}

	char* AutoLoad = "(do " \
                     " (def! not (fn* (Value) (if Value false true)))" \
                     " (def! or (fn* (Left Right) (if Left true (if Right true false))))" \
                     " (def! bool->int (fn* (Bool) (if Bool 1 0)))" \
                     " (def! any->bool (fn* (Value) (if Value true false)))" \
                     " (macro! # (fn* (... BinaryParts) `(@(list.index BinaryParts 1) @(list.index BinaryParts 0) @(list.index BinaryParts 2))))" \
                     ")"; //

	Tokenizer* AutoLoadTokenizer = NewTokenizer("AutoLoad", AutoLoad, strlen(AutoLoad));

	Value* AutoLoadTree = ReadForm(AutoLoadTokenizer);

	Evaluate(Env, AutoLoadTree);

	if (argc >= 2) {
		FILE* Script = fopen(argv[1], "r");

		if (Script == NULL) {
			exit(-1);
		}

		fseek(Script, 0, SEEK_END);
		int Size = ftell(Script);
		fseek(Script, 0, SEEK_SET);

		char* ScriptText = malloc(Size + 1);
		fread(ScriptText, Size, 1, Script);
		fclose(Script);

		Tokenizer* ScriptTokenizer = NewTokenizer(argv[1], ScriptText, Size);
		Value* ScriptTree = ReadForm(ScriptTokenizer);
		Value* Result = Evaluate(Env, ScriptTree);

		if (Result->Type == VALUE_TYPE_INTEGER) {
			exit(Result->IntegerValue);
		}

		exit(0);
	}

	struct stat ConfigStatDummy;

	if (!stat("~/config.lishp", &ConfigStatDummy)) {
		char* LoadConfig = "(load! (string.concat (env.get \"HOME\") \"/config.lishp\"))";
		Tokenizer* ConfigTokenizer = NewTokenizer("~/config.lishp", LoadConfig, strlen(LoadConfig));
		Value* ConfigTree = ReadForm(ConfigTokenizer);
		Evaluate(Env, ConfigTree);
	}

	while (1) {
		printf(">");
		//char* Input = ReadLine();
		//printf("You entered '%s'\n", Input);

		///*

		char* Input = ReadLine();
		size_t LineLength = strlen(Input);

		if (LineLength == 0) {
			break;
		}

		Tokenizer* Tokenizer = NewTokenizer("*", Input, LineLength + 1);

		Value* Tree = ReadForm(Tokenizer);

		//Value_Print(Tree);
		//printf("\n");

		Value* Result = Evaluate(Env, AddReferenceToValue(Tree));
		//Value* Result = AddReferenceToValue(Tree);

		ReleaseReferenceToValue(Tree);

		printf("= ");
		Value_Print(Result);
		printf("\n");

		ReleaseReferenceToValue(Result);
		 //*/
	}

	ResetTermios();

	return 0;
}

#include <termios.h>
#include <stdio.h>

struct termios OldTermiosConfig;
struct termios TermiosConfig;

void SetupTermios() {
	tcgetattr(0, &OldTermiosConfig);
	TermiosConfig            = OldTermiosConfig;
	TermiosConfig.c_lflag   &= ~(ICANON | ECHO) | ISIG;
	TermiosConfig.c_iflag   &= ~(ISTRIP | INLCR | ICRNL |
	                        IGNCR | IXON | IXOFF);

	TermiosConfig.c_cc[VMIN] = 1;

	tcsetattr(STDIN_FILENO, TCSANOW, &TermiosConfig);
}
void ResetTermios() {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &OldTermiosConfig);
}

char* ReadLine() {
	char* Buffer = alloc(200);
	int BufferSize = 200;
	int BufferIndex = 0;

	while (1) {
		if (BufferIndex >= BufferSize) {
			BufferSize += 200;
			Buffer = realloc(Buffer, BufferSize);
		}

		char NextCharacter = getchar();

		if (NextCharacter == 0x0A || NextCharacter == 0x0D) {
			printf("\n");
			Buffer[BufferIndex] = 0;
			break;
		}
		else if (NextCharacter == 127) {
			if (BufferIndex != 0) {
				CursorHorizontal(-1);
				EraseLine(ERASE_CURSOR_TO_END);
				BufferIndex--;
			}
		}
		else if (NextCharacter == 27) {
			if (getchar() == 91) {
				char ArrowKey = getchar();

				char* HistoryString = History(ArrowKey);

				if (HistoryString) {
					free(Buffer);
					Buffer = strdup(HistoryString);
					BufferSize = BufferIndex = strlen(Buffer);
				}
			}
		}
		else {
			printf("%c", NextCharacter);
			Buffer[BufferIndex] = NextCharacter;
			OnKey(Buffer, BufferIndex);
			BufferIndex += 1;
		}
	}

	if (BufferIndex != 0) {
		AddToHistory(Buffer);
	}

	return Buffer;
}
