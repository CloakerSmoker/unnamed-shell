#include <stdio.h>
#include "tokenizer.h"
#include "reader.h"
#include "printer.h"
#include "eval.h"

int main(int argc, char** argv) {
    printf("Hello, World!\n");

    char* Input = "(let* (c 2) c)";

    if (argc >= 2) {
        Input = argv[1];
    }

    Tokenizer *Tokenizer = Tokenizer_New("*", Input, strlen(Input));

    Value* Tree = ReadForm(Tokenizer);

    Value_Print(Tree); printf("\n");

    Environment* Env = Eval_Setup();

    Value* Result = Eval_Apply(Env, Tree);

    printf("Result: %lli\n", Result->IntegerValue);

    return 0;
}
