#include "printer.h"

void Value_Print(Value* TargetValue) {
    switch (TargetValue->Type) {
        case VALUE_LIST:
            putchar('(');

            for (int Index = 0; Index < TargetValue->ListValue->Length; Index++) {
                Value_Print(TargetValue->ListValue->Values[Index]);

                if (Index + 1 != TargetValue->ListValue->Length) {
                    putchar(' ');
                }
            }

            putchar(')');
            break;
        case VALUE_IDENTIFIER:
            String_Print(TargetValue->IdentifierValue);
            break;
        case VALUE_STRING:
            putchar('"');
            String_Print(TargetValue->StringValue);
            putchar('"');
            break;
        case VALUE_INTEGER:
            printf("%lli", TargetValue->IntegerValue);
            break;
    }
}