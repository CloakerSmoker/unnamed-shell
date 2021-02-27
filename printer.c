#include "printer.h"

void Value_Print(Value* TargetValue) {
	switch (TargetValue->Type) {
		case VALUE_TYPE_LIST:
			putchar('(');

			for (int Index = 0; Index < TargetValue->ListValue->Length; Index++) {
				Value_Print(TargetValue->ListValue->Values[Index]);

				if (Index + 1 != TargetValue->ListValue->Length) {
					putchar(' ');
				}
			}

			putchar(')');
			break;
		case VALUE_TYPE_IDENTIFIER:
			PrintString(TargetValue->IdentifierValue);
			break;
		case VALUE_TYPE_STRING:
			putchar('"');
			PrintString(TargetValue->StringValue);
			putchar('"');
			break;
		case VALUE_TYPE_INTEGER:
			printf("%li", TargetValue->IntegerValue);
			break;
		case VALUE_TYPE_FUNCTION:
			printf("<function>");
			break;
		case VALUE_TYPE_BOOL:
			if (TargetValue->BoolValue) {
				printf("true");
			}
			else {
				printf("false");
			}
			break;
		case VALUE_TYPE_NIL:
			printf("nil");
			break;
		case VALUE_TYPE_CHILD:
			printf("<process %i>", TargetValue->ChildValue->PID);
			break;
		case VALUE_TYPE_ANY:
			break;
	}
}
