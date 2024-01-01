#ifndef ANDY_LEXER_H
#define ANDY_LEXER_H

#include <stddef.h>

typedef enum {
	LTK_ARG,
} lex_token_kind_t;

struct lextok {
	const char *p;
	size_t len;
	lex_token_kind_t kind;
};

struct lextoks {
	struct lextok *buf;
	size_t len, cap;
};

void lexstr(const char *, struct lextoks *);

#endif /* !ANDY_LEXER_H */
