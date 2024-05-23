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

struct expr {
	enum exprkind kind;
	union {
		struct basic b;
	};
};

struct program {
	dafields(struct expr)
};

struct program *parse_program(struct lexer, arena *);

#endif /* !ANDY_PARSER_H */
