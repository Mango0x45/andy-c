#ifndef ANDY_LEXER_H
#define ANDY_LEXER_H

#include <uchar.h>

typedef enum {
	LTK_ARG,      /* Argument */
	LTK_BRC_C,    /* Closing brace */
	LTK_BRC_O,    /* Opening brace */
	LTK_NL,       /* End of statement */
	LTK_PIPE,     /* Pipe */
	LTK_PRC_RD,   /* Process reading redirection */
	LTK_PRC_SUB,  /* Process substitution */
	LTK_PRC_WR,   /* Process writing redirection */
	LTK_PRC_RDWR, /* Process reading- and writing redirection */
	LTK_PRN_C,    /* Closing parenthesis */
	LTK_PRN_O,    /* Opening parenthesis */
	LTK_RDR_APP,  /* Appending redirection */
	LTK_RDR_CLB,  /* Clobbering redirection */
	LTK_RDR_RD,   /* Reading redirection */
	LTK_RDR_WR,   /* Writing redirection */
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
