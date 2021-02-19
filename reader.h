#ifndef MAL_READER_H
#define MAL_READER_H

#include "common.h"
#include "tokenizer.h"
#include <string.h>
#include <ctype.h>

Value* ReadForm(Tokenizer*);
Value* ReadList(Tokenizer*);
Value* ReadAtom(Tokenizer*);

#endif //MAL_READER_H
