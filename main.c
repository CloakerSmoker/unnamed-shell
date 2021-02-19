#include <stdio.h>
#include "tokenizer.h"
#include "reader.h"
#include "printer.h"
#include "eval.h"



int main(int argc, char** argv) {
    Environment* Env = Eval_Setup();

    // Better ls: (let* (Entries (list.map (ls) (fn* (Entry) (do (print Entry) (print "\n"))))) (list.length Entries))

#if EXTRA_ADDITIONS
    char* AutoLoad = "(do " \
                     " (def! not (fn* (Value) (if Value false true)))" \
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

        int LineLength = getline(&Input, &Length, stdin);

        if (LineLength == 1) {
            break;
        }

        char* CopiedInput = alloc(LineLength + 1);
        memcpy(CopiedInput, Input, LineLength);

        Tokenizer *Tokenizer = Tokenizer_New("*", CopiedInput, LineLength);

        Value *Tree = ReadForm(Tokenizer);

        //Value_Print(Tree);
        //printf("\n");

        Value *Result = Eval_Apply(Env, Tree);

        printf("= ");
        Value_Print(Result);
        printf("\n");
    }

    return 0;
}
