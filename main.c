#include <stdio.h>
#include "tokenizer.h"
#include "reader.h"
#include "printer.h"
#include "eval.h"

int main(unused int argc, unused char** argv) {
	Environment* Env = SetupEnvironment();

	char* AutoLoad = "(do " \
					 " (def! not (fn* (Value) (if Value false true)))" \
					 " (def! or (fn* (Left Right) (if Left true (if Right true false))))" \
					 " (def! bool->int (fn* (Bool) (if Bool 1 0)))" \
					 " (def! any->bool (fn* (Value) (if Value true false)))" \
					 " (def! . \".\")" \
					 " (def! .. \"..\")" \
					 ")";
	Tokenizer* AutoLoadTokenizer = NewTokenizer("AutoLoad", AutoLoad, strlen(AutoLoad));

	Value* AutoLoadTree = ReadForm(AutoLoadTokenizer);

	Evaluate(Env, AutoLoadTree);

	setjmp(OnError);

	char* Input = NULL;
	size_t Length = 0;

	while (1) {
		printf(">");

		size_t LineLength = getline(&Input, &Length, stdin);

		if (LineLength == 1) {
			break;
		}

		char* CopiedInput = alloc(LineLength + 1);
		memcpy(CopiedInput, Input, LineLength);

		Tokenizer* Tokenizer = NewTokenizer("*", CopiedInput, LineLength);

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
	}

	return 0;
}
