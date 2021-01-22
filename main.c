#include <stdio.h>
#include "tokenizer.h"
#include "reader.h"
#include "printer.h"

int main() {
    printf("Hello, World!\n");

    char *Input = "(add 1 (sub 2 3))";

    Tokenizer *Tokenizer = Tokenizer_New(Input, strlen(Input));

    Value* Result = ReadForm(Tokenizer);

    Value_Print(Result);

    return 0;
}
