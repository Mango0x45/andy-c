#ifndef ANDY_LEXER_H
#define ANDY_LEXER_H

#include <mbstring.h>

struct lexer {
	const char *file;
	struct u8view sv;
	const char8_t *base;
};

enum lex_tok_kind {
	LTK_ARG,  /* Argument */
	LTK_EOF,  /* End of file */
	LTK_ERR,  /* Lexing error */
	LTK_LAND, /* Logical AND */
	LTK_LOR,  /* Logical OR */
	LTK_NL,   /* End of line */
};

struct lextok {
	enum lex_tok_kind kind;
	struct u8view sv;
};

[[nodiscard]] struct lextok lexnext(struct lexer *);
[[nodiscard]] struct lextok lexpeek(struct lexer);

#endif /* !ANDY_LEXER_H */
