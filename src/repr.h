/* Macros and functions to output debug information */

#ifndef ANDY_REPR_H
#define ANDY_REPR_H

#include "lexer.h"

#define _repr(d, x) \
	_Generic((x), \
	    struct lextok: repr_lextok, \
	    lex_token_kind_t: repr_lex_token_kind, \
	    struct lex_pcr_flags: repr_lex_pcr_flags, \
	    struct lex_rdr_flags: repr_lex_rdr_flags, \
	    struct lex_var_flags: repr_lex_var_flags)((x), (d))
#define repr(x) _repr(0, (x))

void repr_lextok(struct lextok, int);
void repr_lex_token_kind(lex_token_kind_t, int);
void repr_lex_pcr_flags(struct lex_pcr_flags, int);
void repr_lex_rdr_flags(struct lex_rdr_flags, int);
void repr_lex_var_flags(struct lex_var_flags, int);

#endif /* !ANDY_REPR_H */
