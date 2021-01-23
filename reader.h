#ifndef MAL_READER_H
#define MAL_READER_H

#include "common.h"
#include "tokenizer.h"
#include <string.h>
#include <ctype.h>

typedef enum {
    VALUE_LIST,
    VALUE_INTEGER,
    VALUE_STRING,
    VALUE_IDENTIFIER,
    VALUE_FUNCTION,
    VALUE_ANY
} ValueType;

struct TagValue;

typedef struct {
    int Length;
    struct TagValue** Values;
} List;

typedef struct TagValue {
    ErrorContext Context;
    ValueType Type;

    union {
        List* ListValue;
        int64_t IntegerValue;
        String* StringValue;
        String* IdentifierValue;
        struct TagValue* (*FunctionValue)(struct TagValue*);
    };
} Value;

Value* ReadForm(Tokenizer*);
Value* ReadList(Tokenizer*);
Value* ReadAtom(Tokenizer*);

#endif //MAL_READER_H
