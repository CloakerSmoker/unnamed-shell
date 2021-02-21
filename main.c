#include <stdio.h>
#include "tokenizer.h"
#include "reader.h"
#include "printer.h"
#include "eval.h"

// (list.filter (list.make 1 2 3) (fn* (Entry) (- Entry 1)))

/*
 * Add a 'pipe' type, and allow grabbing std streams from the process type
 * add a pipeline function, which just connects streams, so
 *
 *
 *
 *
 */

int main(unused int argc, unused char** argv) {

	//ChildProcess* Bash = ChildProcess_New("/bin/echo.exe", argv);

	//char* Data = ChildProcess_ReadStream(Bash, STDOUT_FILENO, NULL);

	//printf("Child: %s |\n", Data);

	//return 0;

	Environment* Env = Eval_Setup();

	// Better ls: (let* (Entries (list.map (ls) (fn* (Entry) (do (print Entry) (print "\n"))))) (list.length Entries))

#if EXTRA_ADDITIONS
	char* AutoLoad = "(do " \
					 " (def! not (fn* (Value) (if Value false true)))" \
					 " (def! or (fn* (Left Right) (if Left true (if Right true false))))" \
					 " (def! bool->int (fn* (Bool) (if Bool 1 0)))" \
					 " (def! any->bool (fn* (Value) (if Value true false)))" \
					 " (def! . \".\")" \
					 " (def! .. \"..\")" \
					 ")";
	Tokenizer* AutoLoadTokenizer = Tokenizer_New("AutoLoad", AutoLoad, strlen(AutoLoad));

	Value* AutoLoadTree = ReadForm(AutoLoadTokenizer);

	Eval_Apply(Env, AutoLoadTree);
#endif

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

		Tokenizer* Tokenizer = Tokenizer_New("*", CopiedInput, LineLength);

		Value* Tree = ReadForm(Tokenizer);

		//Value_Print(Tree);
		//printf("\n");

		Value* Result = Eval_Apply(Env, Value_AddReference(Tree));
		//Value* Result = Value_AddReference(Tree);

		Value_Release(Tree);

		printf("= ");
		Value_Print(Result);
		printf("\n");

		Value_Release(Result);
	}

	return 0;
}
