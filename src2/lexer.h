#ifndef ANDY_LEXER_H
#define ANDY_LEXER_H

#include <mbstring.h>

enum lex_tok_kind {
	LTK_ARG,  /* Argument */
	LTK_LAND, /* Logical AND */
	LTK_LOR,  /* Logical OR */
	LTK_PIPE, /* Pipe */

	/* All enumeration values after this point represent something that could be
	   the end of a unit */
	_LTK_TERM,

	LTK_EOF,  /* End of file */
	LTK_ERR,  /* Lexing error */
	LTK_NL,   /* End of line */
	LTK_SEMI, /* Semicolon */
};

struct lextok {
	enum lex_tok_kind kind;
	struct u8view sv;
};

struct lexer {
	const char *file;
	struct u8view sv;
	const char8_t *base;
	struct {
		bool exists;
		struct lextok t;
	} next;
};

[[nodiscard]] struct lextok lexnext(struct lexer *);
[[nodiscard]] struct lextok lexpeek(struct lexer *);

#endif /* !ANDY_LEXER_H */
