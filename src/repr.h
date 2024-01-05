/* Macros and functions to output debug information */

#ifndef ANDY_REPR_H
#define ANDY_REPR_H

#include "lexer.h"

#define _repr(x, d) \
	_Generic((x), \
	    struct lextok: repr_lextok, \
	    struct lex_rdr_flags: repr_lex_rdr_flags, \
	    struct lex_var_flags: repr_lex_var_flags)((x), (d))
#define repr(x) _repr((x), 0)

void repr_lextok(struct lextok, int);
void repr_lex_rdr_flags(struct lex_rdr_flags, int);
void repr_lex_var_flags(struct lex_var_flags, int);

#endif /* !ANDY_REPR_H */
