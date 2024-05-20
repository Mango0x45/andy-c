#ifndef ANDY_REPR_H
#define ANDY_REPR_H

#include "lexer.h"

#define _repr(d, x)                                                            \
	_Generic((x),                                                              \
	    struct lextok: repr_lextok,                                            \
	    enum lex_tok_kind: repr_lex_tok_kind)((x), (d))
#define repr(x) _repr(0, (x))

void repr_lextok(struct lextok, int);
void repr_lex_tok_kind(enum lex_tok_kind, int);

#endif /* !ANDY_REPR_H */
