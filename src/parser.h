#ifndef ANDY_PARSER_H
#define ANDY_PARSER_H

#include <dynarr.h>
#include <mbstring.h>
#include <setjmp.h>

#include "lexer.h"

enum valkind : int {
	VK_CONCAT,
	VK_LIST,
	VK_VAR,
	VK_VARL,
	VK_WORD,
};

enum unitkind : int {
	UK_CMD,
	UK_CMPND,
};

enum stmtkind : int {
	SK_ANDOR,
};

struct list {
	dafields(struct value)
};

struct concat {
	struct value *l, *r;
};

struct var {
	struct u8view ident;
	dafields(struct value)
};

struct value {
	enum valkind kind;
	union {
		struct concat c;
		struct list l;
		struct u8view w;
		struct var v;
	};
};

struct cmd {
	dafields(struct value)
};

struct cmpnd {
	dafields(struct stmt)
};

struct unit {
	enum unitkind kind;
	bool neg;
	union {
		struct cmd c;
		struct cmpnd cp;
	};
};

struct pipe {
	dafields(struct unit)
};

struct andor {
	char op; /* ‘&’ or ‘|’ */
	struct andor *l;
	struct pipe *r;
};

struct stmt {
	enum stmtkind kind;
	union {
		struct andor ao;
	};
};

struct program {
	dafields(struct stmt)
};

struct parser {
	struct lexer *l;
	arena *a;
	jmp_buf *err;
};

struct program *parse_program(struct parser);

#endif /* !ANDY_PARSER_H */
