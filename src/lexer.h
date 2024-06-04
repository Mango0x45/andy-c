#ifndef ANDY_LEXER_H
#define ANDY_LEXER_H

#include <setjmp.h>

#include <dynarr.h>
#include <mbstring.h>

enum lextokkind {
	LTK_BRC_C, /* Closing brace */
	LTK_BRC_O, /* Opening brace */
	LTK_LAND,  /* Logical AND */
	LTK_LOR,   /* Logical OR */
	LTK_PAR_C, /* Closing parenthesis */
	LTK_PAR_O, /* Opening parenthesis */
	LTK_PIPE,  /* Pipe */
	LTK_VAR,   /* Variable */
	LTK_VARL,  /* Variable length */
	LTK_VAR_C, /* Closing bracket of variable */
	LTK_VAR_O, /* Variable with opening bracket */
	LTK_WORD,  /* Word */

	/* All enumeration values after this point represent something that could be
	   the end of a unit */
	_LTK_TERM,

	LTK_EOF,  /* End of file */
	LTK_NL,   /* End of line */
	LTK_SEMI, /* Semicolon */
};

enum lexstate {
	LS_VAR, /* In a variable index list */
};

struct lextok {
	enum lextokkind kind;
	struct u8view sv;
};

struct lexer {
	const char *file;
	struct u8view sv;
	const char8_t *base;
	struct lextok cur;
	struct {
		bool exists;
		struct lextok t;
	} next;
	jmp_buf *err;
	dynarr(enum lexstate) states;
};

[[nodiscard]] struct lextok lexnext(struct lexer *);
[[nodiscard]] struct lextok lexpeek(struct lexer *);

#endif /* !ANDY_LEXER_H */
