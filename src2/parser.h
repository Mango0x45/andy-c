#ifndef ANDY_PARSER_H
#define ANDY_PARSER_H

#include <dynarr.h>
#include <mbstring.h>

#include "lexer.h"

enum valkind {
	VK_ARG, /* Unquoted string */
};

enum exprkind {
	EK_INVAL = -1, /* Invalid expression */
	EK_BASIC,      /* Basic command */
	EK_BINOP,      /* Binary operator */
};

struct value {
	enum valkind kind;
	union {
		struct u8view arg;
	};
};

struct basic {
	dafields(struct value)
};

struct binop {
	rune op;
	struct expr *lhs, *rhs;
};

struct expr {
	enum exprkind kind;
	union {
		struct basic b;
		struct binop bo;
	};
};

struct program {
	dafields(struct expr)
};

struct program *parse_program(struct lexer, arena *);

#endif /* !ANDY_PARSER_H */
