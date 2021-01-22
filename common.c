#include "common.h"

String* String_New(char* Text, int Length) {
    String* this = malloc(sizeof(String));

    this->Buffer = Text;
    this->Length = Length;

    return this;
}
void String_Print(String* this) {
    printf("%.*s", this->Length, this->Buffer);
}