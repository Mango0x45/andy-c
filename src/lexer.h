#ifndef ANDY_LEXER_H
#define ANDY_LEXER_H

#include <uchar.h>

typedef enum {
	LTK_ARG,
	LTK_BRC_C,
	LTK_BRC_O,
	LTK_NL,
	LTK_PIPE,
	LTK_PRN_C,
	LTK_PRN_O,
} lex_token_kind_t;

struct lextok {
	const char8_t *p;
	size_t len;
	lex_token_kind_t kind;
};

struct lextoks {
	struct lextok *buf;
	size_t len, cap;
};

void lexstr(const char8_t *, struct lextoks *);

#endif /* !ANDY_LEXER_H */
