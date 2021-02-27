#ifndef LISHP_READER_H
#define LISHP_READER_H

#include "common.h"
#include "tokenizer.h"
#include <string.h>
#include <ctype.h>

Value* ReadForm(Tokenizer*);
Value* ReadList(Tokenizer*);
Value* ReadAtom(Tokenizer*);

#endif //LISHP_READER_H
