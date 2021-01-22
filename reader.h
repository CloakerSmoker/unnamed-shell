#ifndef MAL_READER_H
#define MAL_READER_H

#include "common.h"
#include "tokenizer.h"

typedef enum {
    VALUE_LIST,
    VALUE_INTEGER,
    VALUE_STRING,
    VALUE_IDENTIFIER
} ValueType;

struct Value;

typedef struct {
    int Length;
    struct Value** Values;
} List;

typedef struct Value {
    ValueType Type;

    union {
        List* ListValue;
        int64_t IntegerValue;
        String* StringValue;
        String* IdentifierValue;
    };
} Value;

Value* ReadForm(Tokenizer*);
Value* ReadList(Tokenizer*);
Value* ReadAtom(Tokenizer*);

#endif //MAL_READER_H
